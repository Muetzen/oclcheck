oclcheck is a shared library, which supports in debugging OpenCL problems.

This is work in progress, but it is possible to obtain a trace of all
OpenCL calls of a program. The library should also check for memory
leaks, due to missing calls to clReleaseContext and similar functions.


Compile:

This can be compiled with "make". You will need an cl.h header installed
in "/usr/include/CL/cl.h". (See generate\_oclcheck.cpp, if you need to
change the location.)

I've compiled it with Ubuntu 24.04, and there are some assumption abouot
the format of the cl.h header file built into parse\_header.cpp.


Usage:

LD\_PRELOAD=./liboclcheck.so your/program

You can also set OCLCHECK\_LOGFILE to point to a logfile.
Attention: This will be overwritten, if it exists.

OCLCHECK\_LOGFILE=example.log LD\_PRELOAD=./liboclcheck.so your/program
