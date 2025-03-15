#define CL_TARGET_OPENCL_VERSION    300

#include <CL/cl.h>
#include <string>
#include <iostream>
#include <fstream>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>

static std::string  initialize (void);
static const char * errorString (cl_int code);

#include "generated_methods.h"

static std::string  initialize (void)
{
    std::string ret;

    return ret;
}

