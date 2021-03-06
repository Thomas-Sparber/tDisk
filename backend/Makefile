CC=gcc
CXX=g++
CFLAGS=-c -O3 -Wall -Wextra -I../kernel_module/include -I./include -I./utils -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wdouble-promotion -Wuninitialized -fipa-pure-const -Wtrampolines -Wfloat-equal  -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wredundant-decls -Wshadow -Wint-to-pointer-cast
CPPFLAGS=-std=c++11 -Weffc++
LDFLAGS_SHARED=
LDFLAGS=-Wl,-rpath '-Wl,$$ORIGIN' -L. -ltd-backend
SOURCES= \
	src/tdisk.c \
	src/tdisk_fake.c

CPP_SOURCES= \
	src/backend.cpp \
	src/backend_c.cpp \
	src/configfile.cpp \
	src/device.cpp \
	src/deviceadvisor.cpp \
	src/extinodescan.cpp \
	src/filesystem.cpp \
	src/option.cpp \
	src/options.cpp \
	src/shell.cpp \
	src/tdisk.cpp \
	src/tdisk_advice.cpp

MAIN_SOURCE= \
	src/main.cpp


OBJECTS=$(SOURCES:src/%.c=bin/%.c.o) $(CPP_SOURCES:src/%.cpp=bin/%.cpp.o)
MAIN_OBJECT=$(MAIN_SOURCE:src/%.cpp=bin/%.cpp.o)
DEPS=$(OBJECTS:bin/%.o=bin/%.d) $(MAIN_OBJECT:bin/%.o=bin/%.d)

TARGETS=bin tdisk

ifeq ($(OS),Windows_NT)
	LIB_NAME=libtd-backend.dll
	TARGETS+= diaa_sami_comsupp.dll wmi.dll
else
	LIB_NAME=libtd-backend.so
	CFLAGS+= -fPIC
	LDFLAGS_SHARED+= -lparted -lext2fs
	TARGETS+= make_plugins make_tests
endif

all: $(TARGETS)

-include $(DEPS)

$(LIB_NAME): $(OBJECTS)
	@echo "Linking" $@
	@$(CXX) -shared -o $@ $(OBJECTS) $(LDFLAGS_SHARED)

tdisk: $(MAIN_OBJECT) $(LIB_NAME)
	@echo "Linking" $@
	@$(CXX) -o $@ $(MAIN_OBJECT) $(LDFLAGS)

bin/%.c.o: src/%.c
	@echo "Compiling" $<
	@$(CC) -MD $(CFLAGS) -o $@ $<

bin/%.cpp.o: src/%.cpp
	@echo "Compiling" $<
	@$(CXX) -MD $(CFLAGS) $(CPPFLAGS) -o $@ $<

bin:
	@mkdir bin

make_plugins:
	@make -C plugins

make_tests:
	@make -C tests

diaa_sami_comsupp.dll:
	@ln -s libs/wmi/diaa_sami_comsupp/$@ .

wmi.dll:
	@ln -s libs/wmi/$@ .

clean:
	@echo "Cleaning"
	@rm -rf bin/*.o bin/*.d tdisk $(LIB_NAME) diaa_sami_comsupp.dll wmi.dll 
	@make -C plugins clean
	@make -C tests clean
