#include <pthread.h>

#include <map>
#include <string>

#include <websocket.h>

#include "client.h"

/*bool compare_clients(Client c1, Client c2)
{
	return c1.get_socket() < c2.get_socket();
}

bool(*a)(Client, Client) = compare_clients;*/

/*struct ClientComparator
{
	bool operator()(const Client &c1, const Client &c2) const
	{
		return c1.get_socket() < c2.get_socket();
	}
};*/

class WebSocketServer
{
	int port;
	char address[16];
	int max_queue_len;
	int server_socket;
	int id_counter;
	int last_error;
	pthread_mutex_t clients_mutex;
	pthread_cond_t clients_not_empty_cond;
	pthread_t connection_listener;
	pthread_t message_listener;
	std::string certificate_file;
	std::string private_key_file;
	SSL_CTX *tls_ssl_context;
protected:
	bool stop;
	std::map<int,Client> clients;
	
protected:
	static int SendFrame(const Client &client, const uint8_t *buffer, size_t buffer_size);
	int DoHandshake(Client &client);
	static void *ListenConnections(void *arg);
	static void *ListenMessages(void *arg);
	int LockClientsList();
	int ReleaseClientsList();
	int InitSSL();
	void WaitUntilListIsNotEmpty();
	void TellThatListIsNotEmpty();
	int ListenSocket();
	static int RecieveData(const Client &client, char* buf, int len);
	int GenerateID();
public:
	WebSocketServer(int port_, int max_queue_len_, char *cert, char *pkey);
	int Send(const Client &recepient, char *message, int len);
	static void DisconnectClient(Client who);
	virtual int Run();
	virtual void Stop();
	virtual void OnMessage(Client &sender, char *message, int len)=0;
	virtual void OnDisconnect(Client &disconnected)=0;
	Client *get_client(int id);
};
