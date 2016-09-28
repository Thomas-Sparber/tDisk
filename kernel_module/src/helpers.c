#include "helpers.h"
#include "tdisk_tools_config.h"

#ifndef __ASM_ARM_DIV64
#ifndef __div64_32_DEFINED

uint32_t __div64_32(uint64_t *n, uint32_t base)
{
	uint64_t rem = *n;
	uint64_t b = base;
	uint64_t res, d = 1;
	uint32_t high = rem >> 32;

	/* Reduce the thing a bit first */
	res = 0;
	if (high >= base) {
		high /= base;
		res = (uint64_t) high << 32;
		rem -= (uint64_t) (high*base) << 32;
	}

	while ((int64_t)b > 0 && b < rem) {
		b = b+b;
		d = d+d;
	}

	do {
		if (rem >= b) {
			rem -= b;
			res += d;
		}
		b >>= 1;
		d >>= 1;
	} while (d);

	*n = res;
	return rem;
}
EXPORT_SYMBOL(__div64_32);

#endif //__div64_32_DEFINED
#endif //__ASM_ARM_DIV64

#ifdef mutex_lock
#undef mutex_lock
void __sched mutex_lock(struct mutex *lock)
{
	mutex_lock_nested(lock, 0);
}
EXPORT_SYMBOL(mutex_lock);
#endif //mutex_lock
