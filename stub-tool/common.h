#ifndef _COMMON_H_
#define _COMMON_H_

#include "md5.h"

#define CONNECTION_PREFIX "RemJobs75oKmnN7rWX"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MEMBER_SIZE
#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)
#endif

#ifdef WIN32
typedef WCHAR ichar;
#define istrcmp wcscmp
#define istrlen wcslen
#else
typedef char ichar;
#define istrcmp strcmp
#define istrlen strlen
#endif

static uint8_t buffer[65536];
static const ichar *icwd;
static int iarg_count;
static const ichar **iarg;
static int ienv_count;
static const ichar **ienv;
static uint8_t env_hash[16];

// Functions implemented by the platform specific code.
static void fatal(const char *message);
static void connect_to_controller();
static void disconnect_from_controller();
static size_t send_part(const uint8_t *data, size_t max_size);
static size_t recv_part(uint8_t *data, size_t max_size);
static void send_str(const ichar *str);
static void get_process_info(int argc, char *argv[]);
static size_t write_output(int fd, const uint8_t* data, size_t size);

static void test(bool cond, const char *message)
{
    if (!cond)
    {
        fatal(message);
    }
}

static void send_all(const void *data, size_t size)
{
    const uint8_t *ptr = (const uint8_t *)data;
    while (size > 0)
    {
        size_t n = send_part(ptr, size);
        size -= n;
        ptr += n;
    }
}

static void send_int(uint32_t value)
{
    send_all(&value, sizeof(value));
}

static void recv_all(void *data, size_t size)
{
    uint8_t *ptr = data;
    while (size > 0)
    {
        size_t n = recv_part(ptr, size);
        size -= n;
        ptr += n;
    }
}

static uint32_t recv_int()
{
    uint32_t result;
    recv_all(&result, sizeof(result));
    return result;
}

static int ienv_compare(const void *a, const void *b)
{
	return istrcmp(a, b);
}

static void calc_env_hash()
{
    int i;
    md5_ctx ctx;
    qsort(ienv, ienv_count, sizeof(ichar *), ienv_compare);
    md5_init(&ctx);
    for (i = 0; i < ienv_count; i++)
    {
        md5_update(&ctx, (uint8_t *)ienv[i], sizeof(ichar) * (istrlen(ienv[i]) + 1));
    }
    md5_digest(&ctx, env_hash);
}

int main(int argc, char *argv[])
{
    size_t len;
    int i;

    get_process_info(argc, argv);
    calc_env_hash();

    connect_to_controller();

    send_int(0x7F4A9400);
    send_int(iarg_count);
    for (i = 0; i < iarg_count; i++)
    {
        send_str(iarg[i]);
    }
    send_str(icwd);
    send_int(sizeof(env_hash));
    send_all(env_hash, sizeof(env_hash));

    while (1)
    {
        uint32_t cmd = recv_int();
        switch (cmd)
        {
        case 0:
            i = recv_int();
            disconnect_from_controller();
            return i;
        case 1:
        case 2:
            len = recv_int();
            while (len > 0)
            {
                size_t chunk_len = recv_part(buffer, MIN(len, sizeof(buffer)));
                uint8_t *ptr = buffer;
                while (chunk_len > 0)
                {
                    int n = write_output(cmd, ptr, chunk_len);
                    ptr += n;
                    chunk_len -= n;
                    len -= n;
                }
            }
            break;
        case 3:
            send_int(ienv_count);
            for (i = 0; i < ienv_count; i++)
            {
                send_str(ienv[i]);
            }
            break;
        default:
            fatal("Controller version mismatch.");
        }
    }
}

/*
Communication protocol:

Direction  Bytes  Name        Description
------------------------------------------------------------------------------------------------------
OUT           4   magic        0x7F4A9400: magic value at higher bits, protocol version at lower 8 bits
OUT           4   argc         number of arguments
Repeat for each argument:
    OUT       4   argv_len[]   number of bytes in argument
    OUT       N   argv[]       string containing argument (not null-terminated)
OUT           4   cwd_len      number of bytes in current working directory path
OUT           N   cwd          current working directory (not null-terminated)
OUT           4   env_hash_len number of bytes in environment variables hash
OUT           N   env_hash     environment variables hash (its up to stub program how to calculated it)
Command exchange starts here.

Command "EXIT":
IN            4   cmd          Exit command (cmd=0)
IN            4   status       Program exit status

Command "STDOUT"/"STDERR"
IN            4   cmd          Write to stdout (cmd=1) or stderr (cmd=2)
IN            4   length       Length of the string to write (in bytes)
IN            N   data         String to write

Command "ENV":
IN            4   cmd          Get environment command (cmd=3)
OUT           4   count        Number of environment variables
Repeat for each variable:
    OUT       4   env_len[]    number of bytes in environment variable string
    OUT       N   env[]        string containing environment variable (not null-terminated)

Command "VERSION_ERROR":
IN            4   cmd          Any other value (cmd > 3) should be treated like a protocol version mismatch command.

*/

#endif /* _COMMON_H_ */
