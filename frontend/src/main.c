#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tdisk.h>

#define DECLARE_OPERATION(op) {#op,op}

enum {
	add,
	delete,
	add_disk,
	clear,
	get_max_sectors,
	get_sector_index,
	get_all_sector_indices
};

struct
{
	char *operation;
	int enum_type;
} operations[] = {
	DECLARE_OPERATION(add),
	DECLARE_OPERATION(delete),
	DECLARE_OPERATION(add_disk),
	DECLARE_OPERATION(clear),
	DECLARE_OPERATION(get_max_sectors),
	DECLARE_OPERATION(get_sector_index),
	DECLARE_OPERATION(get_all_sector_indices)
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
	case get_max_sectors:
		do
		{
			__u64 max_sectors;
			ret = tdisk_get_max_sectors(args[2], &max_sectors);
			if(!print_error(ret))printf("%llu\n", max_sectors);
		}
		while(0);
		break;
	case get_sector_index:
		if(argc <= 2)
		{
			fprintf(stderr, "Error: \"get_sector_index\" needs the sector index to retrieve\n");
			ret = 1;
			break;
		}

		do
		{
			struct sector_index index;
			ret = tdisk_get_sector_index(args[2], strtoull(args[3], NULL, 10), &index);
			if(!print_error(ret))printf("Disk: %u\nSector: %llu\nAccess_count: %u\n", index.disk, index.sector, index.access_count);
		}
		while(0);

		break;
	case get_all_sector_indices:
		do
		{
			__u64 max_sectors;
			ret = tdisk_get_max_sectors(args[2], &max_sectors);
			if(print_error(ret))break;
			struct sector_index *indices = malloc(sizeof(struct sector_index) * (size_t)max_sectors);
			ret = tdisk_get_all_sector_indices(args[2], indices);
			if(!print_error(ret))
			{
				__u64 j;
				for(j = 0; j < max_sectors; ++j)
				{
					printf("Disk: %u\nSector: %llu\nAccess_count: %u\n\n", indices[j].disk, indices[j].sector, indices[j].access_count);
				}
			}
		}
		while(0);
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
