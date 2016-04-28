/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <tdisk/config.h>
#include "tdisk_nl.h"

#ifdef USE_NETLINK

/**
  * Defines the operation SIZE which can be used to
  * get the size of a plugin disk. This is a similar operation
  * as READ or WRITE.
 **/
#define SIZE 65438

/**
  * Controls access to the registered plugins
 **/
spinlock_t plugin_lock;

/**
  * Defines a registered plugin which just has a name
  * and a netlink port
 **/
struct tdisk_plugin
{
	char name[NLTD_MAX_NAME];
	u32 port;
	struct list_head list;
}; //end struct tdisk_plugin

/**
  * The list which holds all registered plugins
 **/
LIST_HEAD(registered_plugins);

/**
  * Is used to synchronize access to the pending
  * requests struct
 **/
spinlock_t request_lock;

/**
  * Internally, each netlink message gets a request
  * counter which is used to identify the message
  * when the answer is received. This variable
  * is incremented each time a message is sent.
 **/
u32 request_counter;

/**
  * This struct holds all information which is needed
  * to continue the work for a request whenever an
  * answer is received for it.
 **/
struct pending_request
{
	/** The type of the message such as READ, WRITE or SIZE **/
	int type;

	/** The sequence number which is used to identify it **/
	u32 seq_nr;

	/** Stores the time in jiffies when the message was sent **/
	u64 started;

	/** This counter is used to time the message out if it takes too long **/
	u32 timeout_counter;

	/** This callback is called when an answer is received **/
	plugin_callback callback;

	/** The private data for the callback **/
	void *userobject;

	/** The buffer for the message which is e.g. needed for read operations **/
	char *buffer;

	/** The length of the buffer **/
	size_t buffer_length;

	struct list_head list;
}; //end struct pending_request

/**
  * Contains all requests which are wainting for an
  * answer from the plugin
 **/
LIST_HEAD(pending_requests);

/**
  * The generic netlink family which is used for communication
 **/
static struct genl_family genl_tdisk_family = {
	.id = GENL_ID_GENERATE,
	.name = NLTD_NAME,
	.hdrsize = NLTD_HEADER_SIZE,
	.version = NLTD_VERSION,
	.maxattr = NLTD_MAX,
};

/**
  * This struct is used for synchronized plugin operations.
  * As all calls are done asynchronously, this struct is
  * needed to wait for the specific event.
 **/
struct sync_request
{
	int ret;
	struct completion done;
}; //end struct sync_request

/**
  * This internal callback is used for synchonous plugin
  * operations. This callback simply wakes up the waiting
  * process and sets the result code.
 **/
void sync_request_callback(void *data, int ret)
{
	struct sync_request *sync = data;
	sync->ret = ret;
	complete(&sync->done);
}

/**
  * This function is called for each NLTD_CMD_REGISTER
  * request to register a plugin in the kernel module.
  * The plugin is stored in the global plugins list
  * using its name and port
 **/
static int genl_register(struct sk_buff *skb, struct genl_info *info)
{
	bool found = false;
	char *name;
	size_t name_length;
	struct tdisk_plugin *plugin;

	if(!info->attrs[NLTD_PLUGIN_NAME])return -EINVAL;
	name = nla_data(info->attrs[NLTD_PLUGIN_NAME]);
	name_length = strlen(name);

	//Check if plugin is already registers
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
		//Add plugin in global list
		plugin = kmalloc(sizeof(struct tdisk_plugin), GFP_KERNEL);
		memcpy(plugin->name, name, NLTD_MAX_NAME);
		plugin->port = info->snd_portid;

		list_add(&plugin->list, &registered_plugins);
	}
	spin_unlock_irq(&plugin_lock);

	printk(KERN_INFO "tDisk: Plugin %s registered\n", name);
	return 0;
}

/**
  * This function is called for each NLTD_CMD_UNREGISTER
  * request to unregister a plugin in the kernel module.
 **/
