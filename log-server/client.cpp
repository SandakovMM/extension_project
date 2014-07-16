#include "client.h"
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

Client::Client()
{
	socket = -1;
}

Client::Client(int listener)
{
	Accept(listener);
}

int Client::Accept(int listener)
{
	socket = accept(listener, &address, &address_len);
	if (socket <= 0)
	{
		last_error = errno;
		return -1;
	}
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
