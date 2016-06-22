CC=gcc
CXX=g++
CFLAGS=-c -O3 -Wall -Wextra  -I./include -I../utils -I../include -Wdouble-promotion -Wuninitialized -fipa-pure-const -Wtrampolines -Wfloat-equal  -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wredundant-decls -Wshadow -Wint-to-pointer-cast
CPPFLAGS=-std=c++11 -Weffc++
LDFLAGS=-lole32 -loleaut32 -lwbemuuid -L. -ldiaa_sami_comsupp -L../ -ltd-backend
SOURCES=

CPP_SOURCES= \
	src/serialport_linux.cpp \
	src/serialport_windows.cpp \
	src/main.cpp
	

OBJECTS=$(SOURCES:src/%.c=bin/%.o) $(CPP_SOURCES:src/%.cpp=bin/%.o)
DEPS=$(OBJECTS:bin/%.o=bin/%.d)

all: bin serialport

-include $(DEPS)

serialport: $(OBJECTS) diaa_sami_comsupp.dll
	@echo "Linking" $@
	@$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

bin/%.o: src/%.c
	@echo "Compiling" $<
	@$(CC) -MD $(CFLAGS) -o $@ $<

bin/%.o: src/%.cpp
	@echo "Compiling" $<
	@$(CXX) -MD $(CFLAGS) $(CPPFLAGS) -o $@ $<

diaa_sami_comsupp.dll:
	@make -C diaa_sami_comsupp
	@cp diaa_sami_comsupp/diaa_sami_comsupp.dll diaa_sami_comsupp.dll

bin:
	mkdir bin

clean:
	@echo "Cleaning"
	@rm -rf bin/*.o bin/*.d serialport diaa_sami_comsupp.dll
