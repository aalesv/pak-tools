//This is needed to brute force fast
//Right way is to use vector
//Maybe someday I could reduce number of keys
using QByteArray = std::array<uint8_t, 4>;
//using QByteArray = std::vector<uint32_t>;

QByteArray subaru_denso_calculate_32bit_payload(const QByteArray &buf, const uint16_t *key_to_generate_index, const uint8_t *index_transformation);
QByteArray subaru_denso_encrypt_32bit_payload(const QByteArray &buf);
QByteArray subaru_denso_encrypt_32bit_payload(const QByteArray &buf, uint16_t *key_to_generate_index);
QByteArray subaru_denso_decrypt_32bit_payload(const QByteArray &buf);
QByteArray subaru_denso_decrypt_32bit_payload(const QByteArray &buf, uint16_t *key_to_generate_index);
//Convert from string to integer
//If string starts with '0x', assume hex, else - dec
int stoi(std::string s);
