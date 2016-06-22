CC=gcc
CXX=g++
CFLAGS=-c -O3 -Wall -Wextra  -I./include -I../../utils -I../base/include -isystem../../libs/liboauthcpp/include/ -isystem../../libs/jsoncpp/include -Wdouble-promotion -Wuninitialized -fipa-pure-const -Wtrampolines -Wfloat-equal  -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wredundant-decls -Wshadow -Wint-to-pointer-cast
CPPFLAGS=-std=c++11 -Weffc++
LDFLAGS=-Wl,-rpath '-Wl,$$ORIGIN' -lcurl -L. -loauthcpp -ljsoncpp -lplugin-base -lnl-3 -lnl-genl-3 -lpthread
SOURCES=

CPP_SOURCES= \
	src/dropbox.cpp \
	src/dropboxtdisk.cpp \
	src/main.cpp

OBJECTS=$(SOURCES:src/%.c=bin/%.o) $(CPP_SOURCES:src/%.cpp=bin/%.o)
DEPS=$(OBJECTS:bin/%.o=bin/%.d)

all: bin dropbox

-include $(DEPS)

dropbox: $(OBJECTS) liboauthcpp.so libjsoncpp.so libplugin-base.so libnl-3.so libnl-genl-3.so
	@echo "Linking" $@
	@$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

bin/%.o: src/%.c
	@echo "Compiling" $<
	@$(CC) -MD $(CFLAGS) -o $@ $<

bin/%.o: src/%.cpp
	@echo "Compiling" $<
	@$(CXX) -MD $(CFLAGS) $(CPPFLAGS) -o $@ $<

bin:
	@mkdir bin

liboauthcpp.so:
	@ln -s ../../libs/liboauthcpp/$@ .

libjsoncpp.so:
	@ln -s ../../libs/jsoncpp/src/lib_json/$@ .

libplugin-base.so:
	@ln -s ../base/$@ .

libnl-3.so:
	@ln -s ../../libs/libnl/lib/.libs/libnl-3.so.200 .
	@ln -s libnl-3.so.200 libnl-3.so

libnl-genl-3.so:
	@ln -s ../../libs/libnl/lib/.libs/libnl-genl-3.so.200 .
	@ln -s libnl-genl-3.so.200 libnl-genl-3.so

clean:
	@echo "Cleaning"
	@rm -rf bin/*.o bin/*.d dropbox liboauthcpp.so libjsoncpp.so libplugin-base.so libnl-3.so libnl-3.so.200 libnl-genl-3.so libnl-genl-3.so.200
