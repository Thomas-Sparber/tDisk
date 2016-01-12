#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tdisk.h>

#define DECLARE_OPERATION(op) {#op,op}

enum {
	add,
	delete,
	add_disk,
	clear
};

struct
{
	char *operation;
	int enum_type;
} operations[] = {
	DECLARE_OPERATION(add),
	DECLARE_OPERATION(delete),
	DECLARE_OPERATION(add_disk),
	DECLARE_OPERATION(clear)
};

int operations_count = sizeof(operations) / sizeof(operations[0]);

int print_error(int ret);
int is_error(int ret);
void print_usage(void);
int get_operation_type(const char *operation);

int main(int argc, char* args[])
{
	char name[256];
	int disk;
	int ret = 0;
	int i;

	if(argc <= 1)
	{
		fprintf(stderr, "Invalid number of arguments\n\n");
		print_usage();
		return 1;
	}

	switch(get_operation_type(args[1]))
	{
	case add:
		if(argc > 2)tdisk_add_specific(name, atoi(args[2]));
		else disk = tdisk_add(name);

		if(print_error(disk))
		{
			ret = 1;
			break;
		}

		printf("Device %s opened\n", name);

		break;
	case delete:
		if(argc <= 2)
		{
			fprintf(stderr, "Error: \"delete\" needs the minor device number, e.g \"/dev/td0\" --> delete 0\n");
			ret = 1;
			break;
		}

		ret = tdisk_remove(atoi(args[2]));
		if(!print_error(ret))printf("Device %s closed\n", args[2]);

		break;
	case add_disk:
		if(argc <= 3)
		{
			fprintf(stderr, "Error: \"add_disk\" needs the tDisk and the file to add\n");
			ret = 1;
			break;
		}

		for(i = 3; i < argc; ++i)
		{
			ret = tdisk_add_disk(args[2], args[i]);
			if(print_error(ret))break;
		}

		printf("%d device%s added successfully\n", argc-3, (argc==4) ? "" : "s");

		break;
	case clear:
		if(argc <= 2)
		{
			fprintf(stderr, "Error: \"clear\" needs the td device\n");
			ret = 1;
			break;
		}

		ret = tdisk_clear(args[2]);
		if(!print_error(ret))printf("Device %s cleared\n", args[2]);

		break;
	default:
		fprintf(stderr, "Error: Invalid argument %s\n", args[1]);
	}

	return ret;
}

int print_error(int ret)
{
	if(is_error(ret))
	{
		switch(ret)
		{
		case -EDRVNTLD:
			fprintf(stderr, "Error: Driver not loaded\n");
			break;
		case -ENOPERM:
			fprintf(stderr, "Error: Insufficient permissions to control td devices\n");
			break;
		default:
			fprintf(stderr, "Error: Unknown error %d\n", ret);
		}

		return 1;
	}

	return 0;
}

int is_error(int ret)
{
	return (ret < 0);
}

void print_usage(void)
{

}

int get_operation_type(const char *operation)
{
	int i;

	for(i = 0; i < operations_count; ++i)
	{
		if(strcasecmp(operations[i].operation, operation) == 0)
		{
			return operations[i].enum_type;
		}
	}

	return -1;
}
