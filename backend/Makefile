CC=gcc
CXX=g++
CFLAGS=-c -g -Wall -Wextra -I../kernel_module/include -I./include -I./utils -Wdouble-promotion -Wuninitialized -fipa-pure-const -Wtrampolines -Wfloat-equal  -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wredundant-decls -Wshadow -Wint-to-pointer-cast
CPPFLAGS=-std=c++11 -Weffc++
LDFLAGS=-L. -ltd-backend
SOURCES= \
	src/tdisk.c \
	src/tdisk_fake.c \
	src/backend_c.c

CPP_SOURCES= \
	src/configfile.cpp \
	src/device.cpp \
	src/deviceadvisor.cpp \
	src/filesystem.cpp \
	src/backend.cpp \
	src/option.cpp \
	src/options.cpp \
	src/tdisk_advice.cpp


OBJECTS=$(SOURCES:src/%.c=bin/%.o) $(CPP_SOURCES:src/%.cpp=bin/%.o)
DEPS=$(OBJECTS:bin/%.o=bin/%.d)

all: bin tdisk

ifeq ($(OS),Windows_NT)
else
	LDFLAGS+= -lparted
	all+= make_plugins make_tests
endif

-include $(DEPS)

libtd-backend.a: $(OBJECTS)
	@echo "Linking" $@
	@ar rcs $@ $(OBJECTS)

tdisk: libtd-backend.a bin/main.o
	@echo "Linking" $@
	@$(CXX) -o $@ bin/main.o $(LDFLAGS)

bin/%.o: src/%.c
	@echo "Compiling" $<
	@$(CC) -MD $(CFLAGS) -o $@ $<

bin/%.o: src/%.cpp
	@echo "Compiling" $<
	@$(CXX) -MD $(CFLAGS) $(CPPFLAGS) -o $@ $<

bin:
	mkdir bin

make_plugins:
	make -C plugins

make_tests:
	make -C tests

clean:
	@echo "Cleaning"
	@rm -rf bin/*.o bin/*.d tdisk libtd-backend.a
	@make -C plugins clean
	@make -C tests clean
