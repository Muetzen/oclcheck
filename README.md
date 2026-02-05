oclcheck
========

oclcheck is a shell script, which starts a program with the liboclcheck
library preloaded. This can be used, to debug certain OpenCL problems.

This is work in progress, but it is possible to obtain a trace of all
OpenCL calls of a program. The library also checks for memory leaks, due
to missing calls to clReleaseContext and similar functions.


Compile
-------

The liboclcheck library can be compiled with `make`. You will need the `cl.h`
and `cl\_ext.h` headers in `/usr/include/CL/`. (See `generate\_oclcheck.cpp`,
if you need to change the location of these header files.)

I've compiled the library with Ubuntu 24.04. There are some assumptions about
the format of `cl.h` and `cl\_ext.h` built into `parse\_header.cpp`. Therefore
this might or might not compile with different linux distributions.


Install
-------

Copy `oclcheck` and `liboclcheck.so` into the same directory inside your
PATH.


Usage:

```
oclcheck [-l logfile] your/program [program parameters]
```

Attention: An existing logfile will be overwritten.
