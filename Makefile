
all: make_ko make_backend

make_ko:
	@make -C kernel_module

make_backend:
	@make -C backend

disks:
	sudo dd conv=notrunc if=/dev/zero of=slow_disk bs=1M count=10
	sudo dd conv=notrunc if=/dev/zero of=big_disk bs=1M count=10
	sudo dd conv=notrunc if=/dev/zero of=other_disk bs=1M count=10

doc:
	@doxygen documentation/kernel_module/doxygen.conf
	@doxygen documentation/backend/doxygen.conf

count_lines:
	@git ls-files | \
		grep -v ".*zip$$" | \
		grep -v ".*docx$$" | \
		grep -v ".*pdf$$" | \
		grep -v ".*docx\"$$" | \
		grep -v ".*pdf\"$$" | \
		grep -v ".*xlsx$$" | \
		grep -v ".*odp$$" | \
		grep -v ".*svg$$" | \
		grep -v ".*ico$$" | \
		grep -v ".*png$$" | \
		grep -v ".*doxygen.conf$$" | \
		grep -v ".*/typings/.*" | \
		grep -v ".*/browser.min.js$$" | \
		grep -v ".*/jquery-2.2.1.min.js$$" | \
		grep -v ".*/react-0.14.6.js$$" | \
		grep -v "documentation.*" | \
		grep -v "Business.*" | \
		grep -v ".*/react-dom-0.14.6.js$$" | xargs -d '\n' wc -l

clean:
	@make -C kernel_module clean
	@make -C backend clean
