#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/bio.h>
#include <linux/blkdev.h>

#include "tdisk_worker_thread.h"

void worker_function(struct kthread_work *work);

/**
  * This struct is used to queue the work to be
  * done for the worker thread. It contains the
  * <linux/kthread.h> specific data for a worker
  * thread and the struction bio information to
  * be performed.
 **/
struct bio_work
{
	struct kthread_work work;
	struct bio *request;
}; //end struct bio_work

struct kthread_worker *worker;
struct task_struct *thread;

void tdisk_worker_thread_start(void)
{
	worker = vmalloc(sizeof(struct kthread_worker));
	*worker = (struct kthread_worker)KTHREAD_WORKER_INIT(*worker);

	thread = kthread_run(&kthread_worker_fn, worker, "tdisk_worker_thread");
}

void tdisk_worker_thread_stop(void)
{
	flush_kthread_worker(worker);
	kthread_stop(thread);
	vfree(worker);
}

void tdisk_worker_thread_add_task(struct bio *request)
{
	struct bio_work *work = vmalloc(sizeof(struct bio_work));
	work->work = (struct kthread_work)KTHREAD_WORK_INIT(work->work, &worker_function);
	work->request = request;
	queue_kthread_work(worker, (struct kthread_work*)work);
}

void worker_function(struct kthread_work *w)
{
	struct bio_work *work = (struct bio_work*)w;
	printk(KERN_DEBUG "tDisk: worker_thread: new request to make: %p\n", work->request);

	generic_make_request(work->request);

	printk(KERN_DEBUG "tDIsk: IRQ is %s\n", (irqs_disabled() ? "disabled" : "enabled"));
	//if(work->request->bi_end_io != NULL)work->request->bi_end_io(work->request, 0);
	bio_put(work->request);

	//vfree(work);
}
