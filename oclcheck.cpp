#define CL_TARGET_OPENCL_VERSION    300

#include <CL/cl.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>

static std::ofstream gLogFile;
static std::ostream  * gLogStream = & std::cerr;

static bool gIsInitialized = false;

static void initialize (void)
{
    if (gIsInitialized == false)
    {
        std::cerr << "OCL> Initializing.\n";

        const char * logfile = getenv ("OCLCHECK_LOGFILE");
        if (logfile != nullptr)
        {
            // TODO: gLogFile.close () in atexit method
            gLogFile.open (logfile);
            if (gLogFile.is_open ())
            {
                gLogStream = & gLogFile;
            }
            else
            {
                * gLogStream << "OCL> Could not open logfile \"" << logfile << "\".\n";
                * gLogStream << "OCL> " << strerror (errno) << "\n";
            }
        }

        gIsInitialized = true;
    }
}

template <typename T>
void printValue (T val, const char * name)
{
    * gLogStream << name << " = " << val;
}

#define printOclType(type)                              \
template <>                                                 \
void printValue (type val, const char * name)               \
{                                                           \
    * gLogStream << name << " = " #type " (" << val << ")"; \
}

printOclType (cl_platform_id)
printOclType (cl_context)
printOclType (cl_device_id)
printOclType (cl_command_queue)
printOclType (cl_mem)
printOclType (cl_sampler)
printOclType (cl_program)
printOclType (cl_kernel)
printOclType (cl_event)

template <>
void printValue (const char * val, const char * name)
{
    * gLogStream << name << " = \"" << val << "\"";
}

#include "generated_methods.h"

