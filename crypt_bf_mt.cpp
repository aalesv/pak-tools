// This is an independent project of an individual developer. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
//This program is made in educational purposes.
//Using in another purposes is prohibited!
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <format>
#include <csignal>
#include <string>
#include <functional>
#include <conio.h>
#include <map>
#include <sstream>
#include <thread>
#include <chrono>
#include "crypt_utils.h"
#include "crypt_bf_mt_thread.h"

const char *VERSION = "2024.0426";

//Options
static std::map<std::string, std::string> OPTIONS;
//Config file default name
static std::string cfg_file = "crypt_bf_mt.ini";

//Options
// 0 to autodetect
static int THREAD_COUNT;
static int DEBUG_LEVEL;
//Start key high word
static uint16_t START_K0 = 0,
                //End key high word
                END_K0 = 0xFFFF;
//Clean unencrypted known buffer words
static uint8_t  CLEAN_BUF_K0,
                CLEAN_BUF_K1,
                CLEAN_BUF_K2,
                CLEAN_BUF_K3,
                //Encrypted known buffer words
                ENCR_BUF_K0,
                ENCR_BUF_K1,
                ENCR_BUF_K2,
                ENCR_BUF_K3,
                //Another clean unencrypted known buffer words
                CLEAN_TEST_BUF_K0,
                CLEAN_TEST_BUF_K1,
                CLEAN_TEST_BUF_K2,
                CLEAN_TEST_BUF_K3,
                //Another encrypted known buffer words
                ENCR_TEST_BUF_K0,
                ENCR_TEST_BUF_K1,
                ENCR_TEST_BUF_K2,
                ENCR_TEST_BUF_K3;
static QByteArray   CLEAN_BUF,
                    ENCR_BUF,
                    CLEAN_TEST_BUF,
                    ENCR_TEST_BUF;

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
    CONFIG_STOI(THREAD_COUNT);
    CONFIG_STOI(START_K0);
    CONFIG_STOI(END_K0);
    CONFIG_STOI(CLEAN_BUF_K0);
    CONFIG_STOI(CLEAN_BUF_K1);
    CONFIG_STOI(CLEAN_BUF_K2);
    CONFIG_STOI(CLEAN_BUF_K3);
    CONFIG_STOI(ENCR_BUF_K0);
    CONFIG_STOI(ENCR_BUF_K1);
    CONFIG_STOI(ENCR_BUF_K2);
    CONFIG_STOI(ENCR_BUF_K3);
    CONFIG_STOI(CLEAN_TEST_BUF_K0);
    CONFIG_STOI(CLEAN_TEST_BUF_K1);
    CONFIG_STOI(CLEAN_TEST_BUF_K2);
    CONFIG_STOI(CLEAN_TEST_BUF_K3);
    CONFIG_STOI(ENCR_TEST_BUF_K0);
    CONFIG_STOI(ENCR_TEST_BUF_K1);
    CONFIG_STOI(ENCR_TEST_BUF_K2);
    CONFIG_STOI(ENCR_TEST_BUF_K3);
    CONFIG_STOI(DEBUG_LEVEL);
    CLEAN_BUF[0] = CLEAN_BUF_K0;
    CLEAN_BUF[1] = CLEAN_BUF_K1;
    CLEAN_BUF[2] = CLEAN_BUF_K2;
    CLEAN_BUF[3] = CLEAN_BUF_K3;
    ENCR_BUF[0] = ENCR_BUF_K0;
    ENCR_BUF[1] = ENCR_BUF_K1;
    ENCR_BUF[2] = ENCR_BUF_K2;
    ENCR_BUF[3] = ENCR_BUF_K3;
    CLEAN_TEST_BUF[0] = CLEAN_TEST_BUF_K0;
    CLEAN_TEST_BUF[1] = CLEAN_TEST_BUF_K1;
    CLEAN_TEST_BUF[2] = CLEAN_TEST_BUF_K2;
    CLEAN_TEST_BUF[3] = CLEAN_TEST_BUF_K3;
    ENCR_TEST_BUF[0] = ENCR_TEST_BUF_K0;
    ENCR_TEST_BUF[1] = ENCR_TEST_BUF_K1;
    ENCR_TEST_BUF[2] = ENCR_TEST_BUF_K2;
    ENCR_TEST_BUF[3] = ENCR_TEST_BUF_K3;
}

//Print current key.
void print_current_key()
{
    int key = get_current_key();
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

//Print usage info
void usage ()
{
    std::cout << "Usage:" << std:: endl;
    std::cout << "crypt_bf_mt.exe [config_file.ini]" << std::endl;
}

int main(int argc, char *argv[])
{
    std::cout << std::format("FlashWrite ROM encryption bruteforce utility v{}", VERSION) << std::endl;

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

    //Catch Ctrl+C
    std::signal(SIGINT, sigint_handler);

    ThreadInit init;
    init.thread_count = THREAD_COUNT;
    init.start_k0 = START_K0;
    init.end_k0 = END_K0;
    init.debug_level = DEBUG_LEVEL;
    init.encr_buf = ENCR_BUF;
    init.clean_buf = CLEAN_BUF;
    init.encr_test_buf = ENCR_TEST_BUF;
    init.clean_test_buf = CLEAN_TEST_BUF;

    start_bf_thread_pool(init);
    //Let thread pool start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    //Wait until thread pool finishes
    while (!thread_pool_finished())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        //If a key is pressed, print current key
        if (_kbhit())
        {
            print_current_key();
            _getch();
        }
    }

    print_current_key();
    return 0;
}
