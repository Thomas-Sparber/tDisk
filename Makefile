
all: make_ko make_frontend web_interface/tdisk

make_ko:
	make -C kernel_module

make_frontend:
	make -C frontend

web_interface/tdisk: frontend/tdisk
	cp frontend/tdisk web_interface/tdisk

doc:
	doxygen documentation/kernel_module/doxygen.conf
	doxygen documentation/frontend/doxygen.conf
	make -C documentation/kernel_module/latex
	make -C documentation/frontend/latex
clean:
	make -C kernel_module clean
	make -C frontend clean
