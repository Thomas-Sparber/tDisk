#ifndef TDISK_WORKER_THREAD_H
#define TDISK_WORKER_THREAD_H

#include <linux/blk_types.h>

void tdisk_worker_thread_start(void);
void tdisk_worker_thread_stop(void);

void tdisk_worker_thread_add_task(struct bio *request);

#endif //TDISK_WORKER_THREAD_H
