
all: make_ko make_frontend

make_ko:
	make -C kernel_module

make_frontend:
	make -C frontend

clean:
	make -C kernel_module clean
	make -C frontend clean
