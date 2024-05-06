#include "crypt_bf_ocl.h"
#define PRINT_KEY(KEY) printf("0x%.04x 0x%.04x 0x%.04x 0x%.04x\n", KEY[0], KEY[1], KEY[2], KEY[3]);
#define DEBUG_PRINT_HEX(VAR) printf(#VAR "=0x%.04x\n", VAR)

typedef uchar QByteArray[ARRAY_LEN];
#define POS_KEY(i,j) 4 * i + j


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

//Encrypt and decrypt function. Reversed key is decrypt key.
//[k0, k1, k2, k3] is encrypt key and [k3, k2, k1, k0] is decrypt key.
void subaru_denso_calculate_32bit_payload(const uchar *buf,
                                            const ushort *key_to_generate_index,
                                            const uchar *index_transformation,
                                            uchar *encrypted)
{
    uint data_to_encrypt32, index;
    ushort word_to_generate_index, word_to_be_encrypted, encryption_key;
    int ki, n;

    for (uint i = 0; i < ARRAY_LEN; i += 4) {
        data_to_encrypt32 = ((buf[i] << 24) & 0xFF000000) | ((buf[i+1] << 16) & 0xFF0000) | ((buf[i+2] << 8) & 0xFF00) | (buf[i+3] & 0xFF);
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
}

//Encrypt and decrypt function. Reversed key is decrypt key.
//[k0, k1, k2, k3] is encrypt key and [k3, k2, k1, k0] is decrypt key.
//Optimization list
//1. Odd even test
bool subaru_denso_calculate_32bit_payload_optimized(const uchar *buf,
                                            const ushort *key_to_generate_index,
                                            const uchar *index_transformation,
                                            uchar *encrypted,
                                            uchar *decrypted)
{
    uint data_to_encrypt32, index;
    ushort word_to_generate_index, word_to_be_encrypted, encryption_key;
    int ki, n;

    uint decoded_data;
    
    for (uint i = 0; i < ARRAY_LEN; i += 4) 
    {
        data_to_encrypt32 = ((buf[i] << 24) & 0xFF000000) | ((buf[i+1] << 16) & 0xFF0000) | ((buf[i+2] << 8) & 0xFF00) | (buf[i+3] & 0xFF);
        
        //Additional odd/even test start
        uint decoded_data = ((decrypted[i] << 24) & 0xFF000000) | ((decrypted[i+1] << 16) & 0xFF0000) | ((decrypted[i+2] << 8) & 0xFF00) | (decrypted[i+3]        & 0xFF);
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
            encryption_key = index_transformation[index & 0x1F];

            if (ki == 2)
            {
                if(!((decoded_data & 1) ^ ((encryption_key >> 3) & 1)))
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

//Decrypt payload wrapper
void subaru_denso_decrypt_32bit_payload(const uchar *buf,
                                        ushort *key_to_generate_index,
                                        uchar *decrypted)
{
	const uchar index_transformation[]={
    0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
    0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
    0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
    0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
};

    subaru_denso_calculate_32bit_payload(buf,
                                        key_to_generate_index,
                                        &index_transformation,
                                        decrypted);
}

//Decrypt payload wrapper
bool subaru_denso_decrypt_32bit_payload_optimized(const uchar *buf,
                                        ushort *key_to_generate_index,
                                        uchar *decrypted,
                                        uchar *decrypted_known)
{
	const uchar index_transformation[]={
    0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
    0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
    0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
    0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
};

    return subaru_denso_calculate_32bit_payload_optimized(buf,
                                                key_to_generate_index,
                                                &index_transformation,
                                                decrypted,
                                                decrypted_known);
}

//Decrypt payload wrapper
void subaru_denso_encrypt_32bit_payload(const uchar *buf,
                                        ushort *key_to_generate_index,
                                        uchar *decrypted)
{
	const uchar index_transformation[]={
    0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
    0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
    0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
    0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
};

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
                    __global ushort *keys_found)
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
	const uchar mask_00_00_FF_FF[ARRAY_LEN] = {0x0, 0x0, 0xFF, 0xFF};

    uint index = get_global_id(0);
    ushort key0 = keys_start[index];
    //Zeroize all values. I assume no one will use key to 0 0 0 0
    keys_found[POS_KEY(index, 0)]=1;
    keys_found[POS_KEY(index, 1)]=0;
    keys_found[POS_KEY(index, 2)]=0;
    keys_found[POS_KEY(index, 3)]=0;
    ushort key_to_generate_index[4];
    QByteArray decrypted = {0, 0, 0, 0};
    FOR(i1)
    {
        FOR(i2)
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
                {
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
                                keys_found[POS_KEY(index, 0)]=key_to_generate_index[0];
                                keys_found[POS_KEY(index, 1)]=key_to_generate_index[1];
                                keys_found[POS_KEY(index, 2)]=key_to_generate_index[2];
                                keys_found[POS_KEY(index, 3)]=key_to_generate_index[3];
                                return;
                            }
                        }
                    }//i3
                }
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
                    __global ushort *keys_found,
                    __global int *key_was_found)
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
	const uchar mask_00_00_FF_FF[ARRAY_LEN] = {0x0, 0x0, 0xFF, 0xFF};

    uint index = get_global_id(0);
    ushort key_0        = keys_0[index];
    ushort key_1_start  = keys_1_start[index];
    ushort key_1_end    = keys_1_end[index];
    ushort key_to_generate_index[4];
    //Zeroize all values. I assume no one will use key to 0 0 0 0
    keys_found[POS_KEY(index, 0)]=0;
    keys_found[POS_KEY(index, 1)]=0;
    keys_found[POS_KEY(index, 2)]=0;
    keys_found[POS_KEY(index, 3)]=0;
    //Reset key found flag
    //Only one process should do it
    if (index == 0)
    {
        *key_was_found = 0;
    }
    QByteArray decrypted = {0, 0, 0, 0};
    for (int i1=key_1_start;i1<=key_1_end;i1++)
    {
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
                {
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
                                *key_was_found = 1;
                                keys_found[POS_KEY(index, 0)]=key_to_generate_index[0];
                                keys_found[POS_KEY(index, 1)]=key_to_generate_index[1];
                                keys_found[POS_KEY(index, 2)]=key_to_generate_index[2];
                                keys_found[POS_KEY(index, 3)]=key_to_generate_index[3];
                                return;
                            }
                        }
                    }//i3
                }
            }
        }//i2
    }//i1
}

