#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <vector>

#include <websocket.h>

using namespace std;

#define BUF_LEN 1024
#define LOG_BUF_LEN 2048

struct connection_listener_args{
	int port;
	char address[16];
};

struct log_monitor_args{
	int socket;
};

struct client{
	int socket;
	sockaddr saddr;
	unsigned int saddr_len;
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
        printf("send failed\n");
        return -1;
    }
    if (written != bufferSize) {
        close(clientSocket);
        printf("written not all bytes\n");
        return -1;
    }
    
    return 0;
}

int listen_socket(char address[16], int port, int queue_size)
{
	int sock;
	sockaddr_in saddr;

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	inet_aton(address, &(saddr.sin_addr));

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(sock, (struct sockaddr*)(&saddr), sizeof(saddr)) == -1)
		return -1;
	if (listen(sock, queue_size) == -1)
		return -1;
	return sock;
}

#define prepareBuffer frameSize = BUF_LEN; memset(frame_buf, 0, BUF_LEN);
int do_handshake(int socket)
{
	unsigned char frame_buf[BUF_LEN];
	unsigned char data_buf[BUF_LEN];
	size_t readedLength = 0;
	unsigned int frameSize;
	enum wsFrameType frameType = WS_INCOMPLETE_FRAME;
    	struct handshake hs;
	nullHandshake(&hs);

	while (frameType == WS_INCOMPLETE_FRAME)
	{
	        ssize_t readed = recv(socket, frame_buf+readedLength, BUF_LEN-readedLength, 0);
	        if (!readed)
		{
       			close(socket);
			return -1;
		}
        	readedLength+= readed;
		assert(readedLength <= BUF_LEN);
		frameType = wsParseHandshake(frame_buf, readedLength, &hs);

		if ((frameType == WS_INCOMPLETE_FRAME && readedLength >= BUF_LEN) || frameType == WS_ERROR_FRAME)
		{
			printf("Buffer is too small or invalid frame\n");
                	prepareBuffer;
	                frameSize = sprintf((char *)frame_buf,
                                    "HTTP/1.1 400 Bad Request\r\n"
                                    "%s%s\r\n\r\n",
                                    versionField,
                                    version);
        	        safeSend(socket, frame_buf, frameSize);
			return -1;
		}
        }
	if (frameType == WS_OPENING_FRAME)
	{
		frameSize = BUF_LEN;
		memset(frame_buf, 0, BUF_LEN);
		wsGetHandshakeAnswer(&hs, frame_buf, &frameSize);
		freeHandshake(&hs);
		if (safeSend(socket, frame_buf, frameSize) == -1)
			return -1;
		return 0;
	}
	return -1;
}

int read_frame(int socket, unsigned char *buf, int len)
{
	unsigned char frame_buf[BUF_LEN];
	size_t readedLength = 0;
	unsigned int frameSize;
	unsigned int dataSize;
	enum wsFrameType frameType = WS_INCOMPLETE_FRAME;
	unsigned char *data;

	while (frameType == WS_INCOMPLETE_FRAME)
	{
	        ssize_t readed = recv(socket, frame_buf+readedLength, BUF_LEN-readedLength, 0);
	        if (readed <= 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				printf("socket eagain\n");
				return 0;
			}
			else
			{
				printf("Error while receiving.\n");
				return -1;
			}
		}
        	readedLength+= readed;
		assert(readedLength <= BUF_LEN);
		//frameType = wsParseHandshake(frame_buf, readedLength, &hs);
            	frameType = wsParseInputFrame(frame_buf, readedLength, &data, &dataSize);

		if ((frameType == WS_INCOMPLETE_FRAME && readedLength >= BUF_LEN) || frameType == WS_ERROR_FRAME)
		{
			printf("Buffer is too small or invalid frame\n");
                	prepareBuffer;
	                wsMakeFrame(NULL, 0, frame_buf, &frameSize, WS_CLOSING_FRAME);
        	        safeSend(socket, frame_buf, frameSize);
			return -5;
		}

        }
	if (frameType == WS_TEXT_FRAME)
	{
		if (dataSize >= len)
			return -2;
		memcpy(buf, data, dataSize);
		buf[dataSize] = 0;
		return dataSize;
	}
	if (frameType == WS_CLOSING_FRAME)
	{
		return -4;
	}
	return -3;
}

