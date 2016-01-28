/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#include <linux/types.h>
#include <linux/freezer.h>
#include <linux/completion.h>

#include "worker_timeout.h"

/**
  * This struct represents a work object which is
  * used to flush the worker thread
 **/
struct kthread_flush_work {
	struct kthread_work	work;
	struct completion	done;
};

/**
  * This function is called for the flush work.
  * It notifies the completion object.
 **/
static void kthread_flush_work_fn_timeout(struct kthread_work *work)
{
	struct kthread_flush_work *fwork = container_of(work, struct kthread_flush_work, work);
	complete(&fwork->done);
}

/**
  * This function is more or less copied from the linux
  * kernel version except that it is able to wake
  * up after a specific timeout, even if there is no
  * work to do.
  * This makes it possible to do some background
  * (secondary) work when there is nothing to do.
 **/
int kthread_worker_fn_timeout(void *worker_ptr)
{
	struct worker_timeout_data *data = worker_ptr;
	long current_timeout = data->timeout;
	struct kthread_work *work;

	WARN_ON(data->worker.task);
	data->worker.task = current;
 repeat:
	set_current_state(TASK_INTERRUPTIBLE);

	if(kthread_should_stop())
	{
		__set_current_state(TASK_RUNNING);
		spin_lock_irq(&data->worker.lock);
		data->worker.task = NULL;
		spin_unlock_irq(&data->worker.lock);
		return 0;
	}

	work = NULL;
	spin_lock_irq(&data->worker.lock);
	if(!list_empty(&data->worker.work_list))
	{
		work = list_first_entry(&data->worker.work_list, struct kthread_work, node);
		list_del_init(&work->node);
	}
	data->worker.current_work = work;
	spin_unlock_irq(&data->worker.lock);

	if(unlikely(work && work->func))
	{
		//Flush operation
		BUG_ON(work->func != &kthread_flush_work_fn_timeout);
		work->func(work);
	}
	else if(work || current_timeout == 0)
	{
		__set_current_state(TASK_RUNNING);
		switch(data->work_func(data->private_data, work))
		{
		case next_primary_work:
			current_timeout = data->timeout;
			break;
		case secondary_work_finished:
			current_timeout = MAX_SCHEDULE_TIMEOUT;
			break;
		case secondary_work_to_do:
			current_timeout = 0;
			break;
		}
	}
	else if(!freezing(current))
		current_timeout = schedule_timeout(current_timeout);

	try_to_freeze();
	goto repeat;
}

/**
  * Flushes the given worker thread
 **/
void flush_kthread_worker_timeout(struct kthread_worker *worker)
{
	struct kthread_flush_work fwork =
	{
		KTHREAD_WORK_INIT(fwork.work, kthread_flush_work_fn_timeout),
		COMPLETION_INITIALIZER_ONSTACK(fwork.done),
	};

	queue_kthread_work(worker, &fwork.work);
	wait_for_completion(&fwork.done);
}
