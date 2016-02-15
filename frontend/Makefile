CC=gcc
CXX=g++
CFLAGS=-c -g -Wall -Wextra -I../kernel_module/include -I./include -Wdouble-promotion -Wuninitialized -fipa-pure-const -Wtrampolines -Wfloat-equal  -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wredundant-decls -Wshadow -Wint-to-pointer-cast
CPPFLAGS=-std=c++11 -Weffc++
LDFLAGS=-L. -ltd-frontend
SOURCES= \
	src/tdisk.c \
	src/frontend_c.c

CPP_SOURCES= \
	src/configfile.cpp \
	src/deviceadvisor.cpp \
	src/frontend.cpp
	

OBJECTS=$(SOURCES:src/%.c=bin/%.o) $(CPP_SOURCES:src/%.cpp=bin/%.o)
DEPS=$(OBJECTS:bin/%.o=bin/%.d)

all: tdisk

-include $(DEPS)

libtd-frontend.a: $(OBJECTS)
	@echo "Linking" $@
	@ar rcs $@ $(OBJECTS)

tdisk: libtd-frontend.a bin/main.o
	@echo "Linking" $@
	@$(CXX) -o $@ bin/main.o $(LDFLAGS)

bin/%.o: src/%.c bin
	@echo "Compiling" $<
	@$(CC) -MD $(CFLAGS) -o $@ $<

bin/%.o: src/%.cpp bin
	@echo "Compiling" $<
	@$(CXX) -MD $(CFLAGS) $(CPPFLAGS) -o $@ $<

bin:
	mkdir bin

clean:
	@echo "Cleaning"
	@rm -rf bin/*.o bin/*.d tdisk libtd-frontend.a
