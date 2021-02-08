#ifndef __FUTEX_H__
#define __FUTEX_H__

#include <stdio.h>
#include <stdlib.h>
#include <linux/sched.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/futex.h>
#include <sys/wait.h>
#include <errno.h>

void pi_futex_wait(int *futexp);
void pi_futex_wake(int *futexp);
void pi_futex_wait_many(int *futexp, int count);
void pi_futex_wake_many(int *futexp, int count);

#endif /* __FUTEX_H__ */
