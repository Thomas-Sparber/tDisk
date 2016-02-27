#include <linux/completion.h>
#include <linux/genetlink.h>
#include <linux/list.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <net/genetlink.h>
#include <net/net_namespace.h>
#include <net/sock.h>

#include "tdisk_nl.h"

spinlock_t plugin_lock;

struct tdisk_plugin
{
	char name[NLTD_MAX_NAME];
	u32 port;
	struct list_head list;
}; //end struct tdisk_plugin

LIST_HEAD(registered_plugins);

spinlock_t request_lock;
u32 request_counter;

struct pending_request
{
	int type;
	u32 seq_nr;
	u64 started;
	plugin_callback callback;
	void *userobject;

	char *buffer;
	int buffer_length;

	struct list_head list;
}; //end struct pending_request

LIST_HEAD(pending_requests);

static struct genl_family genl_tdisk_family = {
	.id = GENL_ID_GENERATE,
	.name = NLTD_NAME,
	.hdrsize = NLTD_HEADER_SIZE,
	.version = NLTD_VERSION,
	.maxattr = NLTD_MAX,
};


struct sync_request
{
	int ret;
	struct completion done;
}; //end struct sync_request

void sync_request_callback(void *data, int ret)
{
	struct sync_request *sync = data;
	sync->ret = ret;
	complete(&sync->done);
}

//static struct genl_multicast_group genl_tdisk_multicast = {
//	.name		= NLTD_NAME "Multicast",
//};

static int genl_register(struct sk_buff *skb, struct genl_info *info)
{
	bool found = false;
	char *name;
	int name_length;
	struct tdisk_plugin *plugin;
	if(!info->attrs[NLTD_PLUGIN_NAME])return -EINVAL;
	name = nla_data(info->attrs[NLTD_PLUGIN_NAME]);
	name_length = strlen(name);

	spin_lock_irq(&plugin_lock);
	list_for_each_entry(plugin, &registered_plugins, list)
	{
		if(name_length == strlen(name) && strncmp(name, plugin->name, name_length) == 0)
		{
			//Same plugin registered again. Updating port
			plugin->port = info->snd_portid;
			found = true;
			break;
		}
	}

	if(!found)
	{
		plugin = kmalloc(sizeof(struct tdisk_plugin), GFP_KERNEL);
		memcpy(plugin->name, name, NLTD_MAX_NAME);
		plugin->port = info->snd_portid;

		list_add(&plugin->list, &registered_plugins);
	}
	spin_unlock_irq(&plugin_lock);

	printk(KERN_INFO "tDisk: Plugin %s registered\n", name);
	//nltd_write_async(name, 0, name, NLTD_MAX_NAME, NULL, NULL);
	return 0;
}

static int genl_unregister(struct sk_buff *skb, struct genl_info *info)
{
	bool found = false;
	const char *name;
	int name_length;
	struct tdisk_plugin *n;
	struct tdisk_plugin *plugin;

	if(!info->attrs[NLTD_PLUGIN_NAME])return -EINVAL;
	name = nla_data(info->attrs[NLTD_PLUGIN_NAME]);
	name_length = strlen(name);

	spin_lock_irq(&plugin_lock);
	list_for_each_entry_safe(plugin, n, &registered_plugins, list)
	{
		if(name_length == strlen(name) && strncmp(name, plugin->name, name_length) == 0)
		{
			list_del(&plugin->list);
			kfree(plugin);
			printk(KERN_INFO "tDisk: Plugin %s unregistered\n", name);
			found = true;
			break;
		}
	}
	spin_unlock_irq(&plugin_lock);

	if(!found)
	{
		printk(KERN_WARNING "tDisk: Unable to unregister plugin %s. Not found\n", name);
		return -EINVAL;
	}

	return 0;
}

static struct pending_request* create_request(void)
{
	struct pending_request *req = kzalloc(sizeof(struct pending_request), GFP_KERNEL);
	if(!req)return NULL;

	spin_lock_irq(&request_lock);
	//if(list_empty(&pending_requests) == 0)request_counter = 0;	//Reset counter to prevent overflow -- not needed? Overflow doesnt cause errors...
	req->started = get_jiffies_64();
	req->seq_nr = request_counter++;
	list_add_tail(&req->list, &pending_requests);
	spin_unlock_irq(&request_lock);

	return req;
}

static struct pending_request* pop_request(u32 request_number)
{
	struct pending_request *n;
	struct pending_request *request;
	struct pending_request *req = NULL;

