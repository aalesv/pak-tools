#include "crypt_bf_ocl.h"
#define PRINT_KEY(KEY) printf("0x%.04x 0x%.04x 0x%.04x 0x%.04x\n", KEY[0], KEY[1], KEY[2], KEY[3]);
#define DEBUG_PRINT_HEX(VAR) printf(#VAR "=0x%.04x\n", VAR)

typedef uchar QByteArray[ARRAY_LEN];
#define POS_KEY(i,j) 4 * i + j

//Set this for bruteforce. 4 bytes give max speed but many false positives.
//Clean unencrypted bytes
__private const QByteArray clean_buf = {0xff, 0xff, 0xff, 0xff};
//Encrypted bytes
__private const QByteArray encr_buf = {0x68, 0xFC, 0x4E, 0x1C};

//Set this to discard false positives
//Clean unencrypted bytes
__private const QByteArray clean_test_buf = {0xFF, 0x00, 0x03, 0x00};
//Encrypted bytes
__private const QByteArray encr_test_buf  = {0x85, 0x6F, 0x15, 0x7C};
__private const uchar mask_00_00_FF_FF[ARRAY_LEN] = {0x0, 0x0, 0xFF, 0xFF};

//Return true if both arrays are indentical
static inline int compare(const uchar *first, const uchar *second)
{
    int res=1;
    for (int i=0; i<ARRAY_LEN; i++)
    {
        res = (first[i] == second[i]) && res;
        if (! res) break;
    }
    return res;
}

//Return true if both arrays are indentical
//Mask is applied to corresponding elements
static inline int compare_mask( const uchar *first,
                                const uchar *second,
                                const uchar *mask)
{
    int  res=1;
    for (int i=0; i<ARRAY_LEN; i++)
    {
        int f = first[i];
        int s = second[i];
        int m = mask[i];
        res = ( (f & m) ==
                (s & m)) && res;
        if (! res) break;
    }
    return res;
}

__private const uchar index_transformation[]={
    0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
    0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
    0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
    0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
};

