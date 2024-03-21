#include <iostream>
#include <format>
#include <fstream>
#include <vector>
#include <windows.h>
#include <wincrypt.h>

#define THROW_IF_FALSE(winbool, where) if (winbool == FALSE) \
        throw std::runtime_error(std::format(where " failed with error {}", GetLastError()))

const char *VERSION = "2024.0321";

void
usage()
{
    std::cerr << "Usage:\nrc2decode.exe <infile> <key> [outfile]" << std::endl;
}

int
main( int argc, char* argv[])
{
    char *file_in;
    std::string file_out = "out.txt";
    LPCSTR key;
    const LPCSTR CRYPTO_PROVIDER = "Microsoft Base Cryptographic Provider v1.0";


    std::cerr << std::format("rc2 Subaru PAK decrypt utility v{}", VERSION) << std::endl;
    if (argc < 2 or argc > 4)
    {
        std::cerr << "Wrong number of arguments." << std::endl;
        usage();
        return 1;
    }

    file_in = argv[1];
    key = argv[2];
    if (argc == 4)
        file_out = argv[3];

    HCRYPTPROV *phProv = new HCRYPTPROV;

    WINBOOL r;
    r = CryptAcquireContextA(phProv, NULL, CRYPTO_PROVIDER, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    THROW_IF_FALSE(r, "CryptAcquireContextA");

    HCRYPTHASH *phHash = new HCRYPTHASH;
    r = CryptCreateHash(*phProv, CALG_MD5, 0, 0, phHash);
    THROW_IF_FALSE(r, "CryptCreateHash");

    r = CryptHashData(*phHash, (BYTE *) key, strlen(key), 0);
    THROW_IF_FALSE(r, "CryptHashData");

    HCRYPTKEY *phKey = new HCRYPTKEY;
    r = CryptDeriveKey(*phProv, CALG_RC2, *phHash, 0, phKey);
    THROW_IF_FALSE(r, "CryptDeriveKey");

    std::vector<BYTE> buffer (1024, 0);  
    std::ifstream inFile(file_in, std::ifstream::binary);
    std::ofstream outFile(file_out, std::ios::out | std::ios::binary);
    WINBOOL final;
    while (!inFile.eof())
    {
        inFile.read((char *)buffer.data(), buffer.size());
        //How much did we read
        DWORD len = inFile.gcount();
        //Is this final pass?
        final = (len < 1024);
        r = CryptDecrypt(*phKey, 0, final, 0, (BYTE *)&buffer[0], &len);
        THROW_IF_FALSE((r || final), "CryptDecrypt"); 
        outFile.write((const char*)&buffer[0], len);
    }
    std::cout << std::format("Writed to {}", file_out) << std::endl;
    return 0;
}