int openLog(char *fileName, int *retFd, pid_t *retPid)
{
	int tailPipe[2];
	if (pipe(tailPipe) == -1)
	{
		return -1;
	}

	pid_t pid = fork();
	if (pid < 0)
		return -2;
	else if (pid > 0)
	{
		close(tailPipe[1]);
		*retFd = tailPipe[0];
		*retPid = pid;
		return 0;
	}

	char *args[] = {"tail", "-F", fileName, NULL};
	close(tailPipe[0]);
	dup2(tailPipe[1], 1);
	execvp("tail", args);
	return -3;
}

void *log_monitor(void *arg)
{
	unsigned char frameBuf[BUF_LEN];
	unsigned char dataBuf[BUF_LEN];
	char logBuf[LOG_BUF_LEN];

	int socket = *((int*)arg);

	if (do_handshake(socket) < 0)
	{
		printf("Handshake error. Closing connection.\n");
		close(socket);
		return NULL;
	}

	int ret;
	int logfd;
	pid_t pid;
	printf("forked\n");
	if (openLog("/home/goon/test.txt", &logfd, &pid) < 0)
	{
		printf("Error while opening log file (%i)\n", logfd);
		close(socket);
		return NULL;
	}

	if (fcntl(socket, F_SETFL, O_NONBLOCK) == -1 ||
		fcntl(logfd, F_SETFL, O_NONBLOCK) == -1)
	{
		printf("ALARM\n");
		close(socket);//TODO: proper websocket connectin closing
		close(logfd);
		return NULL;
	}

	FILE *logStream = fdopen(logfd, "r");
	if (!logStream)
	{
		printf("null stream\n");
		return NULL;
	}

	fd_set fds;
	int nfds = socket > logfd ? socket : logfd;
	while (true)
	{
		FD_ZERO(&fds);
		FD_SET(logfd, &fds);
		FD_SET(socket, &fds);
		printf("selecting\n");
		select(nfds, &fds, NULL, NULL, NULL);

		if(FD_ISSET(socket, &fds))
		{
			printf("Trying to recv\n");
			ret = read_frame(socket, dataBuf, BUF_LEN);
			if (ret > 0)
				printf("Received: %s\n", dataBuf);
			else if (ret == -4)
			{
				printf("User disconnected\n");
				close(socket);
				break;
			}
			else
			{
				printf("Error while reading frame (%i). Closing connection.\n", ret);
				close(socket);
				break;
			}
		}
	
		if (FD_ISSET(logfd, &fds))
		{
			printf("Trying to read log\n");
			if (fgets(logBuf, LOG_BUF_LEN, logStream) == NULL)
			{
				printf("Failed to read from tail, killing fork\n");
				fclose(logStream);
				waitpid(pid, NULL, 0);
				printf("Fork exited\n");
				break;
			}
			printf("%s", logBuf);
		}
	}

}

void *connection_listener(void *arg)
{
	vector<client> pending_clients;
	connection_listener_args *param = (connection_listener_args*)arg;
	int listener_socket;

	listener_socket = listen_socket(param->address, param->port, 10);
	if (listener_socket == -1)
	{
		printf("Could not bind/listen\n");
		return NULL;
	}

	printf("Bound to %s:%i\n", param->address, param->port);

	client new_client;

	while (true)
	{
		clear_client(new_client);

		printf("ready to accept\n");

		new_client.socket = accept(listener_socket, &(new_client.saddr), &(new_client.saddr_len));
		if (new_client.socket > 0)
		{
			clients.push_back(new_client);
	
			pthread_t thread;
			pthread_create(&thread, NULL, log_monitor, &(clients.at(clients.size() - 1).socket));

			printf("Connection accepted\n");
		}
		else
			printf("Connection failed\n");

		/*if (new_client.socket != -1)
		{
			pending_clients.push_back(new_client);
		}*/
	}
}


int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("log-server <ip> <port>\n");
		return -1;
	}
	connection_listener_args arg;
	arg.port = atoi(argv[2]);
	memcpy(arg.address, argv[1], 12);
	pthread_t thread;
	pthread_create(&thread, NULL, connection_listener, &(arg));
	pthread_join(thread, NULL);
}
