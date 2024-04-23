// This is an independent project of an individual developer. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include <iostream>
#include <vector>
#include <cstdint>
#include <format>
#include <csignal>
#include <string>
#include <functional>
#include <conio.h>
#include "crypt_utils.h"

//Return true if both arrays are indentical
static inline bool compare(const QByteArray &first, const QByteArray &second)
{
    //if (first.size() != second.size())
    //    throw std::runtime_error("Can't compare arrays of different size");
    
    bool res=true;
    for (int i=0; i<first.size(); i++)
    //for (int i=0; i<1; i++)
    {
        res = (first[i] == second[i]) && res;
        if (! res) break;
    }
    //#define CMP(a,b,i) a[i]==b[i]
    //res = (first[3] == second[3]) && (first[2] == second[2]) && (first[1] == second[1]) && (first[0] == second[0]);

    //res = CMP(first,second,3) && CMP(first,second,2) && CMP(first,second,1) &&CMP(first,second,0);

    return res;
}

#define HEX(x) std::format("0x{:04X}", x)
#define MAX 0xffff
#define FOR(n) for(uint32_t n=0; n<=MAX; n++)
#define WRITELN(i) std::cout << std::format("{} ",i) << std::endl;
#define WRITE(i) std::cout << std::format("{} ",i)

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
    std::cout << "FlashWrite ROM encryption bruteforce" << std::endl;
    std::cout << "Usage:" << std:: endl;
    std::cout << "crypt_bf.exe [key_word0_start key_word1_start key_word2_start key_word3_start [key_word0_end key_word1_end key_word2_end key_word3_end]]" << std::endl;
}

int main(int argc, char *argv[])
{
    //Set this for bruteforce. 4 bytes give max speed but many false positives.
    //Clean unencrypted bytes
    const QByteArray clean_buf = {0xff, 0xff, 0xff, 0xff};
    //Encrypted bytes
    const QByteArray encr_buf = {0x96, 0x55, 0x4f, 0x6e};

    //Set this to discard false positives
    //Clean unencrypted bytes
    const QByteArray clean_test_buf = {0xFF, 0xFF, 0x20, 0x4C};
    //Encrypted bytes
    const QByteArray encr_test_buf  = {0xFE, 0x77, 0x09, 0x27};
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
        key_word0_start = stoi(argv[1]);
        key_word1_start = stoi(argv[2]);
        key_word2_start = stoi(argv[3]);
        key_word3_start = stoi(argv[4]);
    };
    //set end key
    std::function<void()> set_key_end = [&](){
        key_word0_end = stoi(argv[5]);
        key_word1_end = stoi(argv[6]);
        key_word2_end = stoi(argv[7]);
        key_word3_end = stoi(argv[8]);
    };

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
                    r = subaru_denso_decrypt_32bit_payload(encr_buf, key_to_generate_index);
                    //Check if buffer is properly decrypted
                    if (compare(r, clean_buf))
                    {
                        //Check if this key can encrypt
                        uint16_t encr_key []= { key_to_generate_index[3],
                                                key_to_generate_index[2],
                                                key_to_generate_index[1],
                                                key_to_generate_index[0]};
                        r = subaru_denso_encrypt_32bit_payload(clean_buf, encr_key);
                        //Check if buffer is properly encrypted
                        if (compare(r, encr_buf))
                        {
                            //Finally, check if this key can decrypt other data
                            r = subaru_denso_decrypt_32bit_payload(encr_test_buf, key_to_generate_index);
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
