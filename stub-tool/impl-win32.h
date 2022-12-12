#ifndef _IMPL_WIN32_H_
#define _IMPL_WIN32_H_

#include <malloc.h>
#include <stdio.h>
#include <windows.h>
#include <wchar.h>
#include <fcntl.h>
#include <io.h>

#include "main.h"

#define _WIN32_CONNECTION_PREFIX2(x) L"\\\\.\\pipe\\" L##x L"."
#define _WIN32_CONNECTION_PREFIX1(x) _WIN32_CONNECTION_PREFIX2(x)
#define WIN32_CONNECTION_PREFIX _WIN32_CONNECTION_PREFIX1(CONNECTION_PREFIX)

static HANDLE pipe_handle = INVALID_HANDLE_VALUE;

static void fatal(const char *message)
{
	if (pipe_handle != INVALID_HANDLE_VALUE)
	{
		disconnect_from_controller();
	}
	fprintf(stderr, "%s\r\n", message);
	exit(99);
}

static void connect_to_controller()
{
	int n;
	WCHAR pipe_name[256];
	size_t prefix_len = wcslen(WIN32_CONNECTION_PREFIX);
	WCHAR *id_ptr = pipe_name + prefix_len;
	wcscpy(pipe_name, WIN32_CONNECTION_PREFIX);
	n = GetEnvironmentVariableW(L"REMOTE_JOBS_CONNECTION_ID", id_ptr, sizeof(pipe_name) - prefix_len);
	if (n <= 0)
	{
		wcscpy(id_ptr, L"0");
	}
	test(n < sizeof(pipe_name) - prefix_len, "Connection id too long.");
	pipe_handle = CreateFileW(pipe_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	test(pipe_handle != INVALID_HANDLE_VALUE, "Cannot open pipe to controller.");
}

static void disconnect_from_controller()
{
	CloseHandle(pipe_handle);
	pipe_handle = INVALID_HANDLE_VALUE;
}

static size_t send_part(const uint8_t *data, size_t max_size)
{
	DWORD written;
	WINBOOL ok = WriteFile(pipe_handle, data, max_size, &written, NULL);
	test(ok, "Writing to pipe failed.");
	test(written > 0, "Controller stopped receiving data.");
	return written;
}

static size_t recv_part(uint8_t *data, size_t max_size)
{
	DWORD read;
	WINBOOL ok = ReadFile(pipe_handle, data, max_size, &read, NULL);
	test(ok, "Reading from pipe failed.");
	test(read > 0, "Controller stopped sending data.");
	return read;
}

static void send_str(const ichar *str)
{
	int n;
	static char *temp = NULL;
	static int temp_len = 0;
	n = WideCharToMultiByte(CP_UTF8, 0, str, -1, temp, temp_len, NULL, NULL);
	if (n <= 0 || n > temp_len)
	{
		n = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
		if (temp != NULL)
		{
			free(temp);
		}
		temp_len = n + n / 2 + 16;
		temp = (char *)malloc(temp_len);
		test(temp != NULL, "Failed to allocate memory.");
		n = WideCharToMultiByte(CP_UTF8, 0, str, -1, temp, temp_len, NULL, NULL);
	}
	test(n > 0 && n <= temp_len, "Failed to convert string.");
	send_int(n - 1);
	send_all(temp, n - 1);
}

static void get_process_info(int argc, char *argv[])
{
	// Set stdout to binary mode to avoid any tranformations
	_setmode(_fileno(stdout), _O_BINARY);

	// Get process arguments
	iarg = (const ichar **)CommandLineToArgvW(GetCommandLineW(), &iarg_count);

	// Get current directory
	DWORD n = GetCurrentDirectoryW(0, NULL);
	test(n > 0, "GetCurrentDirectory failed.");
	ichar *cwd = (ichar *)malloc(n);
	test(cwd != NULL, "Memory allocation failed.");
	n = GetCurrentDirectoryW(n, cwd);
	test(n > 0, "GetCurrentDirectory failed.\n");
	icwd = cwd;

	// Get environment variables
	ienv_count = 0;
	const ichar *ptr = GetEnvironmentStringsW();
	while (*ptr)
	{
		ienv_count++;
		ptr += wcslen(ptr) + 1;
	}
	ienv = (const ichar **)malloc(ienv_count * sizeof(ichar *));
	test(ienv != NULL, "Memory allocation failed.");
	ptr = GetEnvironmentStringsW();
	int i;
	for (i = 0; i < ienv_count; i++)
	{
		ienv[i] = ptr;
		ptr += wcslen(ptr) + 1;
	}
}

static size_t write_output(int fd, const uint8_t *data, size_t size)
{
	FILE *dest = fd == 1 ? stdout : stderr;
	int n = fwrite(data, 1, size, dest);
	test(n > 0, "Write to stdout or stderr failed.");
	return n;
}

/*void ErrorExit(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
									  (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	wsprintf((LPTSTR)lpDisplayBuf,
			 TEXT("%s failed with error %d: %s"),
			 lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
}*/

#endif
