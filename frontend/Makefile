CC=gcc
CFLAGS=-c -g -Wall -Wextra -I../kernel_module/include -I./include -Wdouble-promotion -Wuninitialized -fipa-pure-const -Wtrampolines -Wfloat-equal  -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wredundant-decls -Wshadow -Wint-to-pointer-cast
LDFLAGS=
SOURCES= \
	src/main.c \
	src/tdisk.c

OBJECTS=$(SOURCES:src/%.c=bin/%.o)
DEPS=$(SOURCES:src/%.c=bin/%.d)

all: tdisk

-include $(DEPS)

documentation:
	doxygen doc/doxygen.conf
	cd doc/latex && make

tdisk: $(OBJECTS)
	@echo "Linking" $@
	@$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

bin/%.o: src/%.c bin
	@echo "Compiling" $<
	@$(CC) -MD $(CFLAGS) -o $@ $<

bin:
	mkdir bin

clean:
	@echo "Cleaning"
	@rm -rf bin/*.o bin/*.d tdisk
