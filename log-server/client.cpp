#include "client.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <openssl/err.h>

#include <websocket.h>


#ifdef DEBUGLOGGING
	#define Log(format, ...) printf(format, ## __VA_ARGS__)
#else
	#define Log(format, ...)
#endif

Client::Client()
{
	id = -1;
	socket = -1;
	frame_buf_pos = 0;
	handshaked = false;
}

Client::Client(int listener, SSL_CTX *tls_ssl_context)
{
	if (Accept(listener, tls_ssl_context) < 0)
	{
		socket = -1;
		tls_connection = NULL;
	}
}

int Client::Accept(int listener, SSL_CTX *tls_ssl_context)
{
	address_len = sizeof(address);
	socket = accept(listener, &address, &address_len);
	if (socket <= 0)
	{
		last_error = errno;
		return -1;
	}

	/*tls_ssl_context = SSL_CTX_new(SSLv3_server_method());
	if (tls_ssl_context == NULL)
	{
		return -2;
	}*/
	tls_connection = SSL_new(tls_ssl_context);
	if (tls_connection == NULL)
	{
		return -3;
	}
	if (SSL_set_fd(tls_connection, socket) == 0)
	{
		return -4;
	}

	/*int ret, err = SSL_ERROR_WANT_READ, nfds = socket + 1;
	fd_set write_fd, read_fd;
	do
	{
		FD_ZERO(&write_fd);
		FD_ZERO(&read_fd);
		FD_SET(socket, &write_fd);
		FD_SET(socket, &read_fd);
		select(nfds, &read_fd, &write_fd, NULL, NULL);

		if (err == SSL_ERROR_WANT_WRITE && !FD_ISSET(socket, &write_fd))
		{
			continue;
		}
		if (err == SSL_ERROR_WANT_READ && !FD_ISSET(socket, &read_fd))
		{
			continue;
		}

		ret = SSL_accept(tls_connection);
		err = SSL_get_error(tls_connection, ret);
	}while(err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);*/
	if (SSL_accept(tls_connection) <= 0)
	{
		Log("Failed to accept ssl connection\n");
		//ERR_print_errors_fp (stdout);
		return -5;
	}
	return 0;
}

void Client::set_id(int new_id)
{
	id = new_id;
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
	int ret;
	ret = SSL_shutdown(tls_connection);
	if (ret == 0)
	{
		ret = SSL_shutdown(tls_connection);
	}
	if (socket > 0)
	{
		close(socket);
		socket = -1;
	}
}

int Client::Receive(char *buf, int *len)
{
	int ret;
	if (!handshaked)
	{
		Log("Handshaking\n");
		//if we didn't do a hanshake yet, start/continue
		//doing it. If failed, return error, if succeed,
		//return flag indicatin that more data is pending
		//(in case it really is)
		ret = Handshake();
		if (ret < 0)
		{
			Log("Handshake failed with code %i\n", ret);
			return HANDSHAKE_FAILED;
		}
		else
		{
			if (handshaked)
			{
				Log("handshaked\n");
				return HANDSHAKE_SUCCEED;
			}
			else
			{
				Log("waiting for more data to hanshake\n");
				return WAITING_FOR_MORE_DATA;
			}
		}
	}
	//Log("Receiving\n");
	//unsigned char frame_buf[BUF_LEN];
	//size_t read_total_len = 0;
	size_t frame_size;
	size_t data_size;
	enum wsFrameType frame_type = WS_INCOMPLETE_FRAME;
	unsigned char *data;

	//int read_len = recv(socket, frame_buf + frame_buf_pos, BUF_LEN - frame_buf_pos, 0);
	ret = SSL_read(tls_connection, frame_buf + frame_buf_pos, BUF_LEN - frame_buf_pos);
	int err = SSL_get_error(tls_connection, ret);
	//Log("err: %i\n", err);
	if (ret < 0)
	{
		//if frame_buf_pos is not 0, we are trying to read a frame from
		//what's left in the buffer after previous call and thus we
		//should not return here
		if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_READ)
		{
			if (frame_buf_pos == 0)
			{
				Log("Returning %i\n", WAITING_FOR_MORE_DATA);
				return WAITING_FOR_MORE_DATA;
			}
		}
		else
		{
			return NETWORK_ERROR;
		}
	}
	else if (ret == 0)
	{
		return CLIENT_DISCONNECTED;
	}
	else
	{
		//change frame_buf_pos only if we actually received something
		frame_buf_pos += ret;
	}

	/*Log("Frame buffer: ");
	for (int i = 0; i < frame_buf_pos; i++)
		Log("%x ", frame_buf[i]);
	Log("\n(frame_buf_pos: %i)\n", frame_buf_pos);*/

	frame_type = wsParseInputFrame(frame_buf, frame_buf_pos, &data, &data_size, &frame_size);
	if ((frame_type == WS_INCOMPLETE_FRAME && frame_buf_pos >= BUF_LEN) || frame_type == WS_ERROR_FRAME)
	{
		//if frame is not complete and we already hit buffer size limit
		//then we can't receive this frame.
		//also reject frame if it is malformed
		return INVALID_FRAME;
	}

	if (frame_type == WS_INCOMPLETE_FRAME)
	{
		return WAITING_FOR_MORE_DATA;
	}
	if (frame_type == WS_TEXT_FRAME)
	{
		if (data_size >= *len)
		{
			frame_buf_pos -= frame_size;
			memcpy(frame_buf, frame_buf + frame_size, frame_buf_pos);
			Log("frame_buf_pos: %i, frame_size: %i\n", frame_buf_pos, frame_size);
			return BUFFER_TOO_SMALL;
		}

		memcpy(buf, data, data_size);
		buf[data_size] = 0;

		assert(frame_size <= frame_buf_pos);
		frame_buf_pos -= frame_size;
		memcpy(frame_buf, frame_buf + frame_size, frame_buf_pos);
		Log("frame_buf_pos: %i, frame_size: %i\n", frame_buf_pos, frame_size);

		*len = data_size;
		if (frame_buf_pos == 0)
		{
			return READ_SUCCEED;
		}
		else
		{
			return READ_SUCCEED_DATA_AVAILABLE;
		}
	}
	if (frame_type == WS_CLOSING_FRAME)
	{
		return CLIENT_DISCONNECTED;
	}
	return INVALID_FRAME;
}

