#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <limits.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "web_socket_server.h"

#define BUF_LEN 2048

#define prepareBuffer frame_size = BUF_LEN; memset(frame_buf, 0, BUF_LEN);

#ifdef DEBUGLOGGING
	#define Log(format, ...) printf(format, ## __VA_ARGS__);fflush(stdout)
#else
	#define Log(format, ...)
#endif

WebSocketServer::WebSocketServer(int port_, int max_queue_len_, char *cert, char *pkey)
{
	port = port_;
	max_queue_len = max_queue_len_;
	//strcpy(address, address_);
	pthread_cond_init(&clients_not_empty_cond, NULL);
	clients_mutex = PTHREAD_MUTEX_INITIALIZER;
	connection_listener = -1;
	message_listener = -1;
	id_counter = 0;
	certificate_file.assign(cert);
	private_key_file.assign(pkey);
}

int WebSocketServer::DoHandshake(Client &client)
{
}

int WebSocketServer::ListenSocket()
{
	sockaddr_in saddr;

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	//inet_aton(address, &(saddr.sin_addr));
	saddr.sin_addr.s_addr = INADDR_ANY;

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	if (bind(server_socket, (struct sockaddr*)(&saddr), sizeof(saddr)) == -1)
	{
		last_error = errno;
		return -1;
	}
	if (listen(server_socket, max_queue_len) == -1)
	{
		last_error = errno;
		return -1;
	}
	return 0;
}

int WebSocketServer::InitSSL()
{
	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();
	tls_ssl_context = SSL_CTX_new(SSLv3_server_method());
	if (tls_ssl_context == NULL)
	{
		return -1;
	}
	if (SSL_CTX_set_cipher_list(tls_ssl_context, "ALL") == 0)
	{
		return -2;
	}
	if (SSL_CTX_use_certificate_file(tls_ssl_context, certificate_file.c_str(), SSL_FILETYPE_PEM) != 1)
	{
		return -3;
	}
	if (SSL_CTX_use_PrivateKey_file(tls_ssl_context, private_key_file.c_str(),  SSL_FILETYPE_PEM) != 1)
	{
		return -4;
	}
	SSL_CTX_set_verify(tls_ssl_context, SSL_VERIFY_NONE, NULL);
	return 0;
}

int WebSocketServer::GenerateID()
{
	//TODO: replace incrementing with rng
	// OR replace it all with just incrementing same variable to get new id
	for (int i = 0; i < INT_MAX; i ++)
	{
		if (clients.count(id_counter + i) == 0)
		{
			id_counter = id_counter + i;
			return id_counter;
		}
	}
	return -1;
}

void *WebSocketServer::ListenConnections(void *arg)
{
	WebSocketServer *me = (WebSocketServer*)arg;
	int ret, id;

	ret = me->InitSSL();
	if (ret < 0)
	{
		Log("SSL initialization failed (%i)\n", ret);
		return NULL;
	}

	ret = me->ListenSocket();
	if (ret < 0)
	{
		Log("Failed to bind/listen\n");
		return NULL;
	}
	Log("Bound and listening\n");
	while (!(me->stop))
	{
		Client new_client;
		ret = new_client.Accept(me->server_socket, me->tls_ssl_context);
		//ERR_print_errors_fp (stdout);
		Log("Someone has connected\n");
		if (ret == 0)
		{
			new_client.SetNonBlocking();
			me->LockClientsList();
			id  = me->GenerateID();
			if (id >= 0)
			{
				new_client.set_id(id);
				me->clients[id] = new_client;
				Log("Added client with id %i (socket fd %i)\n", id, new_client.get_socket());
				if (me->clients.size() == 1)
				{
					me->TellThatListIsNotEmpty();
				}
			}
			else
			{
				Log("Generated invalid id %i, disconnecting\n", id);
				new_client.CloseSocket();
			}
			me->ReleaseClientsList();
		}
		else
			Log("Disconnected without handshaking(%i). Errno tells: %s\n", ret, strerror(new_client.get_last_error()));
	}
	
	me->LockClientsList();
	for (auto it = me->clients.begin(); it != me->clients.end(); it++)
	{
		it->second.Disconnect();
	}
	me->clients.clear();
	me->ReleaseClientsList();	
}

