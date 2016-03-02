#ifndef TDISK_NL
#define TDISK_NL

#include "../include/tdisk/interface.h"

#define NLTD_TIMEOUT_MSECS 10000

typedef void (*plugin_callback)(void*,int);

int nltd_register(void);
void nltd_unregister(void);

int nltd_is_registered(const char *plugin);
loff_t nltd_get_size(const char *plugin);

void nltd_read_async(const char *plugin, loff_t offset, char *buffer, int length, plugin_callback callback, void *userobject);
void nltd_write_async(const char *plugin, loff_t offset, char *buffer, int length, plugin_callback callback, void *userobject);

int nltd_read_sync(const char *plugin, loff_t offset, char *buffer, int length);
int nltd_write_sync(const char *plugin, loff_t offset, char *buffer, int length);

#endif //TDISK_NL
