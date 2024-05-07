// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

//Copyright by Courtney Faulkner https://github.com/courtneyfaulkner
#include <stdio.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define CHECK_MALLOC(ptr) if (ptr == NULL) {printf("\nError: Cannot allocate memory for " #ptr "\n"); return 1;}

//Return value set to proper use of macros in this function and main
int print_platform_devices(cl_platform_id platform)
{
    cl_uint deviceCount;
    // get all devices
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
    cl_device_id* devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
    CHECK_MALLOC(devices);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);

    printf("\n");

    // for each device print critical attributes
    for (int j = 0; j < deviceCount; j++) {
        int item_number = 1;
        // print device name
        char* value;
        size_t valueSize;
        clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 0, NULL, &valueSize);
        value = (char*) malloc(valueSize);
        CHECK_MALLOC(value);
        clGetDeviceInfo(devices[j], CL_DEVICE_NAME, valueSize, value, NULL);
        printf(" %d. Device: %s\n", j+1, value);
        free(value);

        // print hardware device version
        clGetDeviceInfo(devices[j], CL_DEVICE_VERSION, 0, NULL, &valueSize);
        value = (char*) malloc(valueSize);
        CHECK_MALLOC(value);
        clGetDeviceInfo(devices[j], CL_DEVICE_VERSION, valueSize, value, NULL);
        printf("  %d.%d Hardware version: %s\n", j+1, item_number++, value);
        free(value);

        // print software driver version
        clGetDeviceInfo(devices[j], CL_DRIVER_VERSION, 0, NULL, &valueSize);
        value = (char*) malloc(valueSize);
        CHECK_MALLOC(value);
        clGetDeviceInfo(devices[j], CL_DRIVER_VERSION, valueSize, value, NULL);
        printf("  %d.%d Software version: %s\n", j+1, item_number++, value);
        free(value);

        // print c version supported by compiler for device
        clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &valueSize);
        value = (char*) malloc(valueSize);
        CHECK_MALLOC(value);
        clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, valueSize, value, NULL);
        printf("  %d.%d OpenCL C version: %s\n", j+1, item_number++, value);
        free(value);

        // print parallel compute units
        cl_uint maxComputeUnits;
        clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS,
        sizeof(maxComputeUnits), &maxComputeUnits, NULL);
        printf("  %d.%d Parallel compute units: %d\n", j+1, item_number++, maxComputeUnits);

        // print maximum number of work-items per compute unit
        cl_uint maxWorkGroupSize;
        clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_GROUP_SIZE,
        sizeof(maxWorkGroupSize), &maxWorkGroupSize, NULL);
        printf("  %d.%d Maximum workgroup size: %d\n", j+1, item_number++, maxWorkGroupSize);
        
        // print preferred work-group size
        cl_uint preferredWorkGroupSize;
        clGetDeviceInfo(devices[j], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
        sizeof(preferredWorkGroupSize), &preferredWorkGroupSize, NULL);
        printf("  %d.%d Preferred workgroup size: %d\n", j+1, item_number++, preferredWorkGroupSize);
        
        printf("  %d.%d Device system number: %d\n", j+1, item_number++, j);
    }
    free(devices);
    return 0;
}

int main() {

    int i, j;
    char* info;
    size_t infoSize;
    cl_uint platformCount;
    cl_platform_id *platforms;
    const char* attributeNames[5] = { "Name", "Vendor",
        "Version", "Profile", "Extensions" };
    const cl_platform_info attributeTypes[5] = { CL_PLATFORM_NAME, CL_PLATFORM_VENDOR,
        CL_PLATFORM_VERSION, CL_PLATFORM_PROFILE, CL_PLATFORM_EXTENSIONS };
    const int attributeCount = sizeof(attributeNames) / sizeof(char*);

    // get platform count
    clGetPlatformIDs(5, NULL, &platformCount);

    // get all platforms
    platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * platformCount);
    CHECK_MALLOC(platforms);
    clGetPlatformIDs(platformCount, platforms, NULL);

    printf("\n");

    // for each platform print all attributes
    for (i = 0; i < platformCount; i++) {

        printf("%d. Platform \n", i+1);

        for (j = 0; j < attributeCount; j++) {

            // get platform attribute value size
            clGetPlatformInfo(platforms[i], attributeTypes[j], 0, NULL, &infoSize);
            info = (char*) malloc(infoSize);
            CHECK_MALLOC(info);

            // get platform attribute value
            clGetPlatformInfo(platforms[i], attributeTypes[j], infoSize, info, NULL);

            printf(" %d.%d %-11s: %s\n", i+1, j+1, attributeNames[j], info);
            free(info);

        }
        printf(" %d.%d Platform system number: %d\n", i+1, j+1, i);

        print_platform_devices(platforms[i]);

        printf("========================================\n");

    }

    free(platforms);
    return 0;

}