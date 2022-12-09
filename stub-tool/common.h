#ifndef _COMMON_H_
#define _COMMON_H_


#define CONNECTION_PREFIX "RemJobs75oKmnN7rWX"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MEMBER_SIZE
#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)
#endif


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