//Encrypt and decrypt function. Reversed key is decrypt key.
//[k0, k1, k2, k3] is encrypt key and [k3, k2, k1, k0] is decrypt key.
void subaru_denso_calculate_32bit_payload(const uchar *buf,
                                            const ushort *key_to_generate_index,
                                            const uchar *index_transformation,
                                            uchar *encrypted)
{
    //QByteArray encrypted;
    uint data_to_encrypt32, index;
    ushort word_to_generate_index, word_to_be_encrypted, encryption_key;
    int ki, n;

    for (uint i = 0; i < ARRAY_LEN; i += 4) {
        ushort buf_i_0 = buf[i];
        ushort buf_i_1 = buf[i+1];
        ushort buf_i_2 = buf[i+2];
        ushort buf_i_3 = buf[i+3];
        data_to_encrypt32 = ((buf_i_0 << 24) & 0xFF000000) | ((buf_i_1 << 16) & 0xFF0000) | ((buf_i_2 << 8) & 0xFF00) | (buf_i_3 & 0xFF);

        for (ki = 0; ki < 4; ki++) {

            word_to_generate_index = data_to_encrypt32;
            word_to_be_encrypted = data_to_encrypt32 >> 16;
            index = word_to_generate_index ^ key_to_generate_index[ki];
            index += index << 16;
            encryption_key = 0;

            for (n = 0; n < 4; n++) {
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
    return encrypted;
}

//Decrypt payload wrapper
//QByteArray* subaru_denso_decrypt_32bit_payload(const QByteArray *buf, ushort *key_to_generate_index)
void subaru_denso_decrypt_32bit_payload(const ushort *buf,
                                        ushort *key_to_generate_index,
                                        ushort *decrypted)
{
    subaru_denso_calculate_32bit_payload(buf,
                                        key_to_generate_index,
                                        &index_transformation,
                                        decrypted);
}

#define FOR(n) for(uint n=0; n<=MAX; n++)

//Checks keys of the following type K0 ???? ???? 0
//Key K0 K1 K2 0 decodes last 2 bytes correctly
//We need to find 3 words and only then find K3
__kernel void try_key_0(__global const ushort *keys_start,
                    __global ushort *key_found)
{
    uint index = get_global_id(0);
    ushort key0 = keys_start[index];
    //key0 = 0x6587;
    //printf("Kernel %d, key0=%d\n", index, key0);
    //if (index == 1)
    //    key_found[POS_KEY(index, 3)] = 0x100 + key0;
    /*ushort key_to_generate_index[4] = {0x6587, 0x4492, 0xa8b4, 0x7bf2};
    QByteArray decrypted = {0, 0, 0, 0};
    subaru_denso_decrypt_32bit_payload(&encr_buf, &key_to_generate_index, decrypted);
    PRINT_KEY(decrypted);
    const uchar c[4] = {0xff, 0xff, 0xff, 0xff};
    if (compare(&clean_buf, decrypted))
        printf("Key found\n");
    compare_mask(decrypted, &clean_buf, &mask_00_00_FF_FF);
    return;*/
    ushort key_to_generate_index[4];
    QByteArray decrypted = {0, 0, 0, 0};
    //FOR(i1)
    for (int i1=0x4490;i1<=0x4493;i1++)
    {
        //printf("i1=%d\n",i1);
        //FOR(i2)
        for (int i2=0;i2<=MAX;i2++)        
        {
            key_to_generate_index[0]=key0;
            key_to_generate_index[1]=i1;
            key_to_generate_index[2]=i2;
            key_to_generate_index[3]=0;
            subaru_denso_decrypt_32bit_payload(&encr_buf,
                                            &key_to_generate_index,
                                            decrypted);
            //Check if buffer is partially decrypted
            if (compare_mask(decrypted, &clean_buf, &mask_00_00_FF_FF))
            {
                //Check if other data can be partially decrypted
                subaru_denso_decrypt_32bit_payload(&encr_test_buf,
                                                &key_to_generate_index,
                                                decrypted);
                if (compare_mask(decrypted, &clean_test_buf, &mask_00_00_FF_FF))
                    FOR (i3)
                    {
                        key_to_generate_index[3] = i3;
                        subaru_denso_decrypt_32bit_payload(&encr_buf,
                                                &key_to_generate_index,
                                                decrypted);
                        //Check if buffer is properly decrypted
                        if (compare(decrypted, &clean_buf))
                        {
                            subaru_denso_decrypt_32bit_payload(&encr_test_buf,
                                                    &key_to_generate_index,
                                                    decrypted);
                            if (compare(decrypted, &clean_test_buf))
                            {
                                //printf("Candidate key ");
                                //PRINT_KEY(key_to_generate_index);
                                key_found[POS_KEY(index, 0)]=key_to_generate_index[0];
                                key_found[POS_KEY(index, 1)]=key_to_generate_index[1];
                                key_found[POS_KEY(index, 2)]=key_to_generate_index[2];
                                key_found[POS_KEY(index, 3)]=key_to_generate_index[3];
                                return;
                            }
                        }
                    }//i3
            }

        }//i2

    }//i1
}

//Checks keys of the following type K0 K1 ???? 0
//Parameter keys_1_start set start key for K1
//Parameter keys_1_end set end key for K1
//Key K0 K1 K2 0 decodes last 2 bytes correctly
//We need to find 3 words and only then find K3
__kernel void try_key_1(__global const ushort *keys_0,
                    __global const ushort *keys_1_start,
                    __global const ushort *keys_1_end,
                    __global ushort *key_found)
{
    uint index = get_global_id(0);
    ushort key_0        = keys_0[index];
    ushort key_1_start  = keys_1_start[index];
    ushort key_1_end    = keys_1_end[index];
    ushort key_to_generate_index[4];
    //DEBUG_PRINT_HEX(key_0);
    //DEBUG_PRINT_HEX(key_1_start);
    //DEBUG_PRINT_HEX(key_1_end);
    QByteArray decrypted = {0, 0, 0, 0};
    for (int i1=key_1_start;i1<=key_1_end;i1++)
    {
        //DEBUG_PRINT_HEX(i1);
        FOR(i2)
        {
            key_to_generate_index[0]=key_0;
            key_to_generate_index[1]=i1;
            key_to_generate_index[2]=i2;
            key_to_generate_index[3]=0;
            subaru_denso_decrypt_32bit_payload(&encr_buf,
                                            &key_to_generate_index,
                                            decrypted);
            //Check if buffer is partially decrypted
            if (compare_mask(decrypted, &clean_buf, &mask_00_00_FF_FF))
            {
                //Check if other data can be partially decrypted
                subaru_denso_decrypt_32bit_payload(&encr_test_buf,
                                                &key_to_generate_index,
                                                decrypted);
                if (compare_mask(decrypted, &clean_test_buf, &mask_00_00_FF_FF))
                    FOR (i3)
                    {
                        key_to_generate_index[3] = i3;
                        subaru_denso_decrypt_32bit_payload(&encr_buf,
                                                &key_to_generate_index,
                                                decrypted);
                        //Check if buffer is properly decrypted
                        if (compare(decrypted, &clean_buf))
                        {
                            subaru_denso_decrypt_32bit_payload(&encr_test_buf,
                                                    &key_to_generate_index,
                                                    decrypted);
                            if (compare(decrypted, &clean_test_buf))
                            {
                                //printf("Candidate key ");
                                //PRINT_KEY(key_to_generate_index);
                                key_found[POS_KEY(index, 0)]=key_to_generate_index[0];
                                key_found[POS_KEY(index, 1)]=key_to_generate_index[1];
                                key_found[POS_KEY(index, 2)]=key_to_generate_index[2];
                                key_found[POS_KEY(index, 3)]=key_to_generate_index[3];
                                return;
                            }
                        }
                    }//i3
            }
        }//i2
    }//i1
}
