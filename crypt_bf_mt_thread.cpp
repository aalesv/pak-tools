// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include <iostream>
#include <format>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include "crypt_bf_mt_thread.h"
#include "crypt_utils.h"

//Number of workers currently running
static std::atomic_int g_workers_running = 0;
//Indicates that all theads are finished
static std::atomic_bool g_thread_pool_finished = true;
//First available key to bruteforce
static std::atomic_int g_current_key;
//Print to stdout mutex
static std::mutex g_print_lock;

static QByteArray ENCR_BUF;
static QByteArray CLEAN_BUF;
static QByteArray ENCR_TEST_BUF;
static QByteArray CLEAN_TEST_BUF;
static constexpr QByteArray mask_00_00_FF_FF = {0x0, 0x0, 0xFF, 0xFF};
static int DEBUG_LEVEL;

//Thread worker
//Checks keys of the following type K0 ???? ???? 0
//Key K0 K1 K2 0 decodes last 2 bytes correctly
//We need to find 3 words and only then find K3
static void bf_thread_worker(int key0)
{
    uint16_t key_to_generate_index[4];
    const QByteArray encr_buf = ENCR_BUF;
    const QByteArray clean_buf = CLEAN_BUF;
    const QByteArray encr_test_buf = ENCR_TEST_BUF;
    const QByteArray clean_test_buf = CLEAN_TEST_BUF;
    
    //Increase counter to everybody know how many workers are running
    g_workers_running++;
    if (DEBUG_LEVEL)
    {
        std::scoped_lock lock(g_print_lock);
        std::cout << std::format("Worker {} started", HEX(key0)) << std::endl;
    }
    FOR(i1)
    {
        FOR(i2)
        {
            key_to_generate_index[0]=key0;
            key_to_generate_index[1]=i1;
            key_to_generate_index[2]=i2;
            key_to_generate_index[3]=0;
            QByteArray r;
            subaru_denso_decrypt_32bit_payload(encr_buf, key_to_generate_index, r);
            //Check if buffer is partially decrypted
            if (compare(r, clean_buf, mask_00_00_FF_FF))
            {
                //Check if other data can be partially decrypted
                subaru_denso_decrypt_32bit_payload(encr_test_buf, key_to_generate_index, r);
                if (compare(r, clean_test_buf, mask_00_00_FF_FF))
                {
                    if (DEBUG_LEVEL > 2)
                    {
                        std::scoped_lock lock(g_print_lock);
                        std::cout
                            << std::format("Narrowing search to key range {} {} {} ???",
                                            HEX(key_to_generate_index[0]),
                                            HEX(key_to_generate_index[1]),
                                            HEX(key_to_generate_index[2]))
                            << std::endl;
                    }
                    //Search for last word K3
                    FOR (i3)
                    {
                        key_to_generate_index[3] = i3;
                        bool b;
                        b = subaru_denso_decrypt_32bit_payload(encr_buf, key_to_generate_index, r, clean_buf);
                        /*uint16_t reverse_key_to_generate_index[4];
                        std::reverse_copy(std::begin(key_to_generate_index),
                                        std::end(key_to_generate_index),
                                        reverse_key_to_generate_index);
                        b = subaru_denso_encrypt_32bit_payload(clean_buf, reverse_key_to_generate_index, r, encr_buf);
                        */
                        //Check if buffer is properly decrypted
                        //if (b and compare(r, encr_buf))
                        if (b and compare(r, clean_buf))
                        {
                            //Finally, check if this key can decrypt other data
                            subaru_denso_decrypt_32bit_payload(encr_test_buf, key_to_generate_index, r);
                            if (compare(r, clean_test_buf))
                            {
                                std::scoped_lock lock(g_print_lock);
                                std::cout
                                    << std::format("Candidate key is {} {} {} {}",
                                                    HEX(key_to_generate_index[0]),
                                                    HEX(key_to_generate_index[1]),
                                                    HEX(key_to_generate_index[2]),
                                                    HEX(key_to_generate_index[3]))
                                    << std::endl;
                            }
                        }
                    }//i3
                }
            }
        } //i2
    } //i1
    //Decrease counter to everybody know how many workers are running
    g_workers_running--;
    if (DEBUG_LEVEL)
    {
        std::scoped_lock lock(g_print_lock);
        std::cout << std::format("Worker {} ended", HEX(key0)) << std::endl;
    }
}

//Checks if thread pool has free slot. If not, sleeps for ms milliseconds
static void wait_thread_pool_free_slot(int max, int ms)
{
    while (g_workers_running >= max)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

//Checks if all workers finished. If not, sleeps for ms milliseconds
static void wait_thread_pool_to_finish(int ms)
{
    while (g_workers_running > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

//Thread pool
static void bf_thread_pool(int thread_count, int start_k0, int end_k0)
{
    constexpr auto ms_to_wait = 10;
    g_thread_pool_finished = false;
    //How many thread can run OS (like how many CPUs)
    int hw_c = std::thread::hardware_concurrency();
    //If it cannot be determined, use 1 thread
    int os_max_threads = hw_c == 0 ? 1 : hw_c;
    //Autodetect number of threads or use defined by user
    int max_threads = thread_count == 0 ? os_max_threads : thread_count;
    
    if (DEBUG_LEVEL)
    {
        std::scoped_lock lock(g_print_lock);
        WRITELN("Starting thread pool");
        std::cout << std::format("Thread pool size is {}", max_threads) << std::endl;
    }
    //Launch threads for every key
    for (int i=start_k0; i<=end_k0; i++)
    {
        wait_thread_pool_free_slot(max_threads, ms_to_wait);
        g_current_key = i;
        if (DEBUG_LEVEL > 1)
        {
            std::scoped_lock lock(g_print_lock);
            std::cout << std::format("Starting worker {}", HEX(i)) << std::endl;
        }
        std::thread(bf_thread_worker, i).detach();    
        //Let threads start
        std::this_thread::sleep_for(std::chrono::milliseconds(ms_to_wait));
        if (DEBUG_LEVEL > 1)
        {  
            int n = g_workers_running;
            std::scoped_lock lock(g_print_lock);
            std::cout << std::format("{} active worker(s)", n) << std::endl;
        }
    }
    if (DEBUG_LEVEL)
    {
        std::scoped_lock lock(g_print_lock);
        WRITELN("All workers launched, waiting");
    }
    //Wait until all workers finish
    wait_thread_pool_to_finish(ms_to_wait);
    if (DEBUG_LEVEL)
    {
        std::scoped_lock lock(g_print_lock);
        WRITELN("Stopping thread pool");
    }
    g_thread_pool_finished = true;
}

void start_bf_thread_pool(ThreadInit init)
{
    DEBUG_LEVEL = init.debug_level;
    ENCR_BUF = init.encr_buf;
    CLEAN_BUF = init.clean_buf;
    ENCR_TEST_BUF = init.encr_test_buf;
    CLEAN_TEST_BUF = init.clean_test_buf;
    
    if (DEBUG_LEVEL > 1)
    {
        std::scoped_lock lock(g_print_lock);
        std::cout << std::format("Workers={}, start key={}, end key={}",
                                init.thread_count,
                                HEX(init.start_k0),
                                HEX(init.end_k0))
                    << std::endl;
    }
    
    //Launch thread pool
    std::thread(bf_thread_pool, init.thread_count, init.start_k0, init.end_k0).detach();
}

//Return current key.
int get_current_key()
{
    //Due to multiple threads we cannot be more precise
    int key = g_current_key - g_workers_running + 1;
    return key;
}

bool thread_pool_finished()
{
    bool f = g_thread_pool_finished;
    return f;
}
