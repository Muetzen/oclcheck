#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <vector>
 
#include <CL/cl.h>
 
#define MAX_SOURCE_SIZE (0x100000)
#define LIST_SIZE 1024
 
int main (int argc, char **argv)
{
    printf("started running\n");

    // Create the input and output vectors
    std::vector <int>   A, B, C;
    A.resize (LIST_SIZE);   // in
    B.resize (LIST_SIZE);   // in
    C.resize (LIST_SIZE);   // out

    for (int i = 0; i < A.size (); ++i)
    {
        A[i] = i;
        B[i] = LIST_SIZE - i;
        C[i] = 0;
    }
 
    // Load the kernel source code into the array source_str
    const char *source_str =
R"!(__kernel void vector_add(__global const int *A, __global const int *B, __global int *C) {
 
    // Get the index of the current element to be processed
    int i = get_global_id(0);
 
    // Do the operation
    C[i] = A[i] + B[i];
}
)!";
    size_t  source_size = strlen (source_str);

    // Get platform and device information
    cl_device_id device_id = NULL;   
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    
    
    cl_int ret = clGetPlatformIDs(0, NULL, &ret_num_platforms);
    cl_platform_id *platforms = NULL;
    platforms = (cl_platform_id*)malloc(ret_num_platforms*sizeof(cl_platform_id));

    ret = clGetPlatformIDs(ret_num_platforms, platforms, NULL);
    if (ret != CL_SUCCESS) return 1;

    ret = clGetDeviceIDs( platforms[1], CL_DEVICE_TYPE_ALL, 1, 
            &device_id, &ret_num_devices);
if (ret != CL_SUCCESS) return 1;
    // Create an OpenCL context
    cl_context context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);
    if (ret != CL_SUCCESS) return 1;

    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    if (ret != CL_SUCCESS) return 1;

    // Create memory buffers on the device for each vector 
    cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, 
            LIST_SIZE * sizeof(int), NULL, &ret);
    cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
            LIST_SIZE * sizeof(int), NULL, &ret);
    cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
            LIST_SIZE * sizeof(int), NULL, &ret);
 
    // Copy the lists A and B to their respective memory buffers
    ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0,
            LIST_SIZE * sizeof(int), A.data (), 0, NULL, NULL);
if (ret != CL_SUCCESS) return 1;

    ret = clEnqueueWriteBuffer(command_queue, b_mem_obj, CL_TRUE, 0, 
            LIST_SIZE * sizeof(int), B.data (), 0, NULL, NULL);
if (ret != CL_SUCCESS) return 1;
 
    printf("before building\n");
    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1, 
            (const char **)&source_str, &source_size, &ret);
if (ret != CL_SUCCESS) return 1;
 
    // Build the program
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if (ret != CL_SUCCESS) return 1;

    printf("after building\n");
    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "vector_add", &ret);
    if (ret != CL_SUCCESS) return 1;
 
    // Set the arguments of the kernel
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
    if (ret != CL_SUCCESS) return 1;

    ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem_obj);
    if (ret != CL_SUCCESS) return 1;

    ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&c_mem_obj);
    if (ret != CL_SUCCESS) return 1;

    //added this to fix garbage output problem
    //ret = clSetKernelArg(kernel, 3, sizeof(int), &LIST_SIZE);
 
    printf("before execution\n");
    // Execute the OpenCL kernel on the list
    size_t global_item_size = LIST_SIZE; // Process the entire lists
    size_t local_item_size = 64; // Divide work items into groups of 64
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 
            &global_item_size, &local_item_size, 0, NULL, NULL);
    printf("after execution\n");
    // Read the memory buffer C on the device to the local variable C
    ret = clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0, 
            LIST_SIZE * sizeof(int), C.data (), 0, NULL, NULL);
    printf("after copying\n");
    // Display the result to the screen
    for (int i = 0; i < LIST_SIZE; ++i)
    {
        printf("%d + %d = %d\n", A[i], B[i], C[i]);
    }

    // Allocate shared virtual memory
    void    * svm_ptr = clSVMAlloc (context, CL_MEM_READ_WRITE,
            4096,   // svm_size
            0);     // alignment

    // Clean up
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);

    // Note: This leaks memory on purpose
//  if (svm_ptr != NULL)
//  {
//      clSVMFree (context, svm_ptr);
//  }
//  ret = clReleaseKernel(kernel);
//  ret = clReleaseProgram(program);
//  ret = clReleaseMemObject(a_mem_obj);
//  ret = clReleaseMemObject(b_mem_obj);
//  ret = clReleaseMemObject(c_mem_obj);
//  ret = clReleaseCommandQueue(command_queue);
//  ret = clReleaseContext(context);
    return 0;
}
