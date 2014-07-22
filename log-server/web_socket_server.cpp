#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "web_socket_server.h"

#define BUF_LEN 2048

#define prepareBuffer frame_size = BUF_LEN; memset(frame_buf, 0, BUF_LEN);

#ifdef DEBUGLOGGING
	#define Log(format, ...) printf(format, ## __VA_ARGS__)
#else
	#define Log(format, ...)
#endif

WebSocketServer::WebSocketServer(int port_, const char *address_, int max_queue_len_)
{
	port = port_;
	max_queue_len = max_queue_len_;
	strcpy(address, address_);
	pthread_cond_init(&clients_not_empty_cond, NULL);
	clients_mutex = PTHREAD_MUTEX_INITIALIZER;
	connection_listener = -1;
	message_listener = -1;
}

int WebSocketServer::DoHandshake(Client &client)
{
}

int WebSocketServer::ListenSocket()
{
	sockaddr_in saddr;

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	inet_aton(address, &(saddr.sin_addr));

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
	if (SSL_CTX_use_certificate_file(tls_ssl_context, "/home/goon/pleskcert.pem",  SSL_FILETYPE_PEM) != 1)
	{
		return -3;
	}
	if (SSL_CTX_use_PrivateKey_file(tls_ssl_context, "/home/goon/pleskpkey.pem",  SSL_FILETYPE_PEM) != 1)
	{
		return -4;
	}
	SSL_CTX_set_verify(tls_ssl_context, SSL_VERIFY_NONE, NULL);
	return 0;
}

void *WebSocketServer::ListenConnections(void *arg)
{
	WebSocketServer *me = (WebSocketServer*)arg;
	int ret;

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
		ERR_print_errors_fp (stdout);
		Log("Someone has connected\n");
		if (ret == 0)
		{
			new_client.SetNonBlocking();
			me->LockClientsList();
			me->clients.insert(new_client);
			if (me->clients.size() == 1)
			{
				me->TellThatListIsNotEmpty();
			}
			me->ReleaseClientsList();
		}
		else
			Log("Disconnected without handshaking(%i). Errno tells: %s\n", ret, strerror(new_client.get_last_error()));
	}
	
	me->LockClientsList();
	for (std::set<Client,ClientComparator>::iterator it = me->clients.begin(); it != me->clients.end(); it++)
	{
		//This is normally a bad idea bacause a copy of the element in set is
		//being modified, but here we don't care since we clear the set
		//right after that and Disconnect will do just what we want anyway 
		Client cur = *it;
		cur.Disconnect();
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

/*void WebSocketServer::DisconnectClient(Client who)
{
	unsigned char frame_buf[BUF_LEN];
	unsigned int frame_size;
	
	//TODO: if connection reset by other side then just close socket
	wsMakeFrame(NULL, 0, frame_buf, &frame_size, WS_CLOSING_FRAME);
	SendFrame(who, frame_buf, frame_size);
	who.CloseSocket();
}

int WebSocketServer::SendFrame(const Client &client, const uint8_t *buffer, size_t buffer_size)
{
    int client_socket = client.get_socket();
    ssize_t written = send(client_socket, buffer, buffer_size, 0);
    if (written == -1)
    {
        Log("send failed\n");
        return -1;
    }
    if (written != buffer_size) 
    {
        Log("written not all bytes(%i of %i)\n", written, buffer_size);

		int written_total = written;
		while (written_total < buffer_size && written >= 0)
		{
			written = send(client_socket, buffer + written_total, buffer_size - written_total, 0);
			written_total += written;
			Log("written %i bytes more (%i of %i total)\n", written, written_total, buffer_size);
		}
		if (written < 0)
		{
				return -1;
		}
    }
    
    return 0;
}

int WebSocketServer::RecieveData(const Client &client, char *buf, int len)
{
}

int WebSocketServer::Send(const Client &recepient, char *message, int len)
{
	unsigned char frame_buf[BUF_LEN];
	size_t frame_len = BUF_LEN;

	wsMakeFrame((unsigned char*)message, len, frame_buf, &frame_len, WS_TEXT_FRAME);
	return SendFrame(recepient, frame_buf, frame_len);
}*/

void *WebSocketServer::ListenMessages(void *arg)
{
	WebSocketServer *me = (WebSocketServer*)arg;
	char data_buf[BUF_LEN];
	int ret;
	int data_len;
	fd_set fds;
	int nfds;
	
	while (!(me->stop))
	{
		//Log("%i clients\n", me->clients.size());
		me->WaitUntilListIsNotEmpty();
		//Log("Filling set\n");
		FD_ZERO(&fds);
		for (auto it = me->clients.begin(); it != me->clients.end(); it++)
		{
			FD_SET(it->get_socket(), &fds);
		}
		nfds = me->clients.rbegin()->get_socket() + 1;
		me->ReleaseClientsList();
		
		select(nfds, &fds, NULL, NULL, NULL);
		
		me->LockClientsList();
		for (auto it = me->clients.begin(); it != me->clients.end();)
		{
			if (FD_ISSET(it->get_socket(), &fds))
			{
				//Log("recv...\n");
				//That looks strange, but that's how sets work,
				//you can't modify element while it's in the set.
				//It is way better to use map here since we
				//don't really ever change the socket field of Client
				//and huge send-recv buffers are being copied 
				//every time this shit happens.
				//TODO: fix.
				Client copy = *it;
				me->clients.erase(it++);
				Log("Next client\n");
				do{
					data_len = BUF_LEN;
					ret = copy.Receive(data_buf, &data_len);
					Log("Receive returned %i\n", ret);
					if (ret == READ_SUCCEED || ret == READ_SUCCEED_DATA_AVAILABLE)
					{
						me->OnMessage(copy, data_buf, data_len);
						//me->clients.insert(copy);
					}
					else if (ret < 0)
					{
						//ret < 0 in this branch
						copy.Disconnect();
					}
				}while (ret == READ_SUCCEED_DATA_AVAILABLE || ret == HANDSHAKE_SUCCEED);
				if (ret >= 0)
				{
					me->clients.insert(copy);
				}
			}
		}
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
	/*if (pthread_create(&message_listener, NULL, WebSocketServer::ListenMessages, this) < 0)
	{
		last_error = errno;
		stop = true;
		pthread_join(connection_listener, NULL);
		connection_listener = -1;
		message_listener = -1;
		return -1;
	}*/
	printf("starting to listen messages\n");
	WebSocketServer::ListenMessages(this);
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