	spin_lock_irq(&request_lock);
	list_for_each_entry_safe(request, n, &pending_requests, list)
	{
		if(request->seq_nr == request_number)
		{
			req = request;
			list_del_init(&request->list);
			break;
		}
	}
	spin_unlock_irq(&request_lock);

	return req;
}

static u32 get_plugin_port(const char *name)
{
	u32 port = 0;
	struct tdisk_plugin *plugin;
	unsigned int name_length = strlen(name);

	spin_lock_irq(&plugin_lock);
	list_for_each_entry(plugin, &registered_plugins, list)
	{
		if(name_length == strlen(plugin->name) && strncmp(name, plugin->name, name_length) == 0)
		{
			port = plugin->port;
			break;
		}
	}
	spin_unlock_irq(&plugin_lock);

	return port;
}

int nltd_is_registered(const char *plugin)
{
	return (get_plugin_port(plugin) != 0);
}

static const char* get_plugin_name(u32 port)
{
	char *name = NULL;
	struct tdisk_plugin *plugin;

	spin_lock_irq(&plugin_lock);
	list_for_each_entry(plugin, &registered_plugins, list)
	{
		if(port == plugin->port)
		{
			name = plugin->name;
			break;
		}
	}
	spin_unlock_irq(&plugin_lock);

	return name;
}

static int genl_finished(struct sk_buff *skb, struct genl_info *info)
{
	int length;
	u32 request_number;
	struct pending_request *req;

	if(!info->attrs[NLTD_REQ_NUMBER])
	{
		const char *plugin_name = get_plugin_name(info->snd_portid);

		printk(KERN_WARNING "tDisk: Probably broken plugin: %s (%u) - didn't send a request number\n", (plugin_name ? plugin_name : "Unknown"), info->snd_portid);
		return -EINVAL;
	}

	//Get fields from message
	request_number = nla_get_u32(info->attrs[NLTD_REQ_NUMBER]);
	length = nla_get_s32(info->attrs[NLTD_REQ_LENGTH]);

	//Get original request structure from pending request
	req = pop_request(request_number);
	if(!req)
	{
		const char *plugin_name = get_plugin_name(info->snd_portid);

		printk(KERN_WARNING "tDisk: Plugin %s (%u) finished a request I didn't know\n", (plugin_name ? plugin_name : "Unknown"), info->snd_portid);
		return -EINVAL;
	}

	//If the type was a read operation, the
	//data need to be stored back to the original data
	if(req->type == READ && length > 0)
	{
		void *data = nla_data(info->attrs[NLTD_REQ_BUFFER]);
		memcpy(req->buffer, data, min(length, req->buffer_length));
	}

	//Finally call callback that the request finished
	if(req->callback)
		req->callback(req->userobject, length);

	return 0;
}

int nltd_send_async(const char *plugin, loff_t offset, char *buffer, int length, int operation, plugin_callback callback, void *userobject)
{
	int err;
	void *hdr;
	struct sk_buff *msg;
	struct pending_request *req;
	u32 port = get_plugin_port(plugin);

	//0 means unregistered
	err = -ENODEV;
	if(port == 0)goto error;

	//TODO set good size
	err = -ENOMEM;
	msg = nlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if(!msg)goto error;

	err = -ENOBUFS;
	if(operation == READ)
		hdr = genlmsg_put(msg, port, 0, &genl_tdisk_family, 0, NLTD_CMD_READ);
	else if(operation == WRITE)
		hdr = genlmsg_put(msg, port, 0, &genl_tdisk_family, 0, NLTD_CMD_WRITE);
	else
	{
		hdr = NULL;
		err = -EINVAL;
		printk(KERN_WARNING "tDisk: invalid plugin operation: %d\n", operation);
		goto nlmsg_failure;
	}
	if(!hdr)goto nlmsg_failure;

	err = -ENOMEM;
	req = create_request();
	if(!req)goto nlmsg_failure;

	req->type = operation;
	req->callback = callback;
	req->userobject = userobject;
	req->buffer = buffer;
	req->buffer_length = length;

	nla_put_u32(msg, NLTD_REQ_NUMBER, req->seq_nr);
	nla_put_u64(msg, NLTD_REQ_OFFSET, offset);
	nla_put_s32(msg, NLTD_REQ_LENGTH, length);
	if(operation != READ)nla_put(msg, NLTD_REQ_BUFFER, length, buffer);
	genlmsg_end(msg, hdr);

	err = genlmsg_unicast(&init_net, msg, port);
	return 0;

 nlmsg_failure:
	genlmsg_cancel(msg, hdr);
	kfree_skb(msg);
 error:
	printk(KERN_WARNING "tDisk: Error sending netlink message: %d\n", err);
	if(callback)callback(userobject, err);
	return err;
}

