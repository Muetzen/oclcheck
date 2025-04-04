#define CL_TARGET_OPENCL_VERSION    300

#include <CL/cl.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stacktrace>
#include <vector>
#include <stdlib.h>
#include <string.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>

#include "oclcheck_version.h"

static std::ofstream gLogFile;
static std::ostream  * gLogStream = & std::cerr;

static bool gIsInitialized = false;

struct pointer_info
{
    void            * mPointer;
    int             mRetainCount = 0;
    int             mReleaseCount = 0;
    std::stacktrace mCreateStack;
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
    pi.mCreateStack = std::stacktrace::current ();

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
            for (const std::stacktrace_entry & entry: pi.mCreateStack)                  \
            {                                                                           \
                * gLogStream << "OCL>  " << std::to_string (entry) << "\n";             \
            }                                                                           \
            * gLogStream << "OCL>\n";                                                   \
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
        std::cerr << "OCL> oclcheck version " OCLCHECK_VERSION " started.\n";

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

static
void
printOpenClProgramSource (cl_uint count, const char **strings, const size_t *lengths)
{
    if (count == 0)
    {
        return;
    }

    if (strings == nullptr)
    {
        * gLogStream << "OCL>\tstrings = nullptr,\n";
        return;
    }

    for (cl_uint i = 0; i < count; ++i)
    {
        if (strings [i] == nullptr)
        {
            * gLogStream << "OCL>\tstrings [" << i << "] = nullptr,\n";
            return;
        }
    }

    * gLogStream << "OCL>\tstrings [] = \"\n";
    for (cl_uint i = 0; i < count; ++i)
    {
        std::string program;
        if (lengths != nullptr)
        {
            program.assign (strings [i], lengths [i]);
        }
        else
        {
            program = strings [i];
        }

        while (program.empty () == false)
        {
            size_t  pos = program.find ('\n');
            if (pos != std::string::npos)
            {
                * gLogStream << "OCL>\t\t" << program.substr (0, pos) << "\n";
                program.erase (0, pos + 1);
            }
            else
            {
                * gLogStream << "OCL>\t\t" << program << "\n";
                program.clear ();
            }
        }
    }
    * gLogStream << "OCL>\t\",\n";
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

#include "generated_methods.h"

