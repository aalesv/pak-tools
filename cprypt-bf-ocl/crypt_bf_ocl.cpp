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
#include <map>
#include <conio.h>
#include <csignal>
#include "crypt_bf_ocl.h"

const char *VERSION = "2024.0430";

//Convert from string to integer
//If string starts with '0x', assume hex, else - dec
int _stoi(std::string s)
{
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    int radix = s.starts_with("0x") ? 16 : 10;
    return std::stoi(s, nullptr, radix);
}

int CURRENT_KEY;
#define HEX(x) std::format("0x{:04X}", x)

//Print current key.
void print_current_key()
{
    int key = CURRENT_KEY;
    std::cout << std::format("Current key is {} {} {} {}",
                            HEX(key), 0, 0, 0)
            << std::endl;
}

//SIGINT handler
void sigint_handler(int param)
{
    print_current_key();
    exit(1);
}

//Options
static std::map<std::string, std::string> OPTIONS;
//Config file default name
static std::string cfg_file = "crypt_bf_mt.ini";
//Start key high word
static cl_ushort START_K0 = 0,
                 //End key high word
                 END_K0 = 0xFFFF;
//Global size, total number of work-items
static cl_uint  GLOBAL_WI,
                //Local size, the number of work-items per work-group
                LOCAL_WG;
static int DEBUG_LEVEL=0;
//OpenCL kernel sources file name
static std::string OCL_KERNEL_SOURCES,
                OCL_KERNEL_NAME;

//Parse config file
std::map<std::string, std::string> parse_cfg(std::istream & cfgfile)
{
    int cfg_string_num = 0;
    const int max_str_len = 256;
    std::map<std::string, std::string> options;
    for (std::string line; std::getline(cfgfile, line); )
    {
        std::istringstream iss(line);
        std::string id, eq, val;
        cfg_string_num++;

        bool error = false;

        if (!(iss >> id))
        {
            error = true;
        }
        //Skip comments
        else if (id[0] == '#')
        {
            continue;
        }
        //Malformed string
        else if (!(iss >> eq >> val) or eq != "=" or iss.get() != EOF)
        {
            error = true;
        }
        if (id.length() >= max_str_len or
                val.length() >= max_str_len)
        {
            error = true;
        }

        if (error)
        {
            std::cerr << std::format("Cannot parse config file string {}, skipping", cfg_string_num) << std::endl;
        }
        else
        {
            options[id] = val;
        }
    }
    return options;
}

