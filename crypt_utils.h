#if !defined(CRYPT_UTILS_H)
#define CRYPT_UTILS_H

#define HEX(x) std::format("0x{:04X}", x)
#define MAX 0xffff
#define FOR(n) for(uint32_t n=0; n<=MAX; n++)
#define WRITELN(i) std::cout << std::format("{} ",i) << std::endl;
#define WRITE(i) std::cout << std::format("{} ",i)
#define DEBUG_PRINT_HEX(VAR) printf(#VAR "=0x%.04x\n", VAR)

//This is needed to brute force fast
//Right way is to use vector
//Maybe someday I could reduce number of keys
using QByteArray = std::array<uint8_t, 4>;
//using QByteArray = std::vector<uint32_t>;

//QByteArray subaru_denso_calculate_32bit_payload(const QByteArray &buf, const uint16_t *key_to_generate_index, const uint8_t *index_transformation);
bool subaru_denso_calculate_32bit_payload(const QByteArray &buf,
                                    const uint16_t *key_to_generate_index,
                                    const uint8_t *index_transformation,
                                    QByteArray &encrypted);
bool subaru_denso_calculate_32bit_payload(const QByteArray &buf,
                                    const uint16_t *key_to_generate_index,
                                    const uint8_t *index_transformation,
                                    QByteArray &encrypted,
                                    const QByteArray &decrypted);
/*QByteArray subaru_denso_encrypt_32bit_payload(const QByteArray &buf);
QByteArray subaru_denso_encrypt_32bit_payload(const QByteArray &buf, uint16_t *key_to_generate_index);
QByteArray subaru_denso_decrypt_32bit_payload(const QByteArray &buf);
QByteArray subaru_denso_decrypt_32bit_payload(const QByteArray &buf, uint16_t *key_to_generate_index);*/
bool subaru_denso_encrypt_32bit_payload(const QByteArray &buf,
                                        uint16_t *key_to_generate_index,
                                        QByteArray &encrypted);
bool subaru_denso_encrypt_32bit_payload(const QByteArray &buf,
                                        uint16_t *key_to_generate_index,
                                        QByteArray &encrypted,
                                        const QByteArray &encrypted_known);

bool subaru_denso_decrypt_32bit_payload(const QByteArray &buf,
                                        uint16_t *key_to_generate_index,
                                        QByteArray &encrypted);
bool subaru_denso_decrypt_32bit_payload(const QByteArray &buf,
                                        uint16_t *key_to_generate_index,
                                        QByteArray &encrypted,
                                        const QByteArray &decrypted_known);

//Convert from string to integer
//If string starts with '0x', assume hex, else - dec
int _stoi(std::string s);

//Return true if both arrays are indentical
static inline bool compare(const QByteArray &first, const QByteArray &second)
{
    bool res=true;
    for (int i=0; i<first.size(); i++)
    {
        res = (first[i] == second[i]) && res;
        if (! res) break;
    }
    return res;
}

//Return true if both arrays are indentical
//Mask is applied to corresponding elements
static inline bool compare(const QByteArray &first, const QByteArray &second, const QByteArray &mask)
{
    bool res=true;
    for (int i=0; i<first.size(); i++)
    {
        res = ( (first[i]  & mask[i]) ==
                (second[i] & mask[i])) && res;
        if (! res) break;
    }
    return res;
}
#endif //CRYPT_UTILS_H
