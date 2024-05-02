// This is an independent project of an individual developer. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
//This program is made in educational purposes.
//Using in another purposes is prohibited!
#include <iostream>
#include <vector>
#include <cstdint>
#include <format>
#include <csignal>
#include <string>
#include <functional>
#include <conio.h>
#include "crypt_utils.h"

#define HEX(x) std::format("0x{:04X}", x)
#define MAX 0xffff
#define FOR(n) for(uint32_t n=0; n<=MAX; n++)
#define WRITELN(i) std::cout << std::format("{} ",i) << std::endl;
#define WRITE(i) std::cout << std::format("{} ",i)

const char *VERSION = "2024.0426";

uint16_t key_to_generate_index[4]={0};

void print_current_key()
{
    std::cout << std::format("Current key is {} {} {} {}",
                            HEX(key_to_generate_index[0]),
                            HEX(key_to_generate_index[1]),
                            HEX(key_to_generate_index[2]),
                            HEX(key_to_generate_index[3]))
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
    std::cout << "crypt_bf.exe [key_word0_start key_word1_start key_word2_start key_word3_start [key_word0_end key_word1_end key_word2_end key_word3_end]]" << std::endl;
}

int main(int argc, char *argv[])
{
    //Set this for bruteforce. 4 bytes give max speed but many false positives.
    //Clean unencrypted bytes
    const QByteArray clean_buf = {0xff, 0xff, 0xff, 0xff};
    //Encrypted bytes
    const QByteArray encr_buf = {0x68, 0xFC, 0x4E, 0x1C};

    //Set this to discard false positives
    //Clean unencrypted bytes
    const QByteArray clean_test_buf = {0xFF, 0x00, 0x03, 0x00};
    //Encrypted bytes
    const QByteArray encr_test_buf  = {0x85, 0x6F, 0x15, 0x7C};
    QByteArray r;
    uint32_t key_word0_start = 0,
             key_word1_start = 0,
             key_word2_start = 0,
             key_word3_start = 0,
             key_word0_end = MAX,
             key_word1_end = MAX,
             key_word2_end = MAX,
             key_word3_end = MAX;
    bool just_started = true;

    //Set start key
    std::function<void()> set_key_start = [&](){
        key_word0_start = _stoi(argv[1]);
        key_word1_start = _stoi(argv[2]);
        key_word2_start = _stoi(argv[3]);
        key_word3_start = _stoi(argv[4]);
    };
    //set end key
    std::function<void()> set_key_end = [&](){
        key_word0_end = _stoi(argv[5]);
        key_word1_end = _stoi(argv[6]);
        key_word2_end = _stoi(argv[7]);
        key_word3_end = _stoi(argv[8]);
    };

    std::cout << std::format("FlashWrite ROM encryption bruteforce utility v{}", VERSION) << std::endl;

    if (argc == 5 )
    {
        set_key_start();
    }
    else if (argc == 9 )
    {
        set_key_start();
        set_key_end();
    }
    else if (argc != 1)
    {
        usage();
        exit(1);
    }

    std::cout << std::format("Start key: {} {} {} {}",
                    HEX(key_word0_start),
                    HEX(key_word1_start),
                    HEX(key_word2_start),
                    HEX(key_word3_start))
            << std::endl;
    std::cout << std::format("End key: {} {} {} {}",
                    HEX(key_word0_end),
                    HEX(key_word1_end),
                    HEX(key_word2_end),
                    HEX(key_word3_end))
            << std::endl;

    //Catch Ctrl+C
    std::signal(SIGINT, sigint_handler);

    //for (int i0=0; i0<=key_word0_end; i0++)
    FOR(i0)
    {
        if (just_started)
            i0 = key_word0_start;
        FOR(i1)
        {
            if (just_started)
                i1 = key_word1_start;
            FOR(i2)
            {
                if (just_started)
                    i2 = key_word2_start;
                FOR(i3)
                {
                    if (just_started)
                    {
                        i3 = key_word3_start;
                        just_started = false;
                    }
                    key_to_generate_index[0]=i0;
                    key_to_generate_index[1]=i1;
                    key_to_generate_index[2]=i2;
                    key_to_generate_index[3]=i3;
                    subaru_denso_decrypt_32bit_payload(encr_buf, key_to_generate_index, r);
                    //Check if buffer is properly decrypted
                    if (compare(r, clean_buf))
                    {
                        //Check if this key can encrypt
                        uint16_t encr_key []= { key_to_generate_index[3],
                                                key_to_generate_index[2],
                                                key_to_generate_index[1],
                                                key_to_generate_index[0]};
                        subaru_denso_encrypt_32bit_payload(clean_buf, encr_key, r);
                        //Check if buffer is properly encrypted
                        if (compare(r, encr_buf))
                        {
                            //Finally, check if this key can decrypt other data
                            subaru_denso_decrypt_32bit_payload(encr_test_buf, key_to_generate_index, r);
                            if (compare(r, clean_test_buf))
                            {
                                std::cout
                                    << std::format("Candidate key is {} {} {} {}",
                                                    HEX(key_to_generate_index[0]),
                                                    HEX(key_to_generate_index[1]),
                                                    HEX(key_to_generate_index[2]),
                                                    HEX(key_to_generate_index[3]))
                                    << std::endl;
                            }
                        }
                    }
                    if (i0>=key_word0_end and
                        i1>=key_word1_end and
                        i2>=key_word2_end and
                        i3>=key_word3_end)
                            goto LoopExit;

                } //i3
                //Check if any key is pressed
                if (_kbhit())
                {
                    print_current_key();
                    _getch();
                }
            } //i2
        } //i1
        WRITELN("");
    } //i0
    LoopExit:

    print_current_key();
    return 0;
}