//Convert from INI file string to integer variable
//VAR_NAME = options["VAR_NAME"]
//Throw exception with name of var
#define CONFIG_STOI(s) try  { \
                            s = _stoi(options[#s]); \
                            } catch (...) { \
                            std::cout<<"Can't convert value of "<<#s<<" to integer."<<std::endl; \
                            exit(1);}
//Set variables from INI file
void set_options(std::string &cfgfile)
{
    std::filebuf fb;
    std::map<std::string, std::string> options;
    fb.open(cfgfile, std::ios::in);
    if (fb.is_open())
    {
        std::istream is(&fb);
        options = parse_cfg(is);
    }
    else
        throw std::runtime_error((std::format("Cannot open file {}", cfgfile)));
    fb.close();
    CONFIG_STOI(START_K0);
    CONFIG_STOI(END_K0);
    CONFIG_STOI(GLOBAL_WI);
    CONFIG_STOI(LOCAL_WG);
    CONFIG_STOI(DEBUG_LEVEL);
    OCL_KERNEL_SOURCES = options["OCL_KERNEL_SOURCES"];
    OCL_KERNEL_NAME = options["OCL_KERNEL_NAME"];
}

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
//Size in bytes of vector
#define SIZEOF_VEC(V) V.size()*sizeof(decltype(V)::value_type)

using QByteArray = std::array<cl_ushort, 4>;

//Print usage info
void usage ()
{
    std::cout << "Usage:" << std::endl;
    std::cout << "crypt_bf_ocl.exe [config_file.ini]" << std::endl;
}

int main(int argc, char *argv[])
{
    //Catch Ctrl+C
    std::signal(SIGINT, sigint_handler);

    std::cout << std::format("FlashWrite ROM encryption bruteforce utility (OpenCL) v{}", VERSION) << std::endl;
    std::string cfg_file = "crypt_bf_ocl.ini";

if (argc == 2 )
    {
        if (std::string(argv[1]) == "-h" or
            std::string(argv[1]) == "--help")
        {
            usage();
            exit(0);
        }
        else
        {
            cfg_file = argv[1];
        }
    }
    else if (argc > 2)
    {
        usage();
        exit(1);
    }

    set_options(cfg_file);
    std::string kernel_source_filename = OCL_KERNEL_SOURCES;
    std::string kernel_name = OCL_KERNEL_NAME;

    std::cout << std::format("Start key: {} {} {} {}",
                    HEX(START_K0),
                    0,
                    0,
                    0)
            << std::endl;
    std::cout << std::format("End key: {} {} {} {}",
                    HEX(END_K0),
                    0,
                    0,
                    0)
            << std::endl;

    try
    {
        //Array of found keys
        std::vector<QByteArray> key_found(GLOBAL_WI);

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

        /*I tested couple OpenCL 2.0 devices, these numbers are useless
        //cl_uint maxWorkGroupSize=100;
        std::vector<::size_t> maxWorkGroupSizes;
        //devices[0].getInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE, &maxWorkGroupSize);
        maxWorkGroupSizes = devices[0].getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
        for (int i=0; i<maxWorkGroupSizes.size(); i++)
        {
            std::cout << std::format("Maximum workgroup size for dimension {}: {}", i, maxWorkGroupSizes[i]) << std::endl;
        }
        cl_uint maxWorkGroupSize = maxWorkGroupSizes[0];
        //std::cout << std::format("Maximum workgroup size: {}", maxWorkGroupSize) << std::endl;
        */

        //Build OpenCL from sorces
        std::string kernel_source = load_file(kernel_source_filename);
        cl::Program::Sources source(1,
            std::make_pair(kernel_source.c_str(), kernel_source.length()));
        cl::Program program_ = cl::Program(context, source);
        try
        {
            program_.build(devices, "-I. -O3");
        }
        catch (...)
        {
            for (cl::Device dev : devices)
            {
                // Check the build status
                cl_build_status status = program_.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(dev);
                if (status != CL_BUILD_ERROR)
                    continue;

                // Get the build log
                std::string name     = dev.getInfo<CL_DEVICE_NAME>();
                std::string buildlog = program_.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dev);
                std::cerr << "Build log for " << name << ":" << std::endl
                            << buildlog << std::endl;
            }
        }

        std::vector<cl_ushort> keys_0_start(GLOBAL_WI),
                               keys_1_start(GLOBAL_WI),
                               keys_1_end  (GLOBAL_WI);

        std::cout << "Kernel name: " << kernel_name.c_str() << std::endl;
        //Name MUST be the same as in .cl file
        //cl::Kernel kernel(program_, "try_key_1", &err);
        cl::Kernel kernel(program_, kernel_name.c_str(), &err);

        cl::Buffer cl_buf_keys_0_start(context, CL_MEM_READ_ONLY, SIZEOF_VEC(keys_0_start));
        cl::Buffer cl_buf_keys_1_start(context, CL_MEM_READ_ONLY, SIZEOF_VEC(keys_1_start));
        cl::Buffer cl_buf_keys_1_end  (context, CL_MEM_READ_ONLY, SIZEOF_VEC(keys_1_end));
        cl::Buffer cl_buf_key_found   (context, CL_MEM_WRITE_ONLY,SIZEOF_VEC(key_found));
        
        cl::Event event;
        cl::CommandQueue queue(context, devices[0], 0, &err);
        for (int k0=START_K0; k0<=END_K0; k0++)
        { 
            if (DEBUG_LEVEL > 0)
            {
                std::cout << std::format("Runnig for key 0x{:04X} 0x{:04X} 0x{:04X} 0x{:04X}",
                                                            k0,0,0,0)
                        << std::endl;
            }
            CURRENT_KEY = k0;
            //Fill K0
            for (int k0_i=0; k0_i<keys_0_start.size(); k0_i++)
            {
                keys_0_start[k0_i] = k0;
            }
            //Prepare work items
            //Make several tasks, for example
            //keys_1_start: 0x000 0x100 0x200 ...
            //keys_1_end  : 0x0FF 0x1FF 0x2FF ...
            //Number os such pairs is GLOBAL_WI - total number of work-items, 
            int k1_buckets = (MAX+1) / keys_1_start.size();
            int k1_bucket_size = keys_1_start.size();
            for (int k1_i=0; k1_i<k1_bucket_size; k1_i++)
            {
                keys_1_start.at(k1_i) = k1_buckets*k1_i;
                keys_1_end  .at(k1_i) = k1_buckets*(k1_i+1) - 1;
            }
            queue.enqueueWriteBuffer(cl_buf_keys_0_start, CL_TRUE, 0, SIZEOF_VEC(keys_0_start), keys_0_start.data());
            queue.enqueueWriteBuffer(cl_buf_keys_1_start, CL_TRUE, 0, SIZEOF_VEC(keys_1_start), keys_1_start.data());
            queue.enqueueWriteBuffer(cl_buf_keys_1_end,   CL_TRUE, 0, SIZEOF_VEC(keys_1_end),   keys_1_end.data());

            kernel.setArg(0, cl_buf_keys_0_start);
            kernel.setArg(1, cl_buf_keys_1_start);
            kernel.setArg(2, cl_buf_keys_1_end);
            kernel.setArg(3, cl_buf_key_found);
            
            //Max work items
            int num_global = GLOBAL_WI;
            //Max work groups
            int num_local = LOCAL_WG;
            //Run
            queue.enqueueNDRangeKernel(kernel,
                                    cl::NullRange,
                                    cl::NDRange(num_global),
                                    cl::NDRange(num_local),
                                    NULL,
                                    &event);
            //Zeroize all values on a device side!
			//queue.finish();
            queue.enqueueReadBuffer(cl_buf_key_found,
                                    CL_TRUE,
                                    0,
                                    SIZEOF_VEC(key_found),
                                    key_found.data());
            event.wait();
            //Check if something was found
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
                    std::cout << "Candidate key " << key_as_string << std::endl;
                }
            }
            //Wait for key press and print current key
            if (_kbhit())
            {
                print_current_key();
                _getch();
            }
        }//k0
    }//try
    catch (cl::Error &err)
    {
        std::cerr 
           << "ERROR: "
           << err.what()
           << "("
           << err.err()
           << ")"
           << std::endl;
    }
    print_current_key();
    return 0;
}