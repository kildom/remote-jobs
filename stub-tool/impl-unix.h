#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"
#include "md5.h"


#define UNIX_CONNECTION_PREFIX "/tmp/" CONNECTION_PREFIX "/"

extern char **environ;

// Connection data
static int client_sock = -1;
static char client_path[128];

// Environment info
static int env_count;
static uint8_t env_hash[16];
static const char *cwd;

// Receive buffer
static uint8_t recv_buffer[2048];
static size_t recv_buffer_size = 0;
static size_t recv_buffer_start = 0;

// Send buffer
static uint8_t send_buffer[2048];
static size_t send_buffer_size = 0;

static void disconnect_from_controller();

static void die_with_message(const char *message)
{
	if (client_sock >= 0)
	{
		disconnect_from_controller();
	}
	int r = write(2, message, strlen(message));
	(void)r;
	exit(99);
}

static void assert_with_message(bool cond, const char *message)
{
	if (!cond)
	{
		die_with_message(message);
	}
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
	assert_with_message(rc > 0 && rc < MEMBER_SIZE(struct sockaddr_un, sun_path),
			    "Connection id too long.\n");

	mkdir(UNIX_CONNECTION_PREFIX, 0777);

	client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	assert_with_message(client_sock >= 0, "Cannot create UNIX socket.\n");

	unlink(client_path);
	sockaddr.sun_family = AF_UNIX;
	strcpy(sockaddr.sun_path, client_path);
	rc = bind(client_sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	assert_with_message(rc >= 0, "Cannot bind path to UNIX socket.\n");

	sockaddr.sun_family = AF_UNIX;
	strcpy(sockaddr.sun_path, server_path);
	rc = connect(client_sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	assert_with_message(rc >= 0, "Cannot connect to controller.\n");
}

static void disconnect_from_controller()
{
	close(client_sock);
	unlink(client_path);
	client_sock = -1;
}

static void flush_send_buffer()
{
	const uint8_t *ptr = send_buffer;
	while (send_buffer_size > 0)
	{
		ssize_t n = send(client_sock, ptr, send_buffer_size, 0);
		assert_with_message(n >= 0, "Sending to controller error.\n");
		assert_with_message(n > 0, "Controller stopped receiving data.\n");
		send_buffer_size -= n;
		ptr += n;
	}
}

static size_t send_part_to_controller(const uint8_t *data, size_t size)
{
	size_t copy_bytes = MIN(size, sizeof(send_buffer) - send_buffer_size);
	memcpy(&send_buffer[send_buffer_size], data, copy_bytes);
	send_buffer_size += copy_bytes;
	if (send_buffer_size == sizeof(send_buffer))
	{
		flush_send_buffer();
	}
	return copy_bytes;
}

static void send_to_controller(const void *data, size_t size)
{
	const uint8_t *ptr = (const uint8_t *)data;
	while (size > 0)
	{
		size_t n = send_part_to_controller(ptr, size);
		size -= n;
		ptr += n;
	}
}

static void send_int_to_controller(uint32_t value)
{
	send_to_controller(&value, sizeof(value));
}

static void send_str_to_controller(const char *str)
{
	size_t len = strlen(str);
	send_int_to_controller(len);
	send_to_controller(str, len);
}

static void fill_recv_buffer()
{
	int n;
	if (recv_buffer_start >= recv_buffer_size)
	{
		flush_send_buffer();
		n = recv(client_sock, recv_buffer, sizeof(recv_buffer), 0);
		assert_with_message(n >= 0, "Communication with controller failed.\n");
		assert_with_message(n > 0, "Controller closed communication unexpectedly.\n");
		recv_buffer_start = 0;
		recv_buffer_size = n;
	}
}

static void *recv_raw_from_controller(size_t maximum_size, size_t *returned_size)
{
	void *result;
	fill_recv_buffer();
	result = &recv_buffer[recv_buffer_start];
	*returned_size = MIN(maximum_size, recv_buffer_size - recv_buffer_start);
	recv_buffer_start += *returned_size;
	return result;
}

static void recv_from_controller(void *data, size_t size)
{
	uint8_t *ptr = data;
	while (size > 0)
	{
		size_t n;
		void *src = recv_raw_from_controller(size, &n);
		memcpy(ptr, src, n);
		ptr += n;
		size -= n;
	}
}

static uint32_t recv_int_from_controller()
{
	uint32_t result;
	recv_from_controller(&result, sizeof(result));
	return result;
}

static int env_compare(const void *a, const void *b)
{
	return strcmp(a, b);
}

static void prepare_info()
{
	int i;
	md5_ctx ctx;
	char **env_ptr;

	// Current working directory
	cwd = getcwd(recv_buffer, sizeof(recv_buffer));
	assert_with_message(cwd != NULL, "Cannot get current working directory.\n");

	// Count environment variables
	env_ptr = environ;
	while (*env_ptr != NULL)
	{
		env_ptr++;
	}
	env_count = env_ptr - environ;

	// Copy and sort environment variables
	env_ptr = malloc(env_count * sizeof(char *));
	memcpy(env_ptr, environ, env_count * sizeof(char *));
	qsort(env_ptr, env_count, sizeof(char *), env_compare);

	// Calculate MD5 of environment variables
	md5_init(&ctx);
	for (i = 0; i < env_count; i++)
	{
		md5_update(&ctx, env_ptr[i], strlen(env_ptr[i]) + 1);
	}
	md5_digest(&ctx, env_hash);

	free(env_ptr);
}

int main(int argc, char *argv[])
{
	int i;
	size_t len;

	prepare_info();

	connect_to_controller();

	send_int_to_controller(0x7F4A9400);

	send_int_to_controller(argc);
	for (int i = 0; i < argc; i++)
	{
		send_str_to_controller(argv[i]);
	}

	send_str_to_controller(cwd);

	send_int_to_controller(sizeof(env_hash));
	send_to_controller(env_hash, sizeof(env_hash));

	while (1)
	{
		uint32_t cmd = recv_int_from_controller();
		switch (cmd)
		{
		case 0:
			i = recv_int_from_controller();
			disconnect_from_controller();
			return i;
		case 1:
		case 2:
			len = recv_int_from_controller();
			while (len > 0)
			{
				size_t chunk_len;
				uint8_t *ptr = recv_raw_from_controller(len, &chunk_len);
				while (chunk_len > 0)
				{
					int n = write(cmd, ptr, chunk_len);
					assert_with_message(n > 0, "Unexpected IO error.\n");
					ptr += n;
					chunk_len -= n;
					len -= n;
				}
			}
			break;
		case 3:
			send_int_to_controller(env_count);
			for (i = 0; i < env_count; i++)
			{
				send_str_to_controller(environ[i]);
			}
			break;
		default:
			die_with_message("Controller version mismatch.\n");
		}
	}
}