//Checks keys of the following type K0 K1 ???? 0
//Parameter keys_1_start set start key for K1
//Parameter keys_1_end set end key for K1
//Key K0 K1 K2 0 decodes last 2 bytes correctly
//We need to find 3 words and only then find K3
__kernel void try_key_1_optimized(__global const ushort *keys_0,
                    __global const ushort *keys_1_start,
                    __global const ushort *keys_1_end,
                    __global ushort *keys_found,
                    __global int *key_was_found)
{
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

    __private uint index = get_global_id(0);
    __private ushort key_0        = keys_0[index];
    __private ushort key_1_start  = keys_1_start[index];
    __private ushort key_1_end    = keys_1_end[index];
    __private ushort key_to_generate_index[4];
    //Zeroize all values. I assume no one will use key to 0 0 0 0
    keys_found[POS_KEY(index, 0)]=0;
    keys_found[POS_KEY(index, 1)]=0;
    keys_found[POS_KEY(index, 2)]=0;
    keys_found[POS_KEY(index, 3)]=0;
    //Reset key found flag
    //Only one process should do it
    if (index == 0)
    {
        *key_was_found = 0;
    }
    QByteArray decrypted = {0, 0, 0, 0};
    for (__private int i1=key_1_start;i1<=key_1_end;i1++)
    {
        FOR(i2)
        {
            key_to_generate_index[0]=key_0;
            key_to_generate_index[1]=i1;
            key_to_generate_index[2]=i2;
            key_to_generate_index[3]=0;
            __private bool b = subaru_denso_decrypt_32bit_payload_optimized(&encr_buf,
                                            &key_to_generate_index,
                                            decrypted,
                                            &clean_buf);
            //Check if buffer is partially decrypted
            if (b && compare_mask(decrypted, &clean_buf, &mask_00_00_FF_FF))
            {
                //Check if other data can be partially decrypted
                subaru_denso_decrypt_32bit_payload(&encr_test_buf,
                                                &key_to_generate_index,
                                                decrypted);
                if (compare_mask(decrypted, &clean_test_buf, &mask_00_00_FF_FF))
                {
                    FOR (i3)
                    {
                        key_to_generate_index[3] = i3;
                        b = subaru_denso_decrypt_32bit_payload_optimized(&encr_buf,
                                                &key_to_generate_index,
                                                decrypted,
                                                &clean_buf);
                        //Check if buffer is properly decrypted
                        if (b && compare(decrypted, &clean_buf))
                        {
                            subaru_denso_decrypt_32bit_payload(&encr_test_buf,
                                                    &key_to_generate_index,
                                                    decrypted);
                            if (compare(decrypted, &clean_test_buf))
                            {
                                //printf("Candidate key ");
                                //PRINT_KEY(key_to_generate_index);
                                *key_was_found = 1;
                                keys_found[POS_KEY(index, 0)]=key_to_generate_index[0];
                                keys_found[POS_KEY(index, 1)]=key_to_generate_index[1];
                                keys_found[POS_KEY(index, 2)]=key_to_generate_index[2];
                                keys_found[POS_KEY(index, 3)]=key_to_generate_index[3];
                                return;
                            }
                        }
                    }//i3
                }
            }
        }//i2
    }//i1
}

