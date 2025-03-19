all: liboclcheck.so

parse_header.o: parse_header.h Makefile
generate_oclcheck.o: parse_header.h Makefile
oclcheck.o: generated_methods.h Makefile

DEBUG := -g -fstack-protector

%.o: %.cpp
	g++ $(DEBUG) -Wall -fPIC -shared -c -o $@ $< 

generate_oclcheck: generate_oclcheck.o parse_header.o
	g++ $(DEBUG) -Wall -o $@ $^

generated_methods.h: generate_oclcheck
	./generate_oclcheck > generated_methods.h

liboclcheck.so: oclcheck.o
	g++ -g -Wall -fPIC -shared -o $@ $< -ldl

test-clinfo: liboclcheck.so
	OCLCHECK_LOGFILE=./clinfo.log LD_PRELOAD=./liboclcheck.so clinfo
	cat clinfo.log

clean:
	rm -f *.o generate_oclcheck clinfo.log

realclean: clean
	rm -f liboclcheck.so
