CC=gcc
CXX=g++
CFLAGS=-c -O3 -Wall -Wextra  -I./include -I../utils -I../include -I../libs/wmi/include -Wdouble-promotion -Wuninitialized -fipa-pure-const -Wtrampolines -Wfloat-equal  -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wredundant-decls -Wshadow -Wint-to-pointer-cast -Wno-strict-aliasing
CPPFLAGS=-std=c++11 -Weffc++
LDFLAGS=-lsqlite3 -lz
SOURCES=
DEPENDENCIES=

CPP_SOURCES= \
	src/serialport.cpp \
	src/serialport_linux.cpp \
	src/serialport_windows.cpp \
	src/main.cpp
	

OBJECTS=$(SOURCES:src/%.c=bin/%.o) $(CPP_SOURCES:src/%.cpp=bin/%.o)
DEPS=$(OBJECTS:bin/%.o=bin/%.d)

all: bin serialport

-include $(DEPS)

ifeq ($(OS),Windows_NT)
	DEPENDENCIES+= diaa_sami_comsupp.dll wmi.dll
	LDFLAGS+= -lole32 -loleaut32 -lwbemuuid -L. -ldiaa_sami_comsupp -lwmi
endif

serialport: $(OBJECTS) $(DEPENDENCIES)
	@echo "Linking" $@
	@$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

bin/%.o: src/%.c
	@echo "Compiling" $<
	@$(CC) -MD $(CFLAGS) -o $@ $<

bin/%.o: src/%.cpp
	@echo "Compiling" $<
	@$(CXX) -MD $(CFLAGS) $(CPPFLAGS) -o $@ $<

diaa_sami_comsupp.dll:
	@ln -s ../libs/wmi/diaa_sami_comsupp/$@ .

wmi.dll:
	@ln -s ../libs/wmi/$@ .

bin:
	mkdir bin

clean:
	@echo "Cleaning"
	@rm -rf bin/*.o bin/*.d serialport diaa_sami_comsupp.dll wmi.dll
