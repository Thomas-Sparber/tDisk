# tDisk
A linux kernel module for high performance multi-tier storage with additional features

# Features

## Base features
tDisk combines multiple disks/partitions/files into one big disk with the capacity of (almost) the sum of all disks.
tDisks performs some internal measures to optimize the performance of the disk.
tDisk is not meant for redundancy but for performance improvement but of course it is possible to combine tDisk e.g. with mdadm to create a high performance redundant disk.
tDisk also contains a plugin environment (which is executed in user space) which allows to use any kind of storage as tDisk, e.g. a Dropbox plugin was created to use the dropbox cloud storage with a tDisk

## Sector swapping
tDisk internally counts the number of usage per sector. If a sector is more used than an other one it is moved to a disk with better performance.
To accomplish this a kernel thread is executed when the disk is idle to re-arrange/move the sectors according to their usage

## Initial optimization
The process of finding an optimal sector at first write is called "initial optimization". Immagine the following: A new tDisk is created and therefore does not containn any data.
So if data is written to the disk it doesn't matter to which physical position, because the entire disk is empty.
If the filsystem wants to write data to the tDisk to a slow underlying device, this specific sector is swapped with a sector lying on a high performance disk for optimal performance.
Of course this optimization can only be performed if there is free space on the tDisk

## Caching
Actually this is a combination of the two optimizations above: tDisk always tries to keep some free space on the underlying disk with the best performance.
In order to accomplish this, sectors are moved to lower performing disks during idle time.
This makes it possible that there is always some high performance free space in case some date need to be stored.

## Plugins
Plugins are executed in userspace and tDisk uses netlink to communicate with them. A plugin acts like a physical disk which can be added to a tDisk.
There are two plugins in the repository:
 - blackhole which just acts as a "fake" device. This can be used e.g. to measure performance to userspace
 - dropbox allows the connection to a dropbox account. The data is stored in several files. For testing purposes I created a tDisk with one physical disk and one dropbox disk. The physical disk obviously was used for performance so that e.g. the filesystem, could be stored there. the dropbox "disk" was used for storage. Just for fun, a tDisk with a capacity of 1 EB was created and was actuall usable :-)

# Building
The kernel module is currently supported on linux kernels 3.19 - 4.8.
But I tried to build it on a 4.14 linux kernel which only resulted in some minor compilation errors which shouldn't be hard to fix.
Maybe I will fix it in the near future.

The userspace backend needs the following packages as requirement:
 - build-essential
 - libext2fs-dev 
 - linux-headers
 - libparted-dev
 - libnl-3-dev
 - libnl-genl-3-dev
 - libcurl4-openssl-dev
 - libjsoncpp-dev

In order to build the kernel module and the userspace backend you can just type `make` at the root folder.

# Usage

To load the kernel driver, type `insmod kernel_module/tdisk_tools.ko` and then the main driver `insmod kernel_module/tdisk.ko`. You can type `dmesg` to see if the driver loaded properly.

After the module is loaded you can create a new tdisk using the userspace backend. Here I will demonstrate how to create a tDisk using two files as underlying devices:

    cd backend
    dd if=/dev/zero of=small-disk bs=1M count=10 #first disk, 10MiB
    dd if=/dev/zero of=big-disk bs=1M count=1000 # second disk, 1GiB
    ./tdisk add 16384 #block size of tDisk. 16384 is suitable for such a small disk but probably too small for an actual disk
    ./tdisk add_disk /dev/td0 small-disk
    ./tdisk add_disk /dev/td0 big-disk

Now the tDisk is created as a combination of the two files. Now we can proceed on creating a filesystem and actually mounting the disk

    mkfs.ext4 /dev/td0
    mkdir /media/tdisk
    mount /dev/td0 /media/tdisk

Now the tDisk is mounted under /media/tdisk and can be used as any regular disk. In a real workl scenario you would not use a small file, but an actual disk e.g. `/dev/sda` or maybe just a partition `/dev/sda1` *ATTENTION* All data will be removed on the disk once added to the tDisk!!