int Client::Handshake()
{
	size_t frame_size;
	enum wsFrameType frame_type = WS_INCOMPLETE_FRAME;
	struct handshake hs;
	nullHandshake(&hs);

	//int readed = recv(socket, frame_buf + frame_buf_pos, BUF_LEN - frame_buf_pos, 0);
	int ret = SSL_read(tls_connection, frame_buf + frame_buf_pos, BUF_LEN - frame_buf_pos);
	int err = SSL_get_error(tls_connection, ret);
	if (ret < 0)
	{
		if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_READ)
		{
			return WAITING_FOR_MORE_DATA;
		}
		else
		{
			Log("Recieve error (handshaking): %i\n", err);
			return NETWORK_ERROR;
		}
	}
	else if (ret == 0)
	{
		Log("ssl_read returned 0\n");
		//ERR_print_errors_fp (stdout);
		return CLIENT_DISCONNECTED;
	}

       	frame_buf_pos += ret;
	frame_buf[frame_buf_pos] = 0;

	/*Log("Handshaking. Buffer length: %i.\nBuffer content: ", frame_buf_pos);
	for (int i = 0; i < frame_buf_pos; i++)
		Log("%x ", frame_buf[i]);
	Log("\n");*/

	frame_type = wsParseHandshake(frame_buf, frame_buf_pos, &hs, &frame_size);
	if ((frame_type == WS_INCOMPLETE_FRAME && frame_buf_pos >= BUF_LEN) || frame_type == WS_ERROR_FRAME)
	{
		size_t send_size = BUF_LEN;
		memset(send_buf, 0, BUF_LEN);
		send_size = sprintf((char *)send_buf,
						"HTTP/1.1 400 Bad Request\r\n"
						"%s%s\r\n\r\n",
						versionField,
						version);
		SendFrame(send_buf, send_size);
		return INVALID_FRAME;
	}
        
	if (frame_type == WS_INCOMPLETE_FRAME)
	{
		return WAITING_FOR_MORE_DATA;
	}
	if (frame_type == WS_OPENING_FRAME)
	{
		size_t send_size = BUF_LEN;
		memset(send_buf, 0, BUF_LEN);
		wsGetHandshakeAnswer(&hs, send_buf, &send_size);
		freeHandshake(&hs);

		assert(frame_size <= frame_buf_pos);
		frame_buf_pos -= frame_size;
		memcpy(frame_buf, frame_buf + frame_size, frame_buf_pos);

		if (SendFrame(send_buf, send_size) == -1)
			return NETWORK_ERROR;
		handshaked = true;
		if (frame_buf_pos == 0)
		{
			return READ_SUCCEED;
		}
		else
		{
			return READ_SUCCEED_DATA_AVAILABLE;
		}
	}
	return INVALID_FRAME;
}

int Client::SendFrame(const unsigned char *buffer, size_t buffer_size)
{
    /*ssize_t written = send(socket, buffer, buffer_size, 0);
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
    }*/
	int ret, err = SSL_ERROR_WANT_WRITE, nfds = socket + 1;
	fd_set write_fd, read_fd;
	do
	{
		FD_ZERO(&write_fd);
		FD_ZERO(&read_fd);
		FD_SET(socket, &write_fd);
		FD_SET(socket, &read_fd);
		select(nfds, &read_fd, &write_fd, NULL, NULL);

		if (err == SSL_ERROR_WANT_WRITE && !FD_ISSET(socket, &write_fd))
		{
			continue;
		}
		if (err == SSL_ERROR_WANT_READ && !FD_ISSET(socket, &read_fd))
		{
			continue;
		}

		ret = SSL_write(tls_connection, buffer, buffer_size);
		err = SSL_get_error(tls_connection, ret);
	}while(err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);

	if (ret <= 0)
	{
		return -1;
	}

	return 0;
}

int Client::Send(char *message, int len)
{
	//unsigned char frame_buf[BUF_LEN];
	int frame_len = wsMakeFrame((unsigned char*)message, len, send_buf, SEND_BUF_LEN, WS_TEXT_FRAME);
	if (frame_len < 0)
	{
		return -10;
	}
	else
	{
		return SendFrame(send_buf, frame_len);
	}
}

void Client::Disconnect()
{
	//unsigned char frame_buf[BUF_LEN];
	int frame_size;
	
	//TODO: if connection reset by other side then just close socket
	frame_size = wsMakeFrame(NULL, 0, send_buf, SEND_BUF_LEN, WS_CLOSING_FRAME);
	if (frame_size > 0)
	{
		SendFrame(send_buf, frame_size);
	}
	CloseSocket();
	Log("Disconnecting\n");
}

int Client::get_id()
{
	return id;
}

int Client::get_socket()
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
