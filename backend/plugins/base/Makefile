CC=gcc
CXX=g++
CFLAGS=-c -O3 -fPIC -Wall -Wextra  -I./include -I../../utils -I../../../kernel_module/include -isystem/usr/include/libnl3 -Wdouble-promotion -Wuninitialized -fipa-pure-const -Wtrampolines -Wfloat-equal  -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wredundant-decls -Wshadow -Wint-to-pointer-cast
CPPFLAGS=-std=c++11 -Weffc++
SOURCES=
LDFLAGS=

CPP_SOURCES= \
	src/plugin.cpp
	

OBJECTS=$(SOURCES:src/%.c=bin/%.o) $(CPP_SOURCES:src/%.cpp=bin/%.o)
DEPS=$(OBJECTS:bin/%.o=bin/%.d)

all: bin libplugin-base.so

-include $(DEPS)

libplugin-base.so: $(OBJECTS)
	@echo "Linking" $@
	@$(CXX) -shared -o $@ $(OBJECTS) $(LDFLAGS)

bin/%.o: src/%.c
	@echo "Compiling" $<
	@$(CC) -MD $(CFLAGS) -o $@ $<

bin/%.o: src/%.cpp
	@echo "Compiling" $<
	@$(CXX) -MD $(CFLAGS) $(CPPFLAGS) -o $@ $<

bin:
	mkdir bin

clean:
	@echo "Cleaning"
	@rm -rf bin/*.o bin/*.d libplugin-base.so
