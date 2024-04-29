// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#define __CL_ENABLE_EXCEPTIONS

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.hpp>
#else
#include <CL\cl.hpp>
#endif
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <format>
#include <array>
#include <cstdint>
#include <chrono>
#include <thread>
#include "crypt_bf_ocl.h"

//Load text file to string
std::string load_file(std::string file_name)
{
    std::ifstream inFile;
    inFile.open(file_name);
    if(!inFile.is_open())
        throw std::runtime_error((std::format("Cannot open file {}", file_name)));

    std::stringstream strStream;
    strStream << inFile.rdbuf();
    std::string str = strStream.str();
    return str;
}

#define WRITELN(i) std::cout << std::format("{} ",i) << std::endl;
#define WRITE(i) std::cout << std::format("{} ",i)
#define HEX(x) std::format("0x{:04X}", x)

//Number of keys checked simultaneously.
//Must be power of 2
#define KEYS 65536
#define ARRAY std::array<cl_ushort, KEYS>
using QByteArray = std::array<cl_ushort, 4>;
typedef cl_int CL_BUF[KEYS];
//typedef CL

int main(int argc, char *argv[])
{
    //constexpr auto keys_start = create_array<uint16_t, 4>();
    //constexpr auto keys_end = create_array<uint16_t, 4, 1>();

    std::string kernel_source_filename = "crypt_bf.cl";

    if (argc == 2)
    {
        kernel_source_filename = argv[1];
    }

    try
    {
        //ARRAY keys_start = {0x6587};
        
        std::array<QByteArray, KEYS> key_found = {0};

        cl_int err = CL_SUCCESS;
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        if (platforms.size() == 0) {
            std::cout << "Platform size 0\n";
            return -1;
        }

        std::string platformInfo;
        platforms[0].getInfo(CL_PLATFORM_VERSION, &platformInfo);
        std::cout << "Platform Info: " << platformInfo.c_str() << std::endl;

        cl_context_properties properties[] = 
            { CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};
        cl::Context context(CL_DEVICE_TYPE_DEFAULT, properties);

        std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

        std::string name;
        devices[0].getInfo(CL_DEVICE_NAME, &name);
        std::cout << "Device name: " << name.c_str() << std::endl;

        cl_uint maxWorkGroupSize;
        devices[0].getInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE, &maxWorkGroupSize);
        //std::cout << std::format("Maximum workgroup size: {}", maxWorkGroupSize) << std::endl;

        std::string kernel_source = load_file(kernel_source_filename);
        cl::Program::Sources source(1,
            std::make_pair(kernel_source.c_str(), kernel_source.length()));
        cl::Program program_ = cl::Program(context, source);
        program_.build(devices, "-I.");

        /*
        //Name MUST be the same as in .cl file
        cl::Kernel kernel(program_, "try_key_0", &err);

        cl::Buffer cl_buf_keys_start(context, CL_MEM_READ_ONLY, sizeof(keys_start));
        cl::Buffer cl_buf_key_found  (context, CL_MEM_READ_ONLY, sizeof(key_found));
        
        cl::Event event;
        cl::CommandQueue queue(context, devices[0], 0, &err);
        queue.enqueueWriteBuffer(cl_buf_keys_start, CL_TRUE, 0, sizeof(keys_start), &keys_start);

        kernel.setArg(0, cl_buf_keys_start);
        kernel.setArg(1, cl_buf_key_found);
        
        int num_global = keys_start.size();
        int num_local = 3;//keys_start.size();;
        queue.enqueueNDRangeKernel(kernel,
                                cl::NullRange,
                                cl::NDRange(num_global),
                                cl::NDRange(num_local),
                                NULL,
                                &event);
        queue.enqueueReadBuffer(cl_buf_key_found, CL_TRUE, 0, sizeof(key_found), key_found.begin());
        event.wait();
        for (int i=0; i<KEYS; i++)
        {
            bool all_words_are_zeroes = true;
            std::string key_as_string = "";
            for (int j=0; j<key_found.at(i).size(); j++)
            {
                int key_word = key_found.at(i).at(j);
                all_words_are_zeroes = all_words_are_zeroes && (key_word == 0);
                key_as_string += HEX(key_word) + " ";
            }
            if (!all_words_are_zeroes)
            {
                std::cout << "Host: Candidate key " << key_as_string << std::endl;
            }
            
            next:
            //WRITELN("===");
        }*/
        ARRAY   keys_0_start,
                keys_1_start,
                keys_1_end;

        //Name MUST be the same as in .cl file
        cl::Kernel kernel(program_, "try_key_1", &err);

        cl::Buffer cl_buf_keys_0_start(context, CL_MEM_READ_ONLY, sizeof(keys_0_start));
        cl::Buffer cl_buf_keys_1_start(context, CL_MEM_READ_ONLY, sizeof(keys_1_start));
        cl::Buffer cl_buf_keys_1_end  (context, CL_MEM_READ_ONLY, sizeof(keys_1_end));
        cl::Buffer cl_buf_key_found  (context, CL_MEM_READ_ONLY, sizeof(key_found));
        
        cl::Event event;
        cl::CommandQueue queue(context, devices[0], 0, &err);
        for (int k0=0x6587-9; k0<=0x6587; k0++)
        { 
            //Fill K0
            for (int k0_i=0; k0_i<keys_0_start.size(); k0_i++)
            {
                keys_0_start[k0_i] = k0;
            }
            int k1_buckets = (MAX+1) / keys_1_start.size();
            int k1_bucket_size = keys_1_start.size();
            WRITELN(HEX(k0));
            //WRITELN(k1_bucket_size);
            //WRITELN(k1_buckets);
            //WRITELN("---");
            //for (int k1=0; k1<=MAX; k1+=k1_bucket_size)
            //for (int k1=0; k1<=MAX; k1+=k1_bucket_size)
            //{
                for (int k1_i=0; k1_i<k1_bucket_size; k1_i++)
                {
                    keys_1_start.at(k1_i) = k1_buckets*k1_i;
                    keys_1_end  .at(k1_i) = k1_buckets*(k1_i+1) - 1;
                    //WRITE(k0); WRITE(k1); WRITE(k1_i); WRITELN("");
                    //WRITE(k0); WRITE(k1_i); WRITELN("");
                    //WRITE(keys_1_start[k1_i]); WRITE(keys_1_end[k1_i]); WRITELN("");
                }
                //WRITELN("=");
                //continue;
                queue.enqueueWriteBuffer(cl_buf_keys_0_start, CL_TRUE, 0, sizeof(keys_0_start), &keys_0_start);
                queue.enqueueWriteBuffer(cl_buf_keys_1_start, CL_TRUE, 0, sizeof(keys_1_start), &keys_1_start);
                queue.enqueueWriteBuffer(cl_buf_keys_1_end,   CL_TRUE, 0, sizeof(keys_1_end),   &keys_1_end);

                kernel.setArg(0, cl_buf_keys_0_start);
                kernel.setArg(1, cl_buf_keys_1_start);
                kernel.setArg(2, cl_buf_keys_1_end);
                kernel.setArg(3, cl_buf_key_found);
                
                int num_global = k1_bucket_size;
                int num_local = maxWorkGroupSize;//256;
                queue.enqueueNDRangeKernel(kernel,
                                        cl::NullRange,
                                        cl::NDRange(num_global),
                                        cl::NDRange(num_local),
                                        NULL,
                                        &event);
                queue.enqueueReadBuffer(cl_buf_key_found, CL_TRUE, 0, sizeof(key_found), key_found.begin());
                event.wait();
                for (int i=0; i<key_found.size(); i++)
                {
                    bool all_words_are_zeroes = true;
                    std::string key_as_string = "";
                    for (int j=0; j<key_found.at(i).size(); j++)
                    {
                        int key_word = key_found.at(i).at(j);
                        all_words_are_zeroes = all_words_are_zeroes && (key_word == 0);
                        key_as_string += HEX(key_word) + " ";
                    }
                    if (!all_words_are_zeroes)
                    {
                        std::cout << "Host: Candidate key " << key_as_string << std::endl;
                    }
                }
            //}//k1
        }//k0
    }//try
    catch (cl::Error err)
    {
        std::cerr 
           << "ERROR: "
           << err.what()
           << "("
           << err.err()
           << ")"
           << std::endl;
    }
    return 0;
}