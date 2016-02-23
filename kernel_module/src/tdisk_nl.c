#include <linux/skbuff.h>
#include <linux/genetlink.h>
#include <linux/list.h>
#include <net/genetlink.h>
#include <net/sock.h>

#include "tdisk_nl.h"

struct tdisk_plugin
{
	char name[NLTD_MAX_NAME];
	u32 port;
	struct list_head list;
}; //end struct tdisk_plugin

LIST_HEAD(registered_plugins);

static struct genl_family genl_tdisk_family = {
	.id = GENL_ID_GENERATE,
	.name = NLTD_NAME,
	.hdrsize = NLTD_HEADER_SIZE,
	.version = NLTD_VERSION,
	.maxattr = NLTD_MAX,
};

//static struct genl_multicast_group genl_tdisk_multicast = {
//	.name		= NLTD_NAME "Multicast",
//};

static int genl_finished(struct sk_buff *skb, struct genl_info *info)
{
	
}

static int genl_register(struct sk_buff *skb, struct genl_info *info)
{
	const char *name;
	struct tdisk_plugin *plugin;

	if(!info->attrs[NLTD_PLUGIN_NAME])return -EINVAL;
	name = nla_data(info->attrs[NLTD_PLUGIN_NAME]);

	plugin = kmalloc(sizeof(struct tdisk_plugin), GFP_KERNEL);
	memcpy(plugin->name, name, NLTD_MAX_NAME);
	plugin->port = info->snd_portid;

	list_add(&plugin->list, &registered_plugins);
	printk(KERN_INFO "tDisk: Plugin %s registered\n", name);
	return 0;
}

static int genl_unregister(struct sk_buff *skb, struct genl_info *info)
{
	const char *name;
	struct tdisk_plugin *n;
	struct tdisk_plugin *plugin;

	if(!info->attrs[NLTD_PLUGIN_NAME])return -EINVAL;
	name = nla_data(info->attrs[NLTD_PLUGIN_NAME]);

	list_for_each_entry_safe(plugin, n, &registered_plugins, list)
	{
		if(info->snd_portid == plugin->port && strncmp(name, plugin->name, NLTD_MAX_NAME) == 0)
		{
			list_del(&plugin->list);
			kfree(plugin);
			printk(KERN_INFO "tDisk: Plugin %s unregistered\n", name);
			return 0;
		}
	}

	printk(KERN_WARNING "tDisk: Unable to unregister plugin %s. Not registered\n", name);

	return -EINVAL;
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

	ret = genl_register_family_with_ops(&genl_tdisk_family, genl_tdisk_ops);
	if(ret)return ret;

	return ret;
}

void nltd_unregister()
{
	struct tdisk_plugin *n;
	struct tdisk_plugin *plugin;

	list_for_each_entry_safe(plugin, n, &registered_plugins, list)
	{
		kfree(plugin);
	}

	genl_unregister_family(&genl_tdisk_family);
}
