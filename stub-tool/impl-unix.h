#ifndef _IMPL_UNIX_H_
#define _IMPL_UNIX_H_
#ifndef WIN32

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

#include "main.h"

#define UNIX_CONNECTION_PREFIX "/tmp/" CONNECTION_PREFIX "/"

extern char **environ;

static int client_sock = -1;
static char client_path[128];

static void fatal(const char *message)
{
	if (client_sock >= 0)
	{
		disconnect_from_controller();
	}
	int r = write(2, message, strlen(message));
	r = write(2, "\n", 1);
	(void)r;
	exit(99);
}

static void connect_to_controller()
{
	int rc;
	struct sockaddr_un sockaddr;
	char server_path[128];

	const char *id = getenv("REMOTE_JOBS_CONNECTION_ID");
	if (id == NULL)
	{
		id = "0";
	}

	snprintf(server_path, sizeof(server_path), "%s%sS", UNIX_CONNECTION_PREFIX, id);
	rc = snprintf(client_path, sizeof(client_path), "%s%sC%d", UNIX_CONNECTION_PREFIX, id, (int)getpid());
	test(rc > 0 && rc < MEMBER_SIZE(struct sockaddr_un, sun_path), "Connection id too long.");

	client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	test(client_sock >= 0, "Cannot create UNIX socket.");

	unlink(client_path);
	sockaddr.sun_family = AF_UNIX;
	strcpy(sockaddr.sun_path, client_path);
	rc = bind(client_sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	test(rc >= 0, "Cannot bind path to UNIX socket.");

	sockaddr.sun_family = AF_UNIX;
	strcpy(sockaddr.sun_path, server_path);
	rc = connect(client_sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	test(rc >= 0, "Cannot connect to controller.");
}

static void disconnect_from_controller()
{
	close(client_sock);
	unlink(client_path);
	client_sock = -1;
}

static size_t send_part(const uint8_t *data, size_t max_size)
{
	ssize_t n = send(client_sock, data, max_size, 0);
	test(n >= 0, "Sending to controller error.");
	test(n > 0, "Controller stopped receiving data.");
	return n;
}

static size_t recv_part(uint8_t *data, size_t max_size)
{
	ssize_t n = recv(client_sock, data, max_size, 0);
	test(n >= 0, "Communication with controller failed.");
	test(n > 0, "Controller closed communication unexpectedly.");
	return n;
}

static void send_str(const ichar *str)
{
	size_t len = strlen(str);
	send_int(len);
	send_all(str, len);
}

static void get_process_info(int argc, char *argv[])
{
	// Get process arguments
	iarg_count = argc;
	iarg = (const ichar **)argv;

	// Current working directory
	icwd = getcwd(buffer, sizeof(buffer));
	test(icwd != NULL, "Cannot get current working directory.");

	// Get environment variables
	char **env_ptr = environ;
	while (*env_ptr != NULL)
	{
		env_ptr++;
	}
	ienv_count = env_ptr - environ;
	ienv = malloc(ienv_count * sizeof(ichar *));
	memcpy(ienv, environ, ienv_count * sizeof(ichar *));
}

static size_t write_output(int fd, const uint8_t *data, size_t size)
{
	int n = write(fd, data, size);
	test(n >= 0, "Write to stdout or stderr failed.");
	test(n > 0, "Cannot write more data to stdout or stderr.");
	return n;
}

#endif
#endif