Here is the full list of arguments supported by the user space backend:

```
Usage: ./tdisk ([options]) [command] [command-arguments...]

The following options are available:
         --output-format=[text,json]
           Defines the output format. This can either be "text" or "json"
         --configfile=...
           The file where all configuration data should be read/written.
         --config-only=[no,yes]
           A flag whether the operations should be writte to config file only.
           If this flag is set to yes, nothing is done on the actual system
         --temporary=[no,yes]
           A flag whether the operations should only be done temporary. This
           means the actions are performed, but not stored to any config file.

The following commands are available:
         - get_tdisks
           Prints a list of all available tDisks
         - get_tdisk
           Prints details about the given tDisk. It needs the minornumber or
           path as argument
         - get_devices
           Prints all available devices which could be used for a tDisk.
         - create
           Creates a new tDisk. It needs the blocksize (e.g. 16386) and all
           the desired devices (in this order) as arguments. It is also possible
           possible to specify the minornumber: [minornumber] [blocksize] ...
         - add
           Adds a new tDisk to the system. It needs the blocksize (e.g. 16386)
           as argument. The next free minornumber is taken. It is also possible
           possible to specify the minornumber: [minornumber] [blocksize]
         - remove
           Removes the given tDisk from the system. It needs the minornumber or
           path as argument
         - add_disk
           Adds a new internal device to the tDisk. If it is a file path it is
           added as file. If not it is added as a plugin. It needs the tDisk
           minornumber/path and filename/pluginname as argument
         - remove_disk
           Removes the given internal device from the tDisk.  It needs the
           tDisk minornumber/path and device id as argument
         - get_max_sectors
           Gets the current maximum amount of sectors for the given tDisk. It
           needs the tDisk minornumber/path as argument
         - get_size_bytes
           Gets the size in bytes for the given tDisk. It needs the tDisk
           minornumber/path as argument
         - get_sector_index
           Gets information about the given sector index. It needs the tDisk
           minornumber/path and logical sector (0-max_sectors) as argument
         - get_all_sector_indices
           Returns infomration about all sector indices. It needs the tDisk
           minornumber/path as argument
         - clear_access_count
           Resets the access count of all sectors. It needs the tDisk
           minornumber/path as argument
         - get_internal_devices_count
           Gets the amount of internal devices for the given tDisk. It needs
           the tDisk minornumber/path as argument
         - get_device_info
           Gets device information of the device with the given id. It needs
           the tDisk minornumber/path and device id as argument
         - get_debug_info
           Gets debug information about the given tDisk. It needs the tDisk
           minornumber/path and an optional current debug id to get the next
           info
         - load_config_file
           Loads the given config file. It needs the path to the config file as
           argument
         - get_device_advice
           Returns a device on how to configure a given list of devices
           optimally. It needs all desired devices as agrument
         - get_files_at
           Returns all files which are located on the given disk at the given
           position. It needs the device, start and end bytes as argument
         - get_files_on_disk
           Returns all files which are located on the given internal device of
           the given tDisk. It needs the tDisk, and device index as argument
         - tdisk_post_create
           Runs the tDisk post create script which creates a filesystem,
           mounts it, creates a data folder (to hide lost+found) and prints
           the mount point. It needs the desired tDisk as argument
         - tdisk_pre_remove
           Runs the tDisk pre remove script which just unmounts the tDisk.
           It needs the desired tDisk as argument
         - performance_improvement
           Returns the performance improvement in percent of the tDisk and
           lists the top 20 files with the highest performance improvement.
           It needs the desired tDisk as argument
         - get_internal_device_usage
           Returns the dis usage in percent of the internal devices. It needs
           the desired tDisk as argument
```

# Contribution / Collaboration
If you want to contribute to this project please let me know - I am happy about every input!
