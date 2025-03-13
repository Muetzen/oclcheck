oclcheck is a shared library, which supports in debugging OpenCL problems.

This is work in progress, but it should be possible to obtain a trace of
all OpenCL calls of a program. The library should also check for memory
leaks, due to missing calls to clReleaseContext and similar functions.

First source code will be following soon.
