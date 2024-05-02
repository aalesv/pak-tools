// This is an independent project of an individual developer. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include <cstdint>
#include <array>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <format>
#include <iostream>
#include "crypt_utils.h"

//Crypto algo implementations borrowed from FastECU by miikasyvanen

const uint8_t index_transformation[]={
    0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
    0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
    0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
    0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
};

//Encrypt and decrypt function. Reversed key is decrypt key.
//[k0, k1, k2, k3] is encrypt key and [k3, k2, k1, k0] is decrypt key.
//Returns true if encryption/ecryption was successfull
//This function always returns true
bool subaru_denso_calculate_32bit_payload(const QByteArray &buf, 
                                        const uint16_t *key_to_generate_index,
                                        const uint8_t *index_transformation,
                                        QByteArray &encrypted)
{
    //QByteArray encrypted;
    uint32_t data_to_encrypt32, index;
    uint16_t word_to_generate_index, word_to_be_encrypted, encryption_key;
    int ki, n;

    for (uint32_t i = 0; i < buf.size(); i += 4)
    {
        data_to_encrypt32 = ((buf[i] << 24) & 0xFF000000) | ((buf[i + 1] << 16) & 0xFF0000) | ((buf[i + 2] << 8) & 0xFF00) | (buf[i + 3] & 0xFF);

        for (ki = 0; ki < 4; ki++)
        {

            word_to_generate_index = data_to_encrypt32;
            word_to_be_encrypted = data_to_encrypt32 >> 16;
            index = word_to_generate_index ^ key_to_generate_index[ki];
            index += index << 16;
            encryption_key = 0;

            for (n = 0; n < 4; n++)
            {
                encryption_key += index_transformation[(index >> (n * 4)) & 0x1F] << (n * 4);
            }

            encryption_key = (encryption_key >> 3) + (encryption_key << 13);
            data_to_encrypt32 = (encryption_key ^ word_to_be_encrypted) + (word_to_generate_index << 16);
        }

        data_to_encrypt32 = (data_to_encrypt32 >> 16) + (data_to_encrypt32 << 16);

        //encrypted.push_back((data_to_encrypt32 >> 24) & 0xFF);
        //encrypted.push_back((data_to_encrypt32 >> 16) & 0xFF);
        //encrypted.push_back((data_to_encrypt32 >> 8) & 0xFF);
        //encrypted.push_back(data_to_encrypt32 & 0xFF);
        encrypted[0]=((data_to_encrypt32 >> 24) & 0xFF);
        encrypted[1]=((data_to_encrypt32 >> 16) & 0xFF);
        encrypted[2]=((data_to_encrypt32 >> 8) & 0xFF);
        encrypted[3]=(data_to_encrypt32 & 0xFF);

    }
    //return encrypted;
    return true;
}

