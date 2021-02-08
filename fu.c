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
static int futex_complete_password = 1;
static int futex_complete_users = 1;


static void users_and_passwords_parallel(void)
{
	char *passwords[] = {
		"lalalala",
		"lololo",
		"1",
		"qwerty123",
		"dog777"
	};
	size_t passwords_count = 5;

	char *users[] = {
		"root",
		"nester",
		"user",
		"admin"
	};
	size_t users_count = 4;

	int err = 0, tidx = 0;
	clock_t start, end;
	double cpu_time_used;
	struct ssh_username_options uo[users_count]; 
	size_t completed_users = 0;

	pthread_t *threads;

	threads = malloc(sizeof(pthread_t)*users_count);
	if (!threads) {
		perror("Cannot allocate memory for user threads\n");
		return ;
	}

	printf("\n\n\n lets start user and passwords parallel test\n\n\n");
	start = clock();
	for (tidx = 0; tidx < users_count; tidx++) {
		uo[tidx].host = "192.168.122.190",
		uo[tidx].username = NULL,
		uo[tidx].port = 22,
		uo[tidx].passwords = passwords,
		uo[tidx].pcount = passwords_count,
		uo[tidx].username = users[tidx];
		uo[tidx].completed = &completed_users;
		uo[tidx].futex_result = &futex_result;
		uo[tidx].futex_complete_password = &futex_complete_password;
		uo[tidx].futex_complete_users = &futex_complete_users;

		err = pthread_create(&threads[tidx], 0, user_connection_thread, (void*)&uo[tidx]);
		if (err < 0) {
			printf("Error to create pthread [%d]\n", tidx);
			goto join_threads;
		}
	}

	while (completed_users != users_count) {
		//printf("sleep [%d] [%d]\n", completed, pcount);
		usleep(1000);
	}

join_threads:
	while (tidx) {
		pthread_join(threads[--tidx], NULL);
	}
	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf("user and passwords parallel %f\n", cpu_time_used);

	free(threads);
}

static void only_passwords_parallel(void)
{
	char *passwords[] = {
		"lalalala",
		"lololo",
		"1",
		"qwerty123",
		"dog777"
	};
	size_t passwords_count = 5;

	char *users[] = {
		"root",
		"nester",
		"user",
		"admin"
	};
	size_t users_count = 4;

	size_t completed = 0;

	struct ssh_username_options uo = {
		.host = "192.168.122.190",
		.username = NULL,
		.port = 22,
		.passwords = passwords,
		.pcount = passwords_count,
		.completed = &completed,
		.futex_result = &futex_result,
		.futex_complete_password = &futex_complete_password,
		.futex_complete_users = &futex_complete_users,
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
	users_and_passwords_parallel();
	return 0;
}
