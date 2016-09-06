/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef WORKER_TIMEOUT_H
#define WORKER_TIMEOUT_H

#include <tdisk/config.h>

#pragma GCC system_header
#include <linux/freezer.h>
#include <linux/kthread.h>

/**
  * Initializes a kthread_work to be used
  * with a timeout_worker
 **/
#define init_thread_work_timeout(work) \
	init_kthread_work((work), NULL);

/**
  * This enum represents the current status of
  * the worker thread.
  * It is returned from the user specific worker
  * function and is used to decide about sleeping
  * times. Normally it looks like this:
  * Primary work (which is the work represented
  * by struct kthread_work) always has the higher
  * priority. If there is primary work to be done,
  * it is done before any secondary work even if
  * there is still some secondary work to do.
  * When the function is called to do some primary
  * work it usually should return next_primary_work
  * to wait for the next primary work. If there is
  * no primary work within the given timeout, the
  * work function is called to do some secondary work
  * then the worker function should either return
  * secondary_work_finished or secondary_work_to_do.
  * If secondary_work_to_do is returned, the worker
  * function is immediately called again to do
  * secondary work. Of course, only if there is no
  * primary work to be done .If secondary_work_finished
  * is returned. The thread goes to sleep and is only
  * woken up if there is primary work to be done...
 **/
enum worker_status
{
	next_primary_work,
	secondary_work_finished,
	secondary_work_to_do
}; //end enum worker_status

/**
  * This data needs to be handed over by kthread_run
  * to start the timeout worker thread. e.g.
  * kthread_run(kthread_worker_fn_timeout, worker_timeout_data...
  * timeout is the default timeout which needs to be exceeded
  * in order to start secondary work.
  * secondary_work_delay is the time which is waited between
  * each secondary work. This is e.g. useful if the secondary
  * work is heavy work and the CPU shouldn't be used too much.
  * private_data is the private data which should be handed
  * over to the worker function, worker_func is the worker
  * function which should be started for every work to be done.
  * If kthread_work is null, then it is secondary work, if
  * kthread_work is not null it is primary work.
 **/
struct worker_timeout_data
{
	spinlock_t lock;
 	struct list_head work;
	struct task_struct *thread;

	long timeout;
	long secondary_work_delay;
	void *private_data;
	enum worker_status(*work_func)(void*,struct kthread_work*);
}; //end struct worker timeout data

/**
  * This function is used to initialize
  * the timeout worker
 **/
struct task_struct* start_worker_timeout(struct worker_timeout_data *data, const char namefmt[], ...);

/**
  * This function is used to insert a work into
  * the wor queue
 **/
void enqueue_work(struct worker_timeout_data *data, struct kthread_work *work);

void flush_kthread_worker_timeout(struct worker_timeout_data *data);

#endif //WORKER_TIMEOUT_H
