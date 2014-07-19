#include "web_socket_server.h"

class EchoServer : public WebSocketServer
{
public:
	EchoServer(int p, const char *a, int q)
	: WebSocketServer(p, a, q)
	{
	}

	void OnMessage(Client &sender, char *message, int len)
	{
		if (len == 0)
		{
			printf("Received empty message. Sending back.\n");
			sender.Send(NULL, 0);
			return;
		}
		printf("Received: %s\nSending back.\n", message);
		if (sender.Send(message, len) < 0)
		{
			printf("Error while sending\n");
		}
	}
};

int main(int argc, char *argv[])
{
	if (argc < 2)
		return -1;
	EchoServer s(atoi(argv[2]), argv[1], 10);
	s.Run();
	return 0;
}
