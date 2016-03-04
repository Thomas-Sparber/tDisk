
all: make_ko make_backend web_interface/tdisk

make_ko:
	@make -C kernel_module

make_backend:
	@make -C backend

disks:
	sudo dd if=/dev/zero of=slow_disk bs=512 count=1024
	sudo dd if=/dev/zero of=big_disk bs=1M count=10
	sudo dd if=/dev/zero of=other_disk bs=1M count=5

web_interface/tdisk: backend/tdisk
	@cp backend/tdisk web_interface/tdisk

doc:
	@doxygen documentation/kernel_module/doxygen.conf
	@doxygen documentation/backend/doxygen.conf

clean:
	@make -C kernel_module clean
	@make -C backend clean
	@rm -f web_interface/tdisk
