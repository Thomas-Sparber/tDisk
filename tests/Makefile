CXX=g++
CFLAGS=-c -g -std=c++11 -Weffc++ -Wall -Wextra -I../kernel_module/include -I./include -I../frontend/include -Wdouble-promotion -Wuninitialized -fipa-pure-const -Wtrampolines -Wfloat-equal  -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wredundant-decls -Wshadow -Wint-to-pointer-cast
LDFLAGS=-L../frontend -ltd-frontend
PROGRAMS= \
	random-ios \
	io-verify \
	io-verify-files
OBJECTS=$(PROGRAMS:%=bin/%.o)
DEPS=$(PROGRAMS:%=bin/%.d)

all: $(PROGRAMS) bin

-include $(DEPS)

documentation:
	doxygen doc/doxygen.conf
	cd doc/latex && make

$(PROGRAMS): $(PROGRAMS:%=bin/%.o)
	@echo "Linking" $@
	@$(CXX) -o $@ $(@:%=bin/%.o) $(LDFLAGS)

bin/%.o: src/%.cpp
	@echo "Compiling" $<
	@$(CXX) -MD $(CFLAGS) -o $@ $<

bin:
	mkdir bin

test:
	sudo ./random-ios --disk td0 --operation read --start 0 --end 0 --blocksize 65536 --duration 10 --command "echo 3 | tee /proc/sys/vm/drop_caches"

clean:
	@echo "Cleaning"
	@rm -rf bin/*.o bin/*.d $(PROGRAMS)
