#define CL_TARGET_OPENCL_VERSION    300

#include <CL/cl.h>
#include <string>
#include <iostream>
#include <fstream>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>

static std::string  initialize (void)
{
    std::string ret;

    return ret;
}

template <typename T>
void printValue (T val, const char * name)
{
    std::cerr << name << " = " << val;
}

template <>
void printValue (const char * val, const char * name)
{
    std::cerr << name << " = \"" << val << "\"";
}

#include "generated_methods.h"

