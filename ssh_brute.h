#ifndef __SSH_BRUTE_H__
#define __SSH_BRUTE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <libssh/libssh.h>

#include "futex.h"

struct ssh_brute_options {
	char *password;
	char *host;
	char *username;
	int port;
	int count;
	int *completed;
	char *result;
	int *futex_result;
};

struct ssh_username_options {
	char *host;
	char *username;
	int port;
	char **passwords;
	int pcount;
	int *completed;
	int *futex_result;
};

void *user_connection_thread(void *arg);

#endif
