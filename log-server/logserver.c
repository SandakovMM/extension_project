#include <stdlib.h>

#include <vector>

#define BUF_LEN 1024

struct connection_listener_args{
	int port;
	char address[16];
};

struct log_monitor_args{
	int socket;
};

struct client{
	int socket;
	sockaddr_in saddr;
	int saddr_len;
};

vector<client> clients;

void clear_client(client &cl)
{
	cl.saddr_len = sizeof(cl.saddr);
	cl.socket = -1;
}

int safeSend(int clientSocket, const uint8_t *buffer, size_t bufferSize)
{
    ssize_t written = send(clientSocket, buffer, bufferSize, 0);
    if (written == -1) {
        close(clientSocket);
        perror("send failed");
        return EXIT_FAILURE;
    }
    if (written != bufferSize) {
        close(clientSocket);
        perror("written not all bytes");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

int listen_socket(char address[16], int port, int queue_size)
{
	int sock;
	sockaddr_in saddr;

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	inet_aton(address, &saddr.s_addr);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(sock, (struct sockaddr*)saddr, sizeof(saddr)) == -1)
		return -1;
	if (listen(sock, queue_size) == -1)
		return -1;
	return sock;
}

int do_handshake(int socket)
{
	unsigned char frame_buf[BUF_LEN];
	unsigned char data_buf[BUF_LEN];
	size_t readedLength = 0;
	int frameSize;
	enum wsFrameType frameType = WS_INCOMPLETE_FRAME;
    	struct handshake hs;
	nullHandshake(&hs);

	while (frameType == WS_INCOMPLETE_FRAME)
	{
		if (readedLength >= BUF_LEN)
		{
			printf("Buffer is too small\n");
			return -1;
		}
	        ssize_t readed = recv(socket, frame_buf+readedLength, BUF_LEN-readedLength, 0);
	        if (!readed)
		{
       			close(clientSocket);
	        	//perror("recv failed");
			return -1;
		}
        	readedLength+= readed;
		assert(readedLength <= BUF_LEN);
		frameType = wsParseHandshake(frame_buf, readedLength, &hs);

		 if (frameType == WS_OPENING_FRAME)
		 {
			frameSize = BUF_LEN;
			memset(frame_buf, 0, BUF_LEN);
			wsGetHandshakeAnswer(&hs, gBuffer, &frameSize);
			freeHandshake(&hs);
			if (safeSend(clientSocket, gBuffer, frameSize) == EXIT_FAILURE)
				return -1;
		 }
        }
	return 0;
}

void *connection_listener(void *arg)
{
	vector<client> pending_clients;
	connection_listener_args *param = *arg;
	int listener_socket;

	listener_socket = listen_socket(param->address, param->port, 10);
	if (listener_socket == -1)
	{
		printf("Could not bind/listen\n");
		return;
	}

	client new_client;

	while (true)
	{
		clear_client(new_client);

		new_client.socket = accept(listener_socket, &(new_client->saddr), &(new_client->saddr_len));

		/*if (new_client.socket != -1)
		{
			pending_clients.push_back(new_client);
		}*/
	}
}

void *log_monitor(void *arg)
{
	unsigned char frame_buf[BUF_LEN];
	unsigned char data_buf[BUF_LEN];

	int socket = *((int*)arg);
	if (do_handshake(socket) < 0)
		return NULL;

	while (true)
	{
        ssize_t readed = recv(clientSocket, gBuffer+readedLength, BUF_LEN-readedLength, 0);
        if (!readed) {
            close(clientSocket);
            perror("recv failed");
            return;
        }

	}
}
