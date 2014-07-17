#include <pthread.h>

#include <set>

#include <websocket.h>

#include "client.h"

/*bool compare_clients(Client c1, Client c2)
{
	return c1.get_socket() < c2.get_socket();
}

bool(*a)(Client, Client) = compare_clients;*/

struct ClientComparator
{
	bool operator()(const Client &c1, const Client &c2) const
	{
		return c1.get_socket() < c2.get_socket();
	}
};

class WebSocketServer
{
	int port;
	char address[16];
	int max_queue_len;
	int server_socket;
	int last_error;
	bool stop;
	pthread_mutex_t clients_mutex;
	pthread_cond_t clients_not_empty_cond;
	pthread_t connection_listener;
	pthread_t message_listener;
	std::set<Client,ClientComparator> clients;
	
private:
	static int SendFrame(const Client &client, const uint8_t *buffer, size_t buffer_size);
	int DoHandshake(Client &client);
	static void *ListenConnections(void *arg);
	static void *ListenMessages(void *arg);
	int LockClientsList();
	int ReleaseClientsList();
	void WaitUntilListIsNotEmpty();
	void TellThatListIsNotEmpty();
	int ListenSocket();
	static int RecieveData(const Client &client, char* buf, int len);
public:
	WebSocketServer(int port_, const char *address_, int max_queue_len_);
	int Send(const Client &recepient, char *message, int len);
	static void DisconnectClient(Client who);
	int Run();
	void Stop();
	virtual void OnMessage(Client &sender, char *message, int len)=0;
};
