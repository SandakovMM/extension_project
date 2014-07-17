#include <sys/socket.h>

class Client
{
	int socket;
	sockaddr address;
	unsigned int address_len;
	int last_error;
public:
	Client();
	Client(int listener);
	int Accept(int listener);
	int SetNonBlocking();
	void CloseSocket();
	int get_socket()const;
	sockaddr get_address();
	int get_address_len();
};
