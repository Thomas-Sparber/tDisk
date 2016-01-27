#include <linux/types.h>
#include <linux/freezer.h>
#include <linux/completion.h>

#include "worker_timeout.h"

struct kthread_flush_work {
	struct kthread_work	work;
	struct completion	done;
};

static void kthread_flush_work_fn_timeout(struct kthread_work *work)
{
	struct kthread_flush_work *fwork = container_of(work, struct kthread_flush_work, work);
	complete(&fwork->done);
}

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
		current_timeout = data->timeout;
		__set_current_state(TASK_RUNNING);
		data->work_func(data->private_data, work);
	}
	else if(!freezing(current))
		current_timeout = schedule_timeout(data->timeout);

	try_to_freeze();
	goto repeat;
}

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
