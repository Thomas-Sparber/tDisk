CC=gcc
CXX=g++
CFLAGS=-c -O3 -Wall -Wextra  -I./include -I../utils -I../include -Wdouble-promotion -Wuninitialized -fipa-pure-const -Wtrampolines -Wfloat-equal  -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wredundant-decls -Wshadow -Wint-to-pointer-cast
CPPFLAGS=-std=c++11 -Weffc++
LDFLAGS=-Wl,-rpath '-Wl,$$ORIGIN' -L../ -ltd-backend
SOURCES=

CPP_SOURCES= \
	src/fileshare.cpp \
	src/fileshare_commands.cpp \
	src/main.cpp
	

OBJECTS=$(SOURCES:src/%.c=bin/%.o) $(CPP_SOURCES:src/%.cpp=bin/%.o)
DEPS=$(OBJECTS:bin/%.o=bin/%.d)

all: bin fileshare

-include $(DEPS)

fileshare: $(OBJECTS)
	@echo "Linking" $@
	@$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

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
	@rm -rf bin/*.o bin/*.d fileshare
