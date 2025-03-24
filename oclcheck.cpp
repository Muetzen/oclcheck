#define CL_TARGET_OPENCL_VERSION    300

#include <CL/cl.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <string.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>

static std::ofstream gLogFile;
static std::ostream  * gLogStream = & std::cerr;

static bool gIsInitialized = false;

struct pointer_info
{
    void    * mPointer;
    int     mRetainCount = 0;
    int     mReleaseCount = 0;
};

static std::vector <struct pointer_info> g_cl_context_vector;
static std::vector <struct pointer_info> g_cl_command_queue_vector;
static std::vector <struct pointer_info> g_cl_mem_vector;
static std::vector <struct pointer_info> g_cl_sampler_vector;
static std::vector <struct pointer_info> g_cl_program_vector;
static std::vector <struct pointer_info> g_cl_kernel_vector;
static std::vector <struct pointer_info> g_cl_event_vector;

// TODO: locking
static
void createPointer (std::vector <struct pointer_info> & pointerList, void * pointer)
{
    struct pointer_info pi;

    pi.mPointer = pointer;

    pointerList.push_back (pi);
}

// TODO: locking
static
void retainPointer (std::vector <struct pointer_info> & pointerList, void * pointer)
{
    for (struct pointer_info & pi: pointerList)
    {
        if (pi.mPointer == pointer)
        {
            ++pi.mRetainCount;
            return;
        }
    }

    * gLogStream << "OCL> " << pointer << " not found in vector.\n";
}

// TODO: locking
static
void releasePointer (std::vector <struct pointer_info> & pointerList, void * pointer)
{
    std::vector <struct pointer_info>::iterator itEnd = pointerList.end ();
    std::vector <struct pointer_info>::iterator it = pointerList.begin ();

    for (; it != itEnd; ++it)
    {
        if (it->mPointer == pointer)
        {
            if (it->mReleaseCount == it->mRetainCount)
            {
                it = pointerList.erase (it);
            }
            else
            {
                ++it->mReleaseCount;
            }
            return;
        }
    }

    * gLogStream << "OCL> " << pointer << " not found in vector.\n";

    if (gLogFile.is_open ())
    {
        gLogStream = & std::cerr;
        gLogFile.close ();
    }
}

static void handle_atexit (void)
{
    * gLogStream << "OCL> Normal program termination.\n";

#define OUTPUT_POINTER(vector, singular, plural)                                        \
    if (vector.empty () == false)                                                       \
    {                                                                                   \
        * gLogStream << "OCL> " << vector.size () << " " plural " not released.\n";     \
        for (struct pointer_info & pi: vector)                                          \
        {                                                                               \
            * gLogStream << "OCL> " singular " at " << pi.mPointer                      \
                << ": retain count = " << pi.mRetainCount                               \
                << ", release count = " << pi.mReleaseCount << ".\n";                   \
        }                                                                               \
        vector.clear ();                                                                \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
        * gLogStream << "OCL> All created " plural " were released.\n";                 \
    }

    OUTPUT_POINTER (g_cl_context_vector, "Context", "contexts")
    OUTPUT_POINTER (g_cl_command_queue_vector, "Command quueue", "command queues")
    OUTPUT_POINTER (g_cl_mem_vector, "Memory object", "memory objects")
    OUTPUT_POINTER (g_cl_sampler_vector, "Sampler", "samplers") 
    OUTPUT_POINTER (g_cl_program_vector, "Program", "programs") 
    OUTPUT_POINTER (g_cl_kernel_vector, "Kernel", "kernels") 
    OUTPUT_POINTER (g_cl_event_vector, "Event", "events") 
}

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

        atexit (& handle_atexit);

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
    if (val == nullptr)
    {
        * gLogStream << name << " = nullptr";
    }
    else if (strcmp (name, "errcode_ret") == 0)
    {
        * gLogStream << name;
    }
    else
    {
        * gLogStream << name << " = \"" << val << "\"";
    }
}

#include "generated_methods.h"

