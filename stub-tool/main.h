/*!
 * Copyright (c) 2022, Dominik Kilian <kontakt@dominik.cc>
 * All rights reserved.
 * 
 * This software is distributed under the BSD 3-Clause License. See the
 * LICENSE.txt file for details.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef WIN32
#include <windows.h>
typedef WCHAR ichar;
#define istrcmp wcscmp
#define istrlen wcslen
#else
typedef char ichar;
#define istrcmp strcmp
#define istrlen strlen
#endif

#define CONNECTION_PREFIX "RemJobs75oKmnN7rWX"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MEMBER_SIZE
#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)
#endif

// Temporary buffer
static uint8_t buffer[65536];

// Process information
static const ichar *icwd;
static int iarg_count;
static const ichar **iarg;
static int ienv_count;
static const ichar **ienv;
static uint8_t env_hash[16];

// Functions implemented by the main file.
static void test(bool cond, const char *message);
static void send_all(const void *data, size_t size);
static void send_int(uint32_t value);
static void recv_all(void *data, size_t size);
static uint32_t recv_int();

// Functions implemented by the platform specific code.
static void fatal(const char *message);
static void connect_to_controller();
static void disconnect_from_controller();
static size_t send_part(const uint8_t *data, size_t max_size);
static size_t recv_part(uint8_t *data, size_t max_size);
static void send_str(const ichar *str);
static void get_process_info(int argc, char *argv[]);
static size_t write_output(int fd, const uint8_t *data, size_t size);

#endif