void nltd_read_async(const char *plugin, loff_t offset, char *buffer, int length, plugin_callback callback, void *userobject)
{
	int ret;

	ret = nltd_send_async(plugin, offset, buffer, length, READ, callback, userobject);

	//Call callback in case of initial error
	if(ret && callback)callback(userobject, -EIO);
}

void nltd_write_async(const char *plugin, loff_t offset, char *buffer, int length, plugin_callback callback, void *userobject)
{
	int ret;

	ret = nltd_send_async(plugin, offset, buffer, length, WRITE, callback, userobject);

	//Call callback in case of initial error
	if(ret && callback)callback(userobject, -EIO);
}

int nltd_read_sync(const char *plugin, loff_t offset, char *buffer, int length)
{
	struct sync_request sync = {
		0,
		COMPLETION_INITIALIZER_ONSTACK(sync.done),
	};

	nltd_read_async(plugin, offset, buffer, length, sync_request_callback, &sync);
	if(wait_for_completion_timeout(&sync.done, msecs_to_jiffies(NLTD_TIMEOUT_MSECS)) == 0)
	{
		return -ETIMEDOUT;
	}

	return sync.ret;
}

int nltd_write_sync(const char *plugin, loff_t offset, char *buffer, int length)
{
	struct sync_request sync = {
		0,
		COMPLETION_INITIALIZER_ONSTACK(sync.done),
	};

	nltd_write_async(plugin, offset, buffer, length, sync_request_callback, &sync);
	wait_for_completion(&sync.done);

	return sync.ret;
}

void clear_timed_out_requests(unsigned long data);

struct timer_list timeout_timer = {
	.function = &clear_timed_out_requests
};

void clear_timed_out_requests(unsigned long data)
{
	int amount = 0;
	struct pending_request *n;
	struct pending_request *request;
	LIST_HEAD(removed);

	spin_lock_irq(&request_lock);
	list_for_each_entry_safe(request, n, &pending_requests, list)
	{
		if(get_jiffies_64()-request->started > msecs_to_jiffies(NLTD_TIMEOUT_MSECS))
		{
			list_del_init(&request->list);
			list_add(&request->list, &removed);
		}
	}
	spin_unlock_irq(&request_lock);

	list_for_each_entry_safe(request, n, &removed, list)
	{
		if(request->callback)request->callback(request->userobject, -ETIMEDOUT);
		kfree(request);
		amount++;
	}

	if(amount)printk(KERN_DEBUG "tDisk: Cleared %d timed out requests\n", amount);

	timeout_timer.expires = get_jiffies_64() + HZ;
	add_timer(&timeout_timer);
}

static struct genl_ops genl_tdisk_ops[] = {
	{
		.cmd = NLTD_CMD_REGISTER,
		.doit = genl_register,
	}, {
		.cmd = NLTD_CMD_UNREGISTER,
		.doit = genl_unregister,
	}, {
		.cmd = NLTD_CMD_FINISHED,
		.doit = genl_finished,
	}
};

int nltd_register()
{
	int ret;

	spin_lock_init(&request_lock);

	ret = genl_register_family_with_ops(&genl_tdisk_family, genl_tdisk_ops);
	if(ret)return ret;

	timeout_timer.expires = get_jiffies_64() + HZ;
	add_timer(&timeout_timer);

	return ret;
}

void nltd_unregister()
{
	struct tdisk_plugin *n;
	struct tdisk_plugin *plugin;
	struct pending_request *n2;
	struct pending_request *request;	
	LIST_HEAD(removed);

	del_timer(&timeout_timer);

	//Clear all registered plugins
	spin_lock_irq(&plugin_lock);
	list_for_each_entry_safe(plugin, n, &registered_plugins, list)
	{
		kfree(plugin);
	}
	spin_unlock_irq(&plugin_lock);

	//Clear all remaining requests
	spin_lock_irq(&request_lock);
	list_for_each_entry_safe(request, n2, &pending_requests, list)
	{
		list_del_init(&request->list);
		list_add(&request->list, &removed);
	}
	spin_unlock_irq(&request_lock);

	list_for_each_entry_safe(request, n2, &removed, list)
	{
		if(request->callback)request->callback(request->userobject, -ETIMEDOUT);
		kfree(request);
	}

	genl_unregister_family(&genl_tdisk_family);
}