int WebSocketServer::LockClientsList()
{
	if (pthread_mutex_lock(&clients_mutex))
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

int WebSocketServer::ReleaseClientsList()
{
	if (pthread_mutex_unlock(&clients_mutex))
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

void WebSocketServer::WaitUntilListIsNotEmpty()
{
	pthread_mutex_lock(&clients_mutex);
	if (clients.size() == 0)
	{
		pthread_cond_wait(&clients_not_empty_cond, &clients_mutex);
	}
	//pthread_mutex_unlock(&clients_mutex);
}

void WebSocketServer::TellThatListIsNotEmpty()
{
	pthread_cond_signal(&clients_not_empty_cond);
}

void *WebSocketServer::ListenMessages(void *arg)
{
	WebSocketServer *me = (WebSocketServer*)arg;
	char data_buf[BUF_LEN];
	int ret;
	int data_len;
	fd_set fds;
	int nfds;
	struct timeval timeout;
	
	while (!(me->stop))
	{
		//Log("%i clients\n", me->clients.size());
		me->WaitUntilListIsNotEmpty();
		//Log("Filling set\n");
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		FD_ZERO(&fds);
		nfds = -1;
		for (auto it = me->clients.begin(); it != me->clients.end(); it++)
		{
			//Log("Adding to select list fd %i\n", it->second.get_socket());
			FD_SET(it->second.get_socket(), &fds);
			if (it->second.get_socket() > nfds)
			{
				nfds = it->second.get_socket();
			}
		}
		nfds ++;
		me->ReleaseClientsList();
		
		//Log("Selecting\n");
		ret = select(nfds, &fds, NULL, NULL, &timeout);
		//Log("Select returned %i\n", ret);
		
		me->LockClientsList();
		for (auto it = me->clients.begin(); it != me->clients.end();)
		{
			ret = 0;
			if (FD_ISSET(it->second.get_socket(), &fds))
			{
				Log("Next client (fd %i, id %i)\n", it->second.get_socket(), it->second.get_id());
				do{
					//Log("Trying to recieve\n");
					data_len = BUF_LEN;
					ret = it->second.Receive(data_buf, &data_len);
					Log("Receive returned %i\n", ret);
					if (ret == READ_SUCCEED || ret == READ_SUCCEED_DATA_AVAILABLE)
					{
						me->OnMessage(it->second, data_buf, data_len);
						//it++;
						//Log("onmessage returned\n");
					}
					else if (ret < 0)
					{
						//ret < 0 in this branch
						Log("Disconnecting (fd %i, id %i)\n", it->second.get_socket(), it->second.get_id());
						it->second.Disconnect();
						me->OnDisconnect(it->second);
					}
					//Log("Here\n");
				}while (ret == READ_SUCCEED_DATA_AVAILABLE || ret == HANDSHAKE_SUCCEED);
			}
			if (ret >= 0)
			{
				it ++;
			}
			else
			{
				Log("Erasing (fd %i, id %i)\n", it->second.get_socket(), it->second.get_id());
				me->clients.erase(it++);
			}
		}
		//Log("releasing list\n");
		me->ReleaseClientsList();
	}
	
	return NULL;
}

int WebSocketServer::Run()
{
	stop = false;
	if (pthread_create(&connection_listener, NULL, WebSocketServer::ListenConnections, this) < 0)
	{
		Log("Could not start connection listener\n");
		last_error = errno;
		connection_listener = -1;
		return -1;
	}
	if (pthread_create(&message_listener, NULL, WebSocketServer::ListenMessages, this) < 0)
	{
		last_error = errno;
		stop = true;
		pthread_join(connection_listener, NULL);
		connection_listener = -1;
		message_listener = -1;
		return -2;
	}
	printf("starting to listen messages\n");
	//WebSocketServer::ListenMessages(this);
	return 0;
}

void WebSocketServer::Stop()
{
	stop = true;
	if (connection_listener != -1 && message_listener != -1)
	{
		pthread_join(connection_listener, NULL);
		pthread_join(message_listener, NULL);
	}
}

Client *WebSocketServer::get_client(int id)
{
	auto it = clients.find(id);
	if (it != clients.end())
	{
		return &(it->second);
	}
	else
	{
		return NULL;
	}
}
