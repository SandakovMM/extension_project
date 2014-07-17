#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include "web_socket_server.h"

#define BUF_LEN 2048

#define prepareBuffer frame_size = BUF_LEN; memset(frame_buf, 0, BUF_LEN);

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
	unsigned char frame_buf[BUF_LEN];
	size_t read_total_len = 0;
	unsigned int frame_size;
	enum wsFrameType frame_type = WS_INCOMPLETE_FRAME;
	struct handshake hs;
	int socket = client.get_socket();
	nullHandshake(&hs);

	while (frame_type == WS_INCOMPLETE_FRAME)
	{
	        ssize_t readed = recv(socket, frame_buf+read_total_len, BUF_LEN-read_total_len, 0);
	        if (!readed)
		{
       			close(socket);
			return -1;
		}
        	read_total_len+= readed;
		assert(read_total_len <= BUF_LEN);
		frame_type = wsParseHandshake(frame_buf, read_total_len, &hs);

		if ((frame_type == WS_INCOMPLETE_FRAME && read_total_len >= BUF_LEN) || frame_type == WS_ERROR_FRAME)
		{
			//printf("Buffer is too small or invalid frame\n");
			prepareBuffer;
			frame_size = sprintf((char *)frame_buf,
							"HTTP/1.1 400 Bad Request\r\n"
							"%s%s\r\n\r\n",
							versionField,
							version);
			SendFrame(client, frame_buf, frame_size);
			close(socket);					
			return -1;
		}
        }
	if (frame_type == WS_OPENING_FRAME)
	{
		frame_size = BUF_LEN;
		memset(frame_buf, 0, BUF_LEN);
		wsGetHandshakeAnswer(&hs, frame_buf, &frame_size);
		freeHandshake(&hs);
		if (SendFrame(client, frame_buf, frame_size) == -1)
			return -1;
		return 0;
	}
	return -1;
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

void *WebSocketServer::ListenConnections(void *arg)
{
	WebSocketServer *me = (WebSocketServer*)arg;
	int ret;
	
	ret = me->ListenSocket();
	if (ret < 0)
	{
		return NULL;
	}
	
	Client new_client;
	while (!(me->stop))
	{
		ret = new_client.Accept(me->server_socket);
		if (ret == 0)
		{
			ret = me->DoHandshake(new_client);
			if (ret > 0)
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
		}
	}
	
	me->LockClientsList();
	for (auto it = me->clients.begin(); it != me->clients.end(); it++)
	{
		DisconnectClient(*it);
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

void WebSocketServer::DisconnectClient(Client who)
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
        printf("send failed\n");
        return -1;
    }
    if (written != buffer_size) 
    {
        printf("written not all bytes(%i of %i)\n", written, buffer_size);

		int written_total = written;
		while (written_total < buffer_size && written >= 0)
		{
			written = send(client_socket, buffer + written_total, buffer_size - written_total, 0);
			written_total += written;
			printf("written %i bytes more (%i of %i total)\n", written, written_total, buffer_size);
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
	unsigned char frame_buf[BUF_LEN];
	size_t read_total_len = 0;
	unsigned int frame_size;
	unsigned int data_size;
	enum wsFrameType frame_type = WS_INCOMPLETE_FRAME;
	unsigned char *data;
	int socket = client.get_socket();

	while (frame_type == WS_INCOMPLETE_FRAME)
	{
		size_t read_len = recv(socket, frame_buf + read_total_len, BUF_LEN - read_total_len, 0);
		if (read_len <= 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				return 0;
			}
			else
			{
				return -1;
			}
		}
		read_total_len += read_len;
		assert(read_total_len <= BUF_LEN);
		frame_type = wsParseInputFrame(frame_buf, read_total_len, &data, &data_size);

		if ((frame_type == WS_INCOMPLETE_FRAME && read_total_len >= BUF_LEN) || frame_type == WS_ERROR_FRAME)
		{
			return -5;
		}

	}
	if (frame_type == WS_TEXT_FRAME)
	{
		if (data_size >= len)
			return -2;
		memcpy(buf, data, data_size);
		buf[data_size] = 0;
		return data_size;
	}
	if (frame_type == WS_CLOSING_FRAME)
	{
		return -4;
	}
	return -3;
}

int WebSocketServer::Send(const Client &recepient, char *message, int len)
{
	unsigned char frame_buf[BUF_LEN];
	size_t frame_len = BUF_LEN;

	wsMakeFrame((unsigned char*)message, len, frame_buf, &frame_len, WS_TEXT_FRAME);
	return SendFrame(recepient, frame_buf, frame_len);
}

void *WebSocketServer::ListenMessages(void *arg)
{
	WebSocketServer *me = (WebSocketServer*)arg;
	char data_buf[BUF_LEN];
	int ret;
	fd_set fds;
	int nfds;
	
	while (!(me->stop))
	{
		me->WaitUntilListIsNotEmpty();
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
				ret = RecieveData(*it, data_buf, BUF_LEN);
				if (ret > 0)
				{
					me->OnMessage(*it, data_buf, ret);
					it++;
				}
				else if (ret < 0)
				{
					DisconnectClient(*it);
					me->clients.erase(it++);
				}
			}
		}
		me->ReleaseClientsList();
	}
	me->ReleaseClientsList();
	
	return NULL;
}

int WebSocketServer::Run()
{
	stop = false;
	if (pthread_create(&connection_listener, NULL, WebSocketServer::ListenConnections, this) < 0)
	{
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
