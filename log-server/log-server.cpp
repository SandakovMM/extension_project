#include "web_socket_server.h"
#include <list>
#include <string>

#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

#ifdef DEBUGLOGGING
	#define Log(format, ...) printf(format, ## __VA_ARGS__);fflush(stdout)
#else
	#define Log(format, ...)
#endif

const char add_action[] = "add";
const char del_action[] = "del";

struct LogDescription
{
	std::string name;
	int fd;
	pid_t tail_pid;
	int client_id;
	void Cleanup()
	{
		close(fd);
		pid_t ret = waitpid(tail_pid, NULL, WNOHANG);
		if (ret > 0 || ret < 0)
		{
			//if process already terminated or is not a child process
			return;
		}
		else
		{
			kill(tail_pid, 9);
			waitpid(tail_pid, NULL, 0);
		}
	}
	int SetNonBlocking()
	{
		int ret = fcntl(fd, F_SETFL, O_NONBLOCK);
		if (ret < 0)
		{
			Log("fcntl error\n");
			return -1;
		}
		return 0;
	}
};

class LogServer : public WebSocketServer
{
	std::list<LogDescription> logs;
	pthread_mutex_t logs_mutex;
	pthread_cond_t logs_not_empty_cond;
public:
	LogServer(int p, int q, char *cf, char *pkf)
	: WebSocketServer(p, q, cf, pkf)
	{
		logs_mutex = PTHREAD_MUTEX_INITIALIZER;
		pthread_cond_init(&logs_not_empty_cond, NULL);
	}

	inline int LockLogsList()
	{
		if (pthread_mutex_lock(&logs_mutex))
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}

