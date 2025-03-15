all: liboclcheck.so

parse_header.o: parse_header.h Makefile
generate_oclcheck.o: Makefile
oclcheck.o: Makefile generated_methods.h

%.o: %.cpp
	g++ -g -Wall -fPIC -shared -c -o $@ $< 

generate_oclcheck: generate_oclcheck.o parse_header.o
	g++ -g -Wall -o $@ $^

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
