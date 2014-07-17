#include "web_socket_server.h"

class EchoServer : public WebSocketServer
{
public:
	EchoServer(int p, const char *a, int q)
	: WebSocketServer(p, a, q)
	{
	}

	void OnMessage(const Client &sender, char *message, int len)
	{
		if (Send(sender, message, len) < 0)
		{
			printf("Error while sending\n");
		}
	}
};

int main()
{
	EchoServer s(21054, "192.168.0.7", 10);
	s.Run();
	return 0;
}
