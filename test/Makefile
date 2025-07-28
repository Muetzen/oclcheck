all:	sum2

sum2:	sum2.cpp
	g++ -g -O3 -o sum2 sum2.cpp -lOpenCL

test:	sum2
	../oclcheck -l sum2.log ./sum2
	cat sum2.log
