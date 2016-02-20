CC=gcc
CXX=g++
CFLAGS=-c -g -Wall -Wextra  -I./include -isystem../libs/liboauthcpp/include/ -isystem../libs/jsoncpp/include -Wdouble-promotion -Wuninitialized -fipa-pure-const -Wtrampolines -Wfloat-equal  -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wredundant-decls -Wshadow -Wint-to-pointer-cast
CPPFLAGS=-std=c++11 -Weffc++
LDFLAGS=-lcurl -L../libs/liboauthcpp -loauthcpp -L../libs/jsoncpp/src/lib_json -ljsoncpp
SOURCES=

CPP_SOURCES= \
	src/dropbox.cpp
	

OBJECTS=$(SOURCES:src/%.c=bin/%.o) $(CPP_SOURCES:src/%.cpp=bin/%.o)
DEPS=$(OBJECTS:bin/%.o=bin/%.d)

all: bin dropbox

-include $(DEPS)

dropbox: $(OBJECTS) bin/main.o
	@echo "Linking" $@
	@$(CXX) -o $@ bin/main.o $(OBJECTS) $(LDFLAGS)

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
	@rm -rf bin/*.o bin/*.d tdisk libtd-frontend.a
