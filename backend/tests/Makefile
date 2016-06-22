CXX=g++
CFLAGS=-c -O3 -std=c++11 -Weffc++ -Wall -Wextra -I../../kernel_module/include -I./include -I../include -I../utils -Wdouble-promotion -Wuninitialized -fipa-pure-const -Wtrampolines -Wfloat-equal  -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wredundant-decls -Wshadow -Wint-to-pointer-cast
LDFLAGS=-Wl,-rpath '-Wl,$$ORIGIN' -L. -ltd-backend
PROGRAMS= \
	random-ios \
	io-verify \
	io-verify-files
OBJECTS=$(PROGRAMS:%=bin/%.o)
DEPS=$(PROGRAMS:%=bin/%.d)

all: bin $(PROGRAMS)

-include $(DEPS)

documentation:
	doxygen doc/doxygen.conf
	cd doc/latex && make

$(PROGRAMS): $(PROGRAMS:%=bin/%.o) libtd-backend.so
	@echo "Linking" $@
	@$(CXX) -o $@ $(@:%=bin/%.o) $(LDFLAGS)

bin/%.o: src/%.cpp
	@echo "Compiling" $<
	@$(CXX) -MD $(CFLAGS) -o $@ $<

bin:
	@mkdir bin

libtd-backend.so:
	@ln -s ../$@ .

test:
	sudo ./random-ios --disk td0 --operation read --start 0 --end 0 --blocksize 65536 --duration 10 --command "echo 3 | tee /proc/sys/vm/drop_caches"

clean:
	@echo "Cleaning"
	@rm -rf bin/*.o bin/*.d $(PROGRAMS) libtd-backend.so
