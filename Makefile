
all: make_ko make_frontend make_tests make_plugins web_interface/tdisk

make_ko:
	make -C kernel_module

make_frontend:
	make -C frontend

make_tests:
	make -C tests

make_plugins:
	make -C plugins

disks:
	sudo dd if=/dev/zero of=slow_disk bs=512 count=1024
	sudo dd if=/dev/zero of=big_disk bs=1M count=10
	sudo dd if=/dev/zero of=other_disk bs=1M count=5

web_interface/tdisk: frontend/tdisk
	cp frontend/tdisk web_interface/tdisk

doc:
	doxygen documentation/kernel_module/doxygen.conf
	doxygen documentation/frontend/doxygen.conf

clean:
	make -C kernel_module clean
	make -C frontend clean
	make -C tests clean
	make -C plugins clean
	rm web_interface/tdisk