static int genl_unregister(struct sk_buff *skb, struct genl_info *info)
{
	bool found = false;
	const char *name;
	size_t name_length;
	struct tdisk_plugin *n;
	struct tdisk_plugin *plugin;

	if(!info->attrs[NLTD_PLUGIN_NAME])return -EINVAL;
	name = nla_data(info->attrs[NLTD_PLUGIN_NAME]);
	name_length = strlen(name);

	//Search plugin
	spin_lock_irq(&plugin_lock);
	list_for_each_entry_safe(plugin, n, &registered_plugins, list)
	{
		if(name_length == strlen(name) && strncmp(name, plugin->name, name_length) == 0)
		{
			//Delete plugin from global list
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

/**
  * This function should be called to create a
  * new message. It creates a message while
  * protecting against race conditions.
  * Here also the sequence number is set.
 **/
static struct pending_request* create_request(void)
{
	struct pending_request *req = kzalloc(sizeof(struct pending_request), GFP_KERNEL);
	if(!req)return NULL;

	spin_lock_irq(&request_lock);
	//if(list_empty(&pending_requests) == 0)request_counter = 0;	//Reset counter to prevent overflow -- not needed? Overflow doesnt cause errors...
	req->started = get_jiffies_64();
	req->timeout_counter = 0;
	req->seq_nr = request_counter++;
	list_add_tail(&req->list, &pending_requests);	//Add to global requests list
	spin_unlock_irq(&request_lock);

	return req;
}

/**
  * Removes the request with the given request number
  * from the global list and returns the original
  * structure. It is protected against race conditions.
 **/
static struct pending_request* pop_request(u32 request_number)
{
	struct pending_request *n;
	struct pending_request *request;
	struct pending_request *req = NULL;

	//Lock and search
	spin_lock_irq(&request_lock);
	list_for_each_entry_safe(request, n, &pending_requests, list)
	{
		if(request->seq_nr == request_number)
		{
			//Request found - deleting and stopping
			req = request;
			list_del_init(&request->list);
			break;
		}
	}
	spin_unlock_irq(&request_lock);

	return req;
}

/**
  * Gets the port of the plugin with the given name.
 **/
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

/**
  * Gets the name of the plugin with the given port.
  * The string pointed to by the return value
  * should not be freed.
 **/
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

/**
  * This function is called for every finished command
  * This function checks for consistency of the answer,
  * retrieves the corresponsing message, calls the callback
  * and cleans everything up.
 **/
static int genl_finished(struct sk_buff *skb, struct genl_info *info)
{
	int ret;
	size_t length;
	u32 request_number;
	struct pending_request *req;

	//A message must have a request number, return value and a length
	if(!info->attrs[NLTD_REQ_NUMBER])
	{
		const char *plugin_name = get_plugin_name(info->snd_portid);

		printk(KERN_WARNING "tDisk: Probably broken plugin: %s (%u) - didn't send a request number\n", (plugin_name ? plugin_name : "Unknown"), info->snd_portid);
		return -EINVAL;
	}
	if(!info->attrs[NLTD_REQ_LENGTH])
	{
		const char *plugin_name = get_plugin_name(info->snd_portid);

		printk(KERN_WARNING "tDisk: Probably broken plugin: %s (%u) - didn't send a length\n", (plugin_name ? plugin_name : "Unknown"), info->snd_portid);
		return -EINVAL;
	}
	if(!info->attrs[NLTD_REQ_RET])
	{
		const char *plugin_name = get_plugin_name(info->snd_portid);

		printk(KERN_WARNING "tDisk: Probably broken plugin: %s (%u) - didn't send a return value\n", (plugin_name ? plugin_name : "Unknown"), info->snd_portid);
		return -EINVAL;
	}

	//Get fields from message
	request_number = nla_get_u32(info->attrs[NLTD_REQ_NUMBER]);
	length = (size_t)nla_get_u32(info->attrs[NLTD_REQ_LENGTH]);
	ret = nla_get_s32(info->attrs[NLTD_REQ_RET]);

	if(ret < 0)printk(KERN_DEBUG "tDisk: received error message %d\n", ret);

	//Get original request structure from pending request
	req = pop_request(request_number);
	if(!req)
	{
		const char *plugin_name = get_plugin_name(info->snd_portid);

		printk(KERN_WARNING "tDisk: Plugin %s (%u) finished a request I didn't know of: %u\n", (plugin_name ? plugin_name : "Unknown"), info->snd_portid, request_number);
		return -EINVAL;
	}

	//If the type was not write operation, the
	//data need to be stored back to the original data
	if(req->type != WRITE && length > 0)
	{
		void *data;
		if(!info->attrs[NLTD_REQ_BUFFER])
		{
			const char *plugin_name = get_plugin_name(info->snd_portid);
			printk(KERN_WARNING "tDisk: Probably broken plugin: %s (%u) - type is not WRITE but no buffer was sent.\n", (plugin_name ? plugin_name : "Unknown"), info->snd_portid);
			ret = -EINVAL;
		}
		else
		{
			data = nla_data(info->attrs[NLTD_REQ_BUFFER]);
			memcpy(req->buffer, data, min(length, req->buffer_length));
		}
	}

	//Finally call callback that the request finished
	if(req->callback)
		req->callback(req->userobject, ret);

	kfree(req);
	return 0;
}

/**
  * This function sends a message asynchronously.
  * It allocates a message object, sets all the
  * values and sends the message to the plugin with
  + the given name.
 **/
int nltd_send_async(const char *plugin, loff_t offset, char *buffer, size_t length, int operation, plugin_callback callback, void *userobject)
{
	int err;
	void *hdr;
	struct sk_buff *msg;
	struct pending_request *req = NULL;
	u32 port = get_plugin_port(plugin);
	size_t msg_size = sizeof(__u32) /*u32 -> req_nr*/ +
							sizeof(__u64) /*u64 -> offset*/ +
							sizeof(__u32) /*u32 -> length*/ +
							length /* buffer */;

	if(msg_size < PAGE_SIZE)msg_size = PAGE_SIZE;

	if(length > PAGE_SIZE)printk_ratelimited(KERN_WARNING "tDisk: Warning when sending nl msg: length (%d) is larger than PAGE_SIZE(%lu)\n", length, PAGE_SIZE);

	//0 means plugin is not registered
	err = -ENODEV;
	if(port == 0)goto error;

	//Allocate message
	err = -ENOMEM;
	msg = genlmsg_new(msg_size, 0);
	if(!msg)goto error;

	//Set plugin operation
	err = -ENOBUFS;
	if(operation == READ)
		hdr = genlmsg_put(msg, port, 0, &genl_tdisk_family, 0, NLTD_CMD_READ);
	else if(operation == SIZE)
		hdr = genlmsg_put(msg, port, 0, &genl_tdisk_family, 0, NLTD_CMD_SIZE);
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

	//Create request in request list and set values
	//This is needed to identify the answer of the
	//message later on
	err = -ENOMEM;
	req = create_request();
	if(!req)goto nlmsg_failure;

	req->type = operation;
	req->callback = callback;
	req->userobject = userobject;
	req->buffer = buffer;
	req->buffer_length = length;

	err = nla_put_u32(msg, NLTD_REQ_NUMBER, req->seq_nr);
	if(err)
	{
		printk(KERN_WARNING "tDisk: Error adding parameter seq_nr to message\n");
		goto nlmsg_failure;
	}

	err = nla_put_u64(msg, NLTD_REQ_OFFSET, (u64)offset);
	if(err)
	{
		printk(KERN_WARNING "tDisk: Error adding parameter offset to message\n");
		goto nlmsg_failure;
	}

	err = nla_put_u32(msg, NLTD_REQ_LENGTH, (u32)length);
	if(err)
	{
		printk(KERN_WARNING "tDisk: Error adding parameter length to message\n");
		goto nlmsg_failure;
	}

	if(operation != READ)
	{
		err = nla_put(msg, NLTD_REQ_BUFFER, (int)length, buffer);
		if(err)
		{
			printk(KERN_WARNING "tDisk: Error adding parameter buffer to message\n");
			goto nlmsg_failure;
		}
	}

	//Send message
	genlmsg_end(msg, hdr);
	err = genlmsg_unicast(&init_net, msg, port);
	if(err)goto nlmsg_failure;
	return 0;

 nlmsg_failure:
	genlmsg_cancel(msg, hdr);
	kfree_skb(msg);
 error:
	printk(KERN_WARNING "tDisk: Error sending netlink message: %d. Operation: %s, Offset: %llu, Length: %d\n",
			err,
			(operation == READ ? "READ" : (operation == WRITE ? "WRITE" : (operation == SIZE ? "SIZE" : "UNKNOWN"))),
			offset,
			length);

	//In an error case the callback needs to be called
	//anyways to prevent waiting for the timeout
	if(callback)callback(userobject, err);

	if(req != NULL)
	{
		//Obviously the request also needs to be removed
		//from the global requests list.
		pop_request(req->seq_nr);
		kfree(req);
	}

	return err;
}

void nltd_read_async(const char *plugin, loff_t offset, char *buffer, size_t length, plugin_callback callback, void *userobject)
{
	nltd_send_async(plugin, offset, buffer, length, READ, callback, userobject);
}

void nltd_write_async(const char *plugin, loff_t offset, char *buffer, size_t length, plugin_callback callback, void *userobject)
{
	nltd_send_async(plugin, offset, buffer, length, WRITE, callback, userobject);
}

int nltd_read_sync(const char *plugin, loff_t offset, char *buffer, size_t length)
{
	size_t pos;

	for(pos = 0; pos < length; pos += PAGE_SIZE)
	{
		size_t currentLength = min(PAGE_SIZE, length-pos);

		struct sync_request sync = {
			0,
			COMPLETION_INITIALIZER_ONSTACK(sync.done),
		};

		nltd_read_async(plugin, offset+pos, buffer+pos, currentLength, sync_request_callback, &sync);
		wait_for_completion(&sync.done);

		if(sync.ret < 0)return -EIO;
	}

	return 0;
}

int nltd_write_sync(const char *plugin, loff_t offset, char *buffer, size_t length)
{
	size_t pos;

	for(pos = 0; pos < length; pos += PAGE_SIZE)
	{
		size_t currentLength = min(PAGE_SIZE, length-pos);

		struct sync_request sync = {
			0,
			COMPLETION_INITIALIZER_ONSTACK(sync.done),
		};

		nltd_write_async(plugin, offset+pos, buffer+pos, currentLength, sync_request_callback, &sync);
		wait_for_completion(&sync.done);

		if(sync.ret < 0)return -EIO;
	}

	return 0;
}

loff_t nltd_get_size(const char *plugin)
{
	int ret;
	loff_t size = 0;

	struct sync_request sync = {
		0,
		COMPLETION_INITIALIZER_ONSTACK(sync.done),
	};

	ret = nltd_send_async(plugin, 0, (char*)&size, sizeof(loff_t), SIZE, sync_request_callback, &sync);
	if(ret)return 0;

	wait_for_completion(&sync.done);

	if(sync.ret < 0)return 0;
	return size;
}

/**
  * This function is called in a regular interval
  * to stop all requests which are waiting for longer
  * than NLTD_TIMEOUT_SECS seconds for an answer
  * This assures that every request is completed - 
  * with or without error
 **/
void clear_timed_out_requests(unsigned long data);

/**
  * This timer is set every second to check for
  * requests which are running too long
 **/
struct timer_list timeout_timer = {
	.function = &clear_timed_out_requests
};

void clear_timed_out_requests(unsigned long data)
{
	int amount = 0;
	struct pending_request *n;
	struct pending_request *request;

	//This is used as temporary storage. All requests
	//to time out are transferred to this list to prevent
	//holding the spinlock too long
	LIST_HEAD(removed);

	//go through each request
	spin_lock_irq(&request_lock);
	list_for_each_entry_safe(request, n, &pending_requests, list)
	{
		if(request->timeout_counter++ >= NLTD_TIMEOUT_SECS)
		{
			list_del_init(&request->list);
			list_add(&request->list, &removed);
		}
	}
	spin_unlock_irq(&request_lock);

	//Time out all requests
	list_for_each_entry_safe(request, n, &removed, list)
	{
		unsigned long time = jiffies_to_msecs((unsigned long)request->started);
		printk(KERN_DEBUG "tDisk: Timing out request %u: start-time: %us %ums\n", request->seq_nr, time/1000, (time%1000));
		if(request->callback)request->callback(request->userobject, -ETIMEDOUT);
		kfree(request);
		amount++;
	}

	if(amount)printk(KERN_WARNING "tDisk: Cleared %d timed out requests\n", amount);

	//Re-set timer
	timeout_timer.expires = jiffies + HZ;
	add_timer(&timeout_timer);
}

/**
  * Defines all message types which are handled in
  * the kernel module.
 **/
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

	timeout_timer.expires = jiffies + HZ;
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

#else
#pragma message "Netlink is disabled"
#endif //USE_NETLINK
