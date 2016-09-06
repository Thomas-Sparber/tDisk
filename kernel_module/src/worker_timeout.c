/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <tdisk/config.h>
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

	while(!kthread_should_stop())
	{
		set_current_state(TASK_INTERRUPTIBLE);

		work = NULL;
		spin_lock_irq(&data->lock);
		if(!list_empty(&data->work))
		{
			work = list_first_entry(&data->work, struct kthread_work, node);
			list_del_init(&work->node);
		}
		spin_unlock_irq(&data->lock);

		if(unlikely(work && work->func))
		{
			//Flush operation
			__set_current_state(TASK_RUNNING);
			WARN_ON(work->func != &kthread_flush_work_fn_timeout);
			work->func(work);
		}
		else if(work || current_timeout == 0)
		{
			//if(!work)printk(KERN_DEBUG "tDisk worker: no work to do, timeout %d/%d\n", current_timeout, data->timeout);
			__set_current_state(TASK_RUNNING);
			switch(data->work_func(data->private_data, work))
			{
			case next_primary_work:
				current_timeout = data->timeout;
				break;
			case secondary_work_to_do:
				//printk(KERN_DEBUG "tDisk worker: No more primary work\n");
				current_timeout = 0;
				set_current_state(TASK_INTERRUPTIBLE);
				schedule_timeout(data->secondary_work_delay);
				break;
			case secondary_work_finished:
				current_timeout = MAX_SCHEDULE_TIMEOUT;
				break;
			default:
				BUG_ON(1);
				break;
			}
		}
		else
		{
			//printk(KERN_DEBUG "tDisk worker: sleeping %d jiffies\n", current_timeout);
			current_timeout = schedule_timeout(current_timeout);
			//printk(KERN_DEBUG "tDisk worker: woke up, %d jiffies left\n", current_timeout);
		}
	}

	__set_current_state(TASK_RUNNING);
	return 0;
}

/**
  * Flushes the given worker thread
 **/
void flush_kthread_worker_timeout(struct worker_timeout_data *data)
{
	struct kthread_flush_work fwork =
	{
		KTHREAD_WORK_INIT(fwork.work, kthread_flush_work_fn_timeout),
		COMPLETION_INITIALIZER_ONSTACK(fwork.done),
	};

	enqueue_work(data, &fwork.work);
	wait_for_completion(&fwork.done);
}
EXPORT_SYMBOL(flush_kthread_worker_timeout);

void enqueue_work(struct worker_timeout_data *data, struct kthread_work *work)
{
	unsigned long flags;

	spin_lock_irqsave(&data->lock, flags);
	list_add_tail(&work->node, &data->work);
	wake_up_process(data->thread);
	spin_unlock_irqrestore(&data->lock, flags);
}
EXPORT_SYMBOL(enqueue_work);

struct task_struct* start_worker_timeout(struct worker_timeout_data *data, const char namefmt[], ...)
{
	char buf[64];
	va_list args;

	spin_lock_init(&data->lock);
	INIT_LIST_HEAD(&data->work);

	va_start(args, namefmt);
	vsnprintf(buf, sizeof(buf), namefmt, args);
	buf[sizeof(buf) - 1] = '\0';	//To really make sure...
	va_end(args);

	data->thread = kthread_create(&kthread_worker_fn_timeout, data, buf);
	wake_up_process(data->thread);

	return data->thread;
}
EXPORT_SYMBOL(start_worker_timeout);
