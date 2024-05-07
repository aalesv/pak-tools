// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#define __CL_ENABLE_EXCEPTIONS
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.hpp>
#else
#include <CL\cl.hpp>
#endif
#include <iostream>
#include <format>

const char *VERSION = "2024.0430";

int main(int argc, char *argv[])
{

    std::cout << std::format("GPU parameters autodetect v{}", VERSION) << std::endl;
    std::cout << "Usage: gpu-settings-detect.exe [platform_number device_number]" << std::endl;

    int platform_number=0,
        device_number=0;
    
    if (argc == 3)
    {
        platform_number = std::stoi(argv[1]);
        device_number   = std::stoi(argv[2]);
    }

    std::string kernel_source = "__kernel void do_nothing(){}";

    cl_int err = CL_SUCCESS;
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.size() == 0) {
        std::cout << "Platform size 0\n";
        return -1;
    }

    std::string platformInfo;
    platforms[platform_number].getInfo(CL_PLATFORM_VERSION, &platformInfo);
    std::cout << "Platform Info: " << platformInfo.c_str() << std::endl;

    cl_context_properties properties[] = 
        { CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[platform_number])(), 0};
    cl::Context context(CL_DEVICE_TYPE_DEFAULT, properties);

    std::vector<cl::Device> devices_ = context.getInfo<CL_CONTEXT_DEVICES>();

    std::string name;
    devices_[device_number].getInfo(CL_DEVICE_NAME, &name);
    std::cout << "Device name: " << name.c_str() << std::endl;
    std::vector<cl::Device> devices;
    devices.push_back(devices_[device_number]);

    cl::Program::Sources source(1,
        std::make_pair(kernel_source.c_str(), kernel_source.length()));
    cl::Program program_ = cl::Program(context, source);
    try
    {
        program_.build(devices);
    }
    catch (cl::Error &err)
    {
        for (cl::Device dev : devices)
        {
            // Check the build status
            cl_build_status status = program_.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(dev);
            if (status != CL_BUILD_ERROR)
            {
                std::cerr 
                << "ERROR: "
                << err.what()
                << "("
                << err.err()
                << ")"
                << std::endl;
                continue;
            }
            // Get the build log
            std::string name     = dev.getInfo<CL_DEVICE_NAME>();
            std::string buildlog = program_.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dev);
            std::cerr << "Build log for " << name << ":" << std::endl
                        << buildlog << std::endl;
        }
    }
    cl::Kernel kernel(program_, "do_nothing", &err);
    cl::Event event;
    cl::CommandQueue queue(context, devices[device_number], 0, &err);
    
    constexpr int MIN = 1,
                  MAX = 0x10000;
    //Max work items
    int num_global;
    //Max work groups
    int num_local = MIN;
    bool finished = false;
    int min = MIN,
        max = MAX;
    int mid = min + (max - min + 1) / 2;
    num_global = mid;
    while ((min < mid) and (mid < max))
    {   
        //std::cout << std::format("min={} mid={} max={}", min, mid, max) << std::endl;
        try
        {
            //Run
            queue.enqueueNDRangeKernel(kernel,
                                cl::NullRange,
                                cl::NDRange(num_global),
                                cl::NDRange(num_local),
                                NULL,
                                &event);
            event.wait();
            min = mid;
        }
        catch (cl::Error &err)
        {
            max = mid;
            /*std::cerr 
           << "ERROR: "
           << err.what()
           << "("
           << err.err()
           << ")"
           << std::endl;*/
        }
        mid = min + (max - min + 1) / 2;
        num_global = mid;
    }

    std::cout << std::format("GLOBAL_WI = {}",num_global) << std::endl;

    min = MIN,
    max = MAX;
    mid = min + (max - min + 1) / 2;
    num_local = mid;
    while ((min < mid) and (mid < max))
    {   
        //std::cout << std::format("min={} mid={} max={}", min, mid, max) << std::endl;
        try
        {
            //Run
            queue.enqueueNDRangeKernel(kernel,
                                cl::NullRange,
                                cl::NDRange(num_global),
                                cl::NDRange(num_local),
                                NULL,
                                &event);
            event.wait();
            min = mid;
        }
        catch (cl::Error &err)
        {
            max = mid - 1;
            /*std::cerr 
           << "ERROR: "
           << err.what()
           << "("
           << err.err()
           << ")"
           << std::endl;*/
        }
        mid = min + (max - min + 1) / 2;
        num_local = mid;
    }
    std::cout << std::format("LOCAL_WG = {}",num_local) << std::endl;

}