//Checks keys of the following type ??? K1 K2 0
//Parameter keys_2_start set start key for K1
//Parameter keys_2_end set end key for K1
//Key K0 K1 K2 0 decodes last 2 bytes correctly
//We need to find 3 words and only then find K3
__kernel void try_key_1_order_optimized(__global const ushort *keys_1,
                    __global const ushort *keys_2_start,
                    __global const ushort *keys_2_end,
                    __global ushort *keys_found,
                    __global int *key_was_found)
{
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

    __private uint index = get_global_id(0);
    __private ushort key_1        = keys_1[index];
    __private ushort key_2_start  = keys_2_start[index];
    __private ushort key_2_end    = keys_2_end[index];
    __private ushort key_to_generate_index[4];
    //Zeroize all values. I assume no one will use key to 0 0 0 0
    keys_found[POS_KEY(index, 0)]=0;
    keys_found[POS_KEY(index, 1)]=0;
    keys_found[POS_KEY(index, 2)]=0;
    keys_found[POS_KEY(index, 3)]=0;
    //Reset key found flag
    //Only one process should do it
    if (index == 0)
    {
        *key_was_found = 0;
    }
    QByteArray decrypted = {0, 0, 0, 0};
    //FOR(i2)
    for (__private int i2=key_2_start;i2<=key_2_end;i2++)
    {
        FOR(i0)
        {
            key_to_generate_index[0]=i0;
            key_to_generate_index[1]=key_1;
            key_to_generate_index[2]=i2;
            key_to_generate_index[3]=0;
            __private bool b = subaru_denso_decrypt_32bit_payload_optimized(&encr_buf,
                                            &key_to_generate_index,
                                            decrypted,
                                            &clean_buf);
            //Check if buffer is partially decrypted
            if (b && compare_mask(decrypted, &clean_buf, &mask_00_00_FF_FF))
            {
                //PRINT_KEY(key_to_generate_index); return;
                //Check if other data can be partially decrypted
                subaru_denso_decrypt_32bit_payload(&encr_test_buf,
                                                &key_to_generate_index,
                                                decrypted);
                if (compare_mask(decrypted, &clean_test_buf, &mask_00_00_FF_FF))
                {
                    //PRINT_KEY(key_to_generate_index); return;
                    ushort reverse_key_to_generate_index[4];
                    reverse_key_to_generate_index[1] = key_to_generate_index[2];
                    reverse_key_to_generate_index[2] = key_to_generate_index[1];
                    reverse_key_to_generate_index[3] = 0;
                    bool f = false;
                    __private int i3_found;
                    //FOR(i3)
                    FOR(i3_found)
                    {
                        reverse_key_to_generate_index[0] = i3_found;
                        subaru_denso_encrypt_32bit_payload(&clean_buf,
                                                        &reverse_key_to_generate_index,
                                                        decrypted);
                        f = f || compare_mask(decrypted, &encr_buf, &mask_00_00_FF_FF);
                        if (f)
                        {
                            break;
                        }
                    }//i3
                    if (!f)
                    {
                        goto Loop_i2_continue;
                    }

                    //FOR (i3)
                    for (__private int i3=i3_found;i3<=MAX;i3++)
                    {
                        key_to_generate_index[3] = i3;
                        b = subaru_denso_decrypt_32bit_payload_optimized(&encr_buf,
                                                &key_to_generate_index,
                                                decrypted,
                                                &clean_buf);
                        //Check if buffer is properly decrypted
                        if (b && compare(decrypted, &clean_buf))
                        {
                            subaru_denso_decrypt_32bit_payload(&encr_test_buf,
                                                    &key_to_generate_index,
                                                    decrypted);
                            if (compare(decrypted, &clean_test_buf))
                            {
                                //printf("Candidate key ");
                                //PRINT_KEY(key_to_generate_index);
                                *key_was_found = 1;
                                keys_found[POS_KEY(index, 0)]=key_to_generate_index[0];
                                keys_found[POS_KEY(index, 1)]=key_to_generate_index[1];
                                keys_found[POS_KEY(index, 2)]=key_to_generate_index[2];
                                keys_found[POS_KEY(index, 3)]=key_to_generate_index[3];
                                return;
                            }
                        }
                    }//i3
                }
            }
        }//i0
        Loop_i2_continue:
    }//i2
}
