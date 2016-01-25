
all: make_ko make_frontend web_interface/tdisk

make_ko:
	make -C kernel_module

make_frontend:
	make -C frontend

web_interface/tdisk: frontend/tdisk
	cp frontend/tdisk web_interface/tdisk

clean:
	make -C kernel_module clean
	make -C frontend clean