	inline int ReleaseLogsList()
	{
		if (pthread_mutex_unlock(&logs_mutex))
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	inline void WaitUntilLogsListIsNotEmpty()
	{
		pthread_mutex_lock(&logs_mutex);
		if (logs.size() == 0)
		{
			pthread_cond_wait(&logs_not_empty_cond, &logs_mutex);
		}
		//pthread_mutex_unlock(&clients_mutex);
	}

	inline void TellThatLogsListIsNotEmpty()
	{
		pthread_cond_signal(&logs_not_empty_cond);
	}

	int OpenLog(char *fileName, int *retFd, pid_t *retPid)
	{
		int tailPipe[2];
		if (pipe(tailPipe) == -1)
		{
			return -1;
		}

		pid_t pid = fork();
		if (pid < 0)
			return -2;
		else if (pid > 0)
		{
			close(tailPipe[1]);
			*retFd = tailPipe[0];
			*retPid = pid;
			return 0;
		}

		char *args[] = {"tail", "-F", "-n", "0", fileName, NULL};
		close(tailPipe[0]);
		dup2(tailPipe[1], 1);
		execvp("tail", args);
		return -3;
	}

	int CheckLogName(char *log_name)
	{
		//TODO: MOAR of some smartass checks
		if (strstr(log_name, "..") != NULL)
		{
			return -1;
		}
		return 0;
	}

	inline void OnDisconnect(Client &who)
	{
		DeleteRequestsFromClient(who.get_id());
	}

	int StartReadingLog(Client &client, char *log_name)
	{
		const int buf_len = 2048;
		char buf[buf_len];
		int ret;

		FILE *f = fopen(log_name, "r");
		if (!f)
		{
			return -2;
		}

		while (fgets(buf, buf_len, f))
		{
			client.Send(buf, strlen(buf));
		}
		fclose(f);
		Log("Content of \"%s\" has been sent\n", log_name);

		LogDescription log;
		ret = OpenLog(log_name, &(log.fd), &(log.tail_pid));
		if (ret < 0)
		{
			Log("OpenLog returned %i, rejecting\n", ret);
			return -1;
		}
		else
		{
			log.SetNonBlocking();
			Log("Opened \"%s\", fd flags: %i\n", log_name, fcntl(log.fd, F_GETFL));
			log.name.assign(log_name);
			log.client_id = client.get_id();

			Log("Locking logs list\n");
			LockLogsList();
			logs.push_back(log);
			if (logs.size() == 1)
			{
				TellThatLogsListIsNotEmpty();
			}
			ReleaseLogsList();
			Log("Released logs list\n");
		}
		return 0;
	}

	inline void StopReadingLog(Client &client, char *log_name)
	{
		LockLogsList();
		int client_id = client.get_id();
		for (auto it = logs.begin(), end = logs.end(); it != end;)
		{
			if (it->client_id == client_id && it->name.compare(log_name) == 0)
			{
				it->Cleanup();
				logs.erase(it++);
				break;
			}
			else
			{
				it++;
			}
		}
		ReleaseLogsList();
	}

	void OnMessage(Client &sender, char *message, int len)
	{
		Log("OnMessage fired\n");
		int ret;
		if (CheckLogName(message) == 0)
		{
			char *file_name_offset = strstr(message, " ");
			if (file_name_offset == NULL)
			{
				//send something to client to let him know
				//that we rejected his request
				return;
			}
			else
			{
				file_name_offset++;//adjust pointer to point to beginning of file name
				if (strncmp(add_action, message, strlen(add_action)) == 0)
				{
					ret = StartReadingLog(sender, file_name_offset);
					if (ret < 0)
					{
						Log("Failed to open log file: \"%s\" with code %i\n", file_name_offset, ret);
						return;
					}
				}
				else if (strncmp(del_action, message, strlen(del_action)) == 0)
				{
					StopReadingLog(sender, file_name_offset);
				}
			}

		}
		else
		{
			//send something to client to let him know
			//that we rejected his request
		}
	}

	inline void DeleteRequestsFromClient(int client_id)
	{
		LockLogsList();
		for (auto it = logs.begin(), end = logs.end(); it != end;)
		{
			if (it->client_id == client_id)
			{
				it->Cleanup();
				logs.erase(it++);
			}
			else
			{
				it++;
			}
		}
		ReleaseLogsList();
	}


	int Run()
	{
		if (WebSocketServer::Run() < 0)
		{
			return -1;
		}

		Log("Starting read logs\n");	
		
		const int buf_len = 2048;
		char buf[buf_len];
		
		int ret, nfds;
		fd_set read_fds, except_fds;
		struct timeval timeout;
		
		while (!stop)
		{
			timeout.tv_sec = 2;
			timeout.tv_usec = 0;

			FD_ZERO(&read_fds);
			FD_ZERO(&except_fds);
			nfds = -1;

			//lock the list, make sure it's not empty and fill fd sets
			//with fd's of openel log files
			WaitUntilLogsListIsNotEmpty();
			for (auto it = logs.begin(), end = logs.end(); it != end; it++)
			{
				FD_SET(it->fd, &read_fds);
				FD_SET(it->fd, &except_fds);
				if (it->fd > nfds)
				{
					nfds = it->fd;
				}
			}
			nfds++;
			ReleaseLogsList();

			//wait until one or more logs have new lines or error happens.
			//this has a 2 seconds timeout so when new log is added to the list
			//we will read it as well (without timeout we would block here until
			//one of previously opened logs is changed)
			ret = select(nfds, &read_fds, NULL, &except_fds, &timeout);
			if (ret < 0)
			{
				if (errno == EINTR)
				{
					Log("Select returned eintr\n");
					continue;
				}
				else
				{
					Log("Select error, errno tells: %s\n", strerror(errno));
				}
			}
			else if (ret == 0)
			{
				continue;
			}

			//iterate through all logs, see if we can read anything and do it if so
			LockLogsList();
			for (auto it = logs.begin(); it != logs.end();)
			{
				if (FD_ISSET(it->fd, &read_fds))
				{
					//if log was requested by client who has already disconnected,
					//delete it. Maybe it's worth deleting this check as we
					//delete all requests from client when he disconnects.
					auto client_it = clients.find(it->client_id);
					if (client_it == clients.end())
					{
						it->Cleanup();
						logs.erase(it++);
						continue;
					}
					do
					{
						ret = read(it->fd, buf, buf_len);
						if (ret > 0)
						{
							client_it->second.Send(buf, ret);
						}
					}
					while (ret == buf_len);
				}
				if (FD_ISSET(it->fd, &except_fds))
				{
					Log("Exception for %s\n", it->name.c_str());
					it->Cleanup();
					logs.erase(it++);
					continue;
				}
				it++;
			}
			ReleaseLogsList();
		}
	}
};

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		printf("log-server <port> </path/to/certificate> </path/to/private/key>\n");
		return -1;
	}
	LogServer s(atoi(argv[1]), 10, argv[2], argv[3]);
	s.Run();

	return 0;
}
