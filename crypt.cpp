// This is an independent project of an individual developer. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include <iostream>
#include <format>
#include <fstream>
#include <cstdint>
#include <algorithm>
#include "crypt_utils.h"

const char *VERSION = "2024.0423";

void usage()
{
    std::cout << "Usage:" << std::endl;
    std::cout << "crypt.exe <infile> <key_word0> <key_word1> <key_word2> <key_word3> [<outfile>]" << std::endl;
}

int main( int argc, char* argv[])
{
    QByteArray buffer;
    uint16_t crypt_key[4];
    std::string in_file_name,
                out_file_name = "out.bin";
    
    std::cerr << std::format("Subaru ROM encrypt\\decrypt utility v{}", VERSION) << std::endl;

    if (argc < 6 or argc > 7)
    {
        std::cerr << "Wrong number of arguments." << std::endl;
        usage();
        return 1;
    }
    in_file_name = argv[1];
    crypt_key[0] = stoi(argv[2]);
    crypt_key[1] = stoi(argv[3]);
    crypt_key[2] = stoi(argv[4]);
    crypt_key[3] = stoi(argv[5]);
    if (argc == 7)
    {
        out_file_name = argv[6];
    }

    std::ifstream inFile(in_file_name, std::ifstream::binary);
    if (!inFile)
        throw std::runtime_error(std::format("Error opening file {}", in_file_name));
    std::ofstream outFile(out_file_name, std::ios::out | std::ios::binary);
    if (!outFile)
        throw std::runtime_error(std::format("Error opening file {}", out_file_name));
    while (!inFile.eof())
    {
        inFile.read((char *)buffer.data(), buffer.size());
        QByteArray r = subaru_denso_decrypt_32bit_payload(buffer, crypt_key);
        auto len = inFile.gcount();
        outFile.write((const char*)&r[0], len);
    }
    std::cout << std::format("Wrote to {}", out_file_name) << std::endl;
    return 0;
}
