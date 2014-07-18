#include <sys/socket.h>
#define BUF_LEN 2048
class Client
{
	int socket;
	sockaddr address;
	unsigned int address_len;
	int last_error;
	unsigned char frame_buf[BUF_LEN + 1];
	unsigned char send_buf[BUF_LEN + 1];
	int frame_buf_pos;
	bool handshaked;
public:
	Client();
	Client(int listener);
	int Accept(int listener);
	int SetNonBlocking();
	void CloseSocket();
	int Receive(char *buf, int len);
	int Handshake();
	int SendFrame(const unsigned char *buffer, size_t buffer_size);
	int Send(char *message, int len);
	void Disconnect();
	int get_socket()const;
	sockaddr get_address();
	int get_address_len();
	int get_last_error();
};
