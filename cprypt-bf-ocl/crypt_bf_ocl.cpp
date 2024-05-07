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
#include <algorithm>
#include "crypt_bf_ocl.h"

#define DEBUG_PRINT_HEX(VAR) printf(#VAR "=0x%.04x\n", VAR)
#define DEBUG_PRINT(VAR) printf(#VAR "=%d\n", VAR)
const char *VERSION = "2024.0507";

//Convert from string to integer
//If string starts with '0x', assume hex, else - dec
int _stoi(std::string s)
{
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    int radix = s.starts_with("0x") ? 16 : 10;
    return std::stoi(s, nullptr, radix);
}

int CURRENT_KEY_0,
    CURRENT_KEY_1;
//Set this to true to gracefully terminate process
bool STOP_PROCESS_NOW = false;
#define HEX(x) std::format("0x{:04X}", x)

//Print current key.
void print_current_key()
{
    int key0 = CURRENT_KEY_0,
        key1 = CURRENT_KEY_1;
    std::cout << std::format("Current key is {} {} {} {}",
                            HEX(key0), HEX(key1), 0, 0)
            << std::endl;
}

//SIGINT handler
void sigint_handler(int param)
{
    //print_current_key();
    STOP_PROCESS_NOW = true;
    std::cout << "Terminating, one moment please..." << std::endl;
    //exit(1);
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
static int OCL_PLATFORM=0;
static int OCL_DEVICE=0;

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
    CONFIG_STOI(OCL_PLATFORM);
    CONFIG_STOI(OCL_DEVICE);
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
        //std::vector<QByteArray> keys_found(GLOBAL_WI);
        std::vector<QByteArray> keys_found;
        keys_found.reserve(GLOBAL_WI);

        cl_int err = CL_SUCCESS;
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        if (platforms.size() == 0) {
            std::cout << "Platform size 0\n";
            return -1;
        }

        std::string platformVersion,
                    platformName;
        platforms[OCL_PLATFORM].getInfo(CL_PLATFORM_VERSION, &platformVersion);
        platforms[OCL_PLATFORM].getInfo(CL_PLATFORM_NAME, &platformName);
        std::cout << "Platform Info: "
                << platformName
                << " "
                << platformVersion.c_str() 
                << std::endl;

        cl_context_properties properties[] = 
            { CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[OCL_PLATFORM])(), 0};
        cl::Context context(CL_DEVICE_TYPE_DEFAULT, properties);

        std::vector<cl::Device> devices_ = context.getInfo<CL_CONTEXT_DEVICES>();

        std::string name;
        devices_[OCL_DEVICE].getInfo(CL_DEVICE_NAME, &name);
        std::cout << "Device name: " << name.c_str() << std::endl;
        std::vector<cl::Device> devices;
        devices.push_back(devices_[OCL_DEVICE]);

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
            program_.build(devices, "-I. -O5 -cl-unsafe-math-optimizations -cl-fast-relaxed-math");
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

        /*std::vector<cl_ushort> keys_0_start(GLOBAL_WI),
                               keys_1_start(GLOBAL_WI),
                               keys_1_end  (GLOBAL_WI);*/
        std::vector<cl_ushort> keys_0_start,
                               keys_1_start,
                               keys_1_end;
        keys_0_start.reserve(GLOBAL_WI);
        keys_1_start.reserve(GLOBAL_WI);
        keys_1_end.reserve(GLOBAL_WI);

        std::cout << "Kernel name: " << kernel_name.c_str() << std::endl;
        //Name MUST be the same as in .cl file
        cl::Kernel kernel(program_, kernel_name.c_str(), &err);

        /*cl::Buffer cl_buf_keys_0_start(context, CL_MEM_READ_ONLY, SIZEOF_VEC(keys_0_start));
        cl::Buffer cl_buf_keys_1_start(context, CL_MEM_READ_ONLY, SIZEOF_VEC(keys_1_start));
        cl::Buffer cl_buf_keys_1_end  (context, CL_MEM_READ_ONLY, SIZEOF_VEC(keys_1_end));
        cl::Buffer cl_buf_keys_found   (context, CL_MEM_WRITE_ONLY,SIZEOF_VEC(keys_found));*/
        
        cl::Event event;
        cl::CommandQueue queue(context, devices[OCL_DEVICE], 0, &err);
        for (int k0=START_K0; k0<=END_K0; k0++)
        { 
            CURRENT_KEY_0 = k0;
            size_t keys_left = END_K0 - k0 + 1;
            //Vector size
            size_t vector_elements = std::min(keys_left*(MAX+1), size_t(GLOBAL_WI));
            //How many key0 we send during this iteration
            size_t keys0_send = std::min(keys_left, vector_elements / (MAX+1));
            //If number of elements in vector is small, we still send 1 key0
            if (keys0_send == 0)
            {
                keys0_send = 1;
            }
            try
            {
                //Resize vectors to fit all elements at once
                keys_0_start.resize(vector_elements);
                keys_1_start.resize(vector_elements);
                keys_1_end.resize(vector_elements);
                keys_found.resize(vector_elements);
            }
            catch (...)
            {
                std::cout << "Failed to resize vectors." << std::endl;
                exit(1);
            }
            
            //Prepare work items
            //Make several tasks, for example
            //keys_0_start: 0x0000 0x0000 0x0000 ... 0x0000 0x0001 ...
            //keys_1_start: 0x0000 0x1000 0x2000 ... 0xF000 0x0000 ...
            //keys_1_end  : 0x0FFF 0x1FFF 0x2FFF ... 0xFFFF 0x0FFF ...
            //Number os such pairs should equal to total number of work-items,
            //that can run simutaneously on OpenCL device          
            //We can run more or less than 65536 work-items,
            //prepeare for this
            //Be careful with those loops - the algorithm turned out to be quite confusing
            for (int k0_i=0; k0_i<keys0_send; k0_i++)
            {
                //If we cannot run all key1 values at once, divide the work
                int bucket_size = std::min(MAX + 1, int( keys_1_start.size() ));
                //How many work-items we can run at once
                int number_of_buckets = (MAX+1) / bucket_size;

                //If number of work-items are less that 65536,
                //process every bucket
                for (int k1_i=0; k1_i<number_of_buckets; k1_i++)
                {
                    CURRENT_KEY_1 = k1_i * bucket_size;
                    
                    //Fill key0 vector
                    //First put k0 how much is needed times, then k0+1 etc.
                    for (int k0_j=0; k0_j<bucket_size; k0_j++)
                    {
                        keys_0_start.at(k0_j + k0_i*bucket_size) = k0+k0_i;
                    }
                    //Fill key1 start and end vectors
                    //We send only one key1 but with portions
                    //if number of work-items are less then 65536
                    for (int k1_j=0; k1_j<bucket_size; k1_j++)
                    {
                        int _key1 = k1_j + bucket_size * k1_i;
                        keys_1_start.at(k1_j + k0_i*bucket_size) = _key1;
                        keys_1_end  .at(k1_j + k0_i*bucket_size) = _key1;
                    }//k1_j

                    //If we process more than 1 key0 at once, submit job only once at end of loop
                    if (keys0_send == (k0_i + 1))
                    {
                        //If we process more than 1 key0 at once, print info on every key only once at end of loop
                        if (DEBUG_LEVEL > 0)
                        {
                            std::cout << std::format("Runnig for key 0x{:04X} 0x{:04X} 0x{:04X} 0x{:04X}",
                                                                        k0,CURRENT_KEY_1,0,0)
                                    << std::endl;
                        }

                        cl_int key_was_found = 0;

                        //We don’t know in advance size of data, it can change at least at last iteration
                        cl::Buffer cl_buf_keys_0_start(context, CL_MEM_READ_ONLY, SIZEOF_VEC(keys_0_start));
                        cl::Buffer cl_buf_keys_1_start(context, CL_MEM_READ_ONLY, SIZEOF_VEC(keys_1_start));
                        cl::Buffer cl_buf_keys_1_end  (context, CL_MEM_READ_ONLY, SIZEOF_VEC(keys_1_end));
                        cl::Buffer cl_buf_keys_found  (context, CL_MEM_WRITE_ONLY,SIZEOF_VEC(keys_found));
                        cl::Buffer cl_buf_is_key_found(context, CL_MEM_WRITE_ONLY,sizeof(key_was_found));

                        queue.enqueueWriteBuffer(cl_buf_keys_0_start, CL_FALSE, 0, SIZEOF_VEC(keys_0_start), keys_0_start.data());
                        queue.enqueueWriteBuffer(cl_buf_keys_1_start, CL_FALSE, 0, SIZEOF_VEC(keys_1_start), keys_1_start.data());
                        queue.enqueueWriteBuffer(cl_buf_keys_1_end,   CL_FALSE, 0, SIZEOF_VEC(keys_1_end),   keys_1_end.data());

                        kernel.setArg(0, cl_buf_keys_0_start);
                        kernel.setArg(1, cl_buf_keys_1_start);
                        kernel.setArg(2, cl_buf_keys_1_end);
                        kernel.setArg(3, cl_buf_keys_found);
                        kernel.setArg(4, cl_buf_is_key_found);
                        
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
                        queue.enqueueReadBuffer(cl_buf_keys_found,
                                                CL_FALSE,
                                                0,
                                                SIZEOF_VEC(keys_found),
                                                keys_found.data());
                        queue.enqueueReadBuffer(cl_buf_is_key_found,
                                                CL_FALSE,
                                                0,
                                                sizeof(key_was_found),
                                                &key_was_found);
                        event.wait();
                        //Check if something was found
                        if (key_was_found)
                        {
                            for (int i=0; i<keys_found.size(); i++)
                            {
                                bool all_words_are_zeroes = true;
                                std::string key_as_string = "";
                                for (int j=0; j<keys_found.at(i).size(); j++)
                                {
                                    int key_word = keys_found.at(i).at(j);
                                    all_words_are_zeroes = all_words_are_zeroes && (key_word == 0);
                                    key_as_string += HEX(key_word) + " ";
                                }
                                if (!all_words_are_zeroes)
                                {
                                    std::cout << "Candidate key " << key_as_string << std::endl;
                                }
                            }
                        }
                        //Wait for key press and print current key
                        if (_kbhit())
                        {
                            print_current_key();
                            _getch();
                        }
                        if (STOP_PROCESS_NOW)
                        {
                            //break;
                            goto exit;
                        }
                    }
                }//k1_i
            }//k0_i
            //We submitted several keys - increase counter
            k0 += keys0_send - 1;
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
    exit:
    print_current_key();
    return 0;
}