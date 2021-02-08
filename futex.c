#include "futex.h"

static int futex(int *uaddr, int futex_op, int val, const struct timespec *timeout,
	  int *uaddr2, int val3)
{
	return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

void pi_futex_wait(int *futexp)
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

void pi_futex_wake(int *futexp)
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

void pi_futex_wait_many(int *futexp, int count)
{
	int s;

	while (1) {
		if (__sync_bool_compare_and_swap(futexp, count, count)) {
			break;
		}

		s = futex(futexp, FUTEX_WAIT, count, NULL, NULL, 0);
		if (s == -1 && errno != EAGAIN) {
			perror("futex wait");
			exit(1);
		}
	}
}

void pi_futex_wake_many(int *futexp, int count)
{
	int s;

	//printf("[%s] val [%u]\n", __func__, __sync_add_and_fetch(futexp, 1));
	__sync_add_and_fetch(futexp, 1);
	if (__sync_bool_compare_and_swap(futexp, count, count)) {
		s = futex(futexp, FUTEX_WAKE, 1, NULL, NULL, 0);
		if (s == -1 && errno != EAGAIN) {
			perror("futex wait");
			exit(1);
		}
	}
}
