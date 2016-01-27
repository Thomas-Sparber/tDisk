#ifndef WORKER_TIMEOUT_H
#define WORKER_TIMEOUT_H

#include <linux/kthread.h>

#define init_thread_work_timeout(work) \
	init_kthread_work((work), NULL);

struct worker_timeout_data
{
	struct kthread_worker worker;
	long timeout;
	void *private_data;
	void(*work_func)(void*,struct kthread_work*);
}; //end struct worker timeout data

int kthread_worker_fn_timeout(void *worker_ptr);
void flush_kthread_worker_timeout(struct kthread_worker *worker);

#endif //WORKER_TIMEOUT_H
