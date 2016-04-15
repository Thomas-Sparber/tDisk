for number in {0..100000}; do sudo dd if=/dev/zero of=/media/cdrom0/testfile bs=512 count=1; echo 59742; echo 3 | sudo tee /proc/sys/vm/drop_caches; done