//Encrypt and decrypt function. Reversed key is decrypt key.
//[k0, k1, k2, k3] is encrypt key and [k3, k2, k1, k0] is decrypt key.
//Returns true if encryption/ecryption was successfull
//Bruteforce version. Use only if decrypted equals 0xFFFFFFFF or 0x00000000
//Additional odd/even test is made
bool subaru_denso_calculate_32bit_payload(const QByteArray &buf, 
                                        const uint16_t *key_to_generate_index,
                                        const uint8_t *index_transformation,
                                        QByteArray &encrypted,
                                        const QByteArray &decrypted)
{
    uint32_t data_to_encrypt32, index;
    uint16_t word_to_generate_index, word_to_be_encrypted, encryption_key;
    int ki, n;

    uint32_t decoded_data;

    for (uint32_t i = 0; i < buf.size(); i += 4)
    {
        data_to_encrypt32 = ((buf[i] << 24) & 0xFF000000) | ((buf[i + 1] << 16) & 0xFF0000) | ((buf[i + 2] << 8) & 0xFF00) | (buf[i + 3] & 0xFF);

        decoded_data = ((decrypted[i] << 24) & 0xFF000000) | ((decrypted[i + 1] << 16) & 0xFF0000) | ((decrypted[i + 2] << 8) & 0xFF00) | (decrypted[i + 3] & 0xFF);

        //Additional odd/even test start
        //But only if decoded_data eq 0xFFFFFFFF or 0x00000000
        /*if ( (decoded_data != 0xFFFFFFFF) and (decoded_data != 0) )
        {
            throw std::runtime_error((std::format("{}: incorrect decrypted value: {}. Must be 0xFFFFFFFF or 0x00000000", __FUNCTION__, HEX(decoded_data))));
        }*/
        //Additional odd/even test end
        
        for (ki = 0; ki < 4; ki++)
        {

            word_to_generate_index = data_to_encrypt32;
            word_to_be_encrypted = data_to_encrypt32 >> 16;
            index = word_to_generate_index ^ key_to_generate_index[ki];
            index += index << 16;
            encryption_key = 0;

            //Additional odd/even test start
            //n=0, see later for loop
            encryption_key = index_transformation[index & 0x1F]; //-V519

            if (ki == 2)
            {
                if(!((decoded_data % 2) ^ ((encryption_key >> 3) % 2)))
                {
                    return false;
                }
            }
            //Additional odd/even test end

            //Value for n=0 calculated earlier
            for (n = 1; n < 4; n++)
            {
                encryption_key += index_transformation[(index >> (n * 4)) & 0x1F] << (n * 4);
            }

            encryption_key = (encryption_key >> 3) + (encryption_key << 13);
            data_to_encrypt32 = (encryption_key ^ word_to_be_encrypted) + (word_to_generate_index << 16);
        }

        data_to_encrypt32 = (data_to_encrypt32 >> 16) + (data_to_encrypt32 << 16);

        encrypted[0]=((data_to_encrypt32 >> 24) & 0xFF);
        encrypted[1]=((data_to_encrypt32 >> 16) & 0xFF);
        encrypted[2]=((data_to_encrypt32 >> 8) & 0xFF);
        encrypted[3]=(data_to_encrypt32 & 0xFF);

    }
    return true;
}

//Encrypt pyaload wrapper
//QByteArray subaru_denso_encrypt_32bit_payload(const QByteArray &buf, uint16_t *key_to_generate_index)
bool subaru_denso_encrypt_32bit_payload(const QByteArray &buf,
                                        uint16_t *key_to_generate_index,
                                        QByteArray &encrypted)
{
    //QByteArray encrypted;

    /*encrypted = subaru_denso_calculate_32bit_payload(buf, key_to_generate_index, index_transformation);

    return encrypted;*/
    return subaru_denso_calculate_32bit_payload(buf,
                                            key_to_generate_index,
                                            index_transformation,
                                            encrypted);
}

//Encrypt pyaload wrapper
bool subaru_denso_encrypt_32bit_payload(const QByteArray &buf,
                                        uint16_t *key_to_generate_index,
                                        QByteArray &encrypted,
                                        const QByteArray &encrypted_known)
{
    return subaru_denso_calculate_32bit_payload(buf,
                                            key_to_generate_index,
                                            index_transformation,
                                            encrypted,
                                            encrypted_known);
}

//Encrypt payload wrapper
//QByteArray subaru_denso_decrypt_32bit_payload(const QByteArray &buf, uint16_t *key_to_generate_index)
bool subaru_denso_decrypt_32bit_payload(const QByteArray &buf,
                                        uint16_t *key_to_generate_index,
                                        QByteArray &decrypted)
{
    //QByteArray decrypted;

    /*decrypted = subaru_denso_calculate_32bit_payload(buf, key_to_generate_index, index_transformation);

    return decrypted;*/
    return subaru_denso_calculate_32bit_payload(buf,
                                                key_to_generate_index,
                                                index_transformation,
                                                decrypted);
}

//Encrypt payload wrapper
//QByteArray subaru_denso_decrypt_32bit_payload(const QByteArray &buf, uint16_t *key_to_generate_index)
bool subaru_denso_decrypt_32bit_payload(const QByteArray &buf,
                                        uint16_t *key_to_generate_index,
                                        QByteArray &decrypted,
                                        const QByteArray &decrypted_known)
{
    //QByteArray decrypted;

    /*decrypted = subaru_denso_calculate_32bit_payload(buf, key_to_generate_index, index_transformation);

    return decrypted;*/
    return subaru_denso_calculate_32bit_payload(buf,
                                                key_to_generate_index,
                                                index_transformation,
                                                decrypted,
                                                decrypted_known);
}

//Convert from string to integer
//If string starts with '0x', assume hex, else - dec
int _stoi(std::string s)
{
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    int radix = s.starts_with("0x") ? 16 : 10;
    return std::stoi(s, nullptr, radix);
}
