#include "client.h"
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <websocket.h>


#ifdef DEBUGLOGGING
	#define Log(format, ...) printf(format, ## __VA_ARGS__)
#else
	#define Log(format, ...)
#endif

Client::Client()
{
	socket = -1;
	frame_buf_pos = 0;
	handshaked = false;
}

Client::Client(int listener)
{
	Accept(listener);
}

int Client::Accept(int listener)
{
	address_len = sizeof(address);
	socket = accept(listener, &address, &address_len);
	if (socket <= 0)
	{
		last_error = errno;
		return -1;
	}
	return 0;
}

int Client::SetNonBlocking()
{
	if (fcntl(socket, F_SETFL, O_NONBLOCK) < 0)
	{
		last_error = errno;
		CloseSocket();
		return -1;
	}
}

void Client::CloseSocket()
{
	if (socket > 0)
	{
		close(socket);
		socket = -1;
	}
}

int Client::Receive(char *buf, int len)
{
	if (!handshaked)
	{
		Log("Handshaking\n");
		//if we didn't do a hanshake yet, start/continue
		//doing it. If failed, return error, if succeed,
		//return 0 (like we got an empty message)
		if (Handshake() < 0)
		{
			return -6;
		}
		else
		{
			if (handshaked)
				Log("handshaked\n");
			else
				Log("waiting for more data to hanshake\n");
			return -7;
		}
	}
	//Log("Receiving\n");
	//unsigned char frame_buf[BUF_LEN];
	//size_t read_total_len = 0;
	unsigned int frame_size;
	unsigned int data_size;
	enum wsFrameType frame_type = WS_INCOMPLETE_FRAME;
	unsigned char *data;
	//int socket = client.get_socket();

	//while (frame_type == WS_INCOMPLETE_FRAME)
	size_t read_len = recv(socket, frame_buf + frame_buf_pos, BUF_LEN - frame_buf_pos, 0);
	if (read_len <= 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			return -8;
		}
		else
		{
			return -1;
		}
	}
	frame_buf_pos += read_len;
	//Log("Read total: %i bytes\n", frame_buf_pos);

	//assert(read_total_len <= BUF_LEN);
	frame_type = wsParseInputFrame(frame_buf, frame_buf_pos, &data, &data_size, &frame_size);
	if ((frame_type == WS_INCOMPLETE_FRAME && frame_buf_pos >= BUF_LEN) || frame_type == WS_ERROR_FRAME)
	{
		//if frame is not complete and we already hit buffer size limit
		//then we can't receive this frame.
		//also reject frame if it is malformed
		return -5;
	}

	if (frame_type == WS_INCOMPLETE_FRAME)
	{
		return -8;
	}
	if (frame_type == WS_TEXT_FRAME)
	{
		frame_buf_pos -= frame_size;
		Log("frame_buf_pos: %i, frame_size: %i\n", frame_buf_pos, frame_size);
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

int Client::Handshake()
{
	//unsigned char frame_buf[BUF_LEN];
	//size_t read_total_len = 0;
	unsigned int frame_size;
	enum wsFrameType frame_type = WS_INCOMPLETE_FRAME;
	struct handshake hs;
	//int socket = client.get_socket();
	nullHandshake(&hs);

	//while (frame_type == WS_INCOMPLETE_FRAME)
	{
	        ssize_t readed = recv(socket, frame_buf + frame_buf_pos, BUF_LEN - frame_buf_pos, 0);
	        if (!readed)
		{
       			//close(socket);
			CloseSocket();
			return -1;
		}
        	frame_buf_pos += readed;
		frame_buf[frame_buf_pos] = 0;
		Log("hs: %s\n", frame_buf);
		//assert(read_total_len <= BUF_LEN);
		frame_type = wsParseHandshake(frame_buf, frame_buf_pos, &hs, &frame_size);

		if ((frame_type == WS_INCOMPLETE_FRAME && frame_buf_pos >= BUF_LEN) || frame_type == WS_ERROR_FRAME)
		{
			//printf("Buffer is too small or invalid frame\n");
			//prepareBuffer;
			size_t send_size = BUF_LEN;
			memset(send_buf, 0, BUF_LEN);
			send_size = sprintf((char *)send_buf,
							"HTTP/1.1 400 Bad Request\r\n"
							"%s%s\r\n\r\n",
							versionField,
							version);
			SendFrame(send_buf, send_size);
			//close(socket);
			CloseSocket();
			return -1;
		}
        }
	if (frame_type == WS_INCOMPLETE_FRAME)
	{
		return 0;
	}
	if (frame_type == WS_OPENING_FRAME)
	{
		frame_buf_pos -= frame_size;
		Log("Handshake done: pos is %i\n", frame_buf_pos);
		//unsigned char send_buf[BUF_LEN];
		size_t send_size = BUF_LEN;
		memset(send_buf, 0, BUF_LEN);
		wsGetHandshakeAnswer(&hs, send_buf, &send_size);
		freeHandshake(&hs);
		if (SendFrame(send_buf, send_size) == -1)
			return -1;
		handshaked = true;
		return 0;
	}
	return -1;
}

int Client::SendFrame(const unsigned char *buffer, size_t buffer_size)
{
    //int client_socket = client.get_socket();
    ssize_t written = send(socket, buffer, buffer_size, 0);
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
			written = send(socket, buffer + written_total, buffer_size - written_total, 0);
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

int Client::Send(char *message, int len)
{
	//unsigned char frame_buf[BUF_LEN];
	size_t frame_len = BUF_LEN;

	wsMakeFrame((unsigned char*)message, len, send_buf, &frame_len, WS_TEXT_FRAME);
	return SendFrame(send_buf, frame_len);
}

void Client::Disconnect()
{
	//unsigned char frame_buf[BUF_LEN];
	unsigned int frame_size;
	
	//TODO: if connection reset by other side then just close socket
	wsMakeFrame(NULL, 0, send_buf, &frame_size, WS_CLOSING_FRAME);
	SendFrame(send_buf, frame_size);
	CloseSocket();
	Log("Disconnecting\n");
}

int Client::get_socket()const
{
	return socket;
}
sockaddr Client::get_address()
{
	return address;
}
int Client::get_address_len()
{
	return address_len;
}

int Client::get_last_error()
{
	return last_error;
}
