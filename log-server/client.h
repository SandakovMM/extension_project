#include <sys/socket.h>

#include <openssl/ssl.h>

#define BUF_LEN 2048

#define READ_SUCCEED 0
#define READ_SUCCEED_DATA_AVAILABLE 1
#define WAITING_FOR_MORE_DATA 2
#define HANDSHAKE_SUCCEED 3

#define NETWORK_ERROR -1
#define HANDSHAKE_FAILED -6
#define INVALID_FRAME -5
#define CLIENT_DISCONNECTED -4
#define BUFFER_TOO_SMALL -3

class Client
{
	int id;
	int socket;
	sockaddr address;
	unsigned int address_len;
	int last_error;
	unsigned char frame_buf[BUF_LEN + 1];
	unsigned char send_buf[BUF_LEN + 1];
	int frame_buf_pos;
	bool handshaked;
	SSL_CTX *tls_ssl_context;
	SSL *tls_connection;
public:
	Client();
	Client(int listener, SSL_CTX *tls_ssl_context);
	int Accept(int listener, SSL_CTX *tls_ssl_context);
	int SetNonBlocking();
	void CloseSocket();
	int Receive(char *buf, int *len);
	int Handshake();
	int SendFrame(const unsigned char *buffer, size_t buffer_size);
	int Send(char *message, int len);
	void Disconnect();
	void set_id(int new_id);
	int get_id();
	int get_socket();
	sockaddr get_address();
	int get_address_len();
	int get_last_error();
};
