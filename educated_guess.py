"""
educated_guess.py: Utility functions for optimized key guesses based on known BIN files.

This file uses a decrypted bin file to search for keys and tests against an encryted bin file

@author: jimihimisimi
@license: GNU General Public License v3.0
@license-url: https://www.gnu.org/licenses/gpl-3.0.html
@repository: https://github.com/example/project
@version: v1.0.0
"""

import sys
from collections import Counter

def find_most_common_sequences(file_path, sequence_length=4, num_results=2):
    sequences_counter = Counter()
    
    with open(file_path, 'rb') as file:
        while True:
            sequence = file.read(sequence_length)
            if not sequence:
                break
            sequences_counter[sequence] += 1
    
    most_common_sequences = sequences_counter.most_common(num_results)
    return most_common_sequences

def find_text_in_binary_file(file_path, text):
    with open(file_path, 'rb') as file:
        chunk_size = 1024  # Read file in chunks of 1024 bytes
        buffer = b''  # Initialize an empty buffer
        
        while True:
            chunk = file.read(chunk_size)  # Read a chunk of bytes from the file
            if not chunk:
                break
            
            buffer += chunk  # Append the chunk to the buffer
            while True:
                index = buffer.find(text.encode())  # Find the index of the text in the buffer
                if index == -1:
                    break  # Text not found in current chunk, break inner loop
                yield index + len(buffer) - len(chunk)  # Yield the absolute index of the text
                buffer = buffer[index + len(text):]  # Remove processed portion from the buffer

#Crypto algo implementations borrowed from FastECU by miikasyvanen
def subaru_denso_calculate_32bit_payload(buf, key_to_generate_index):
    index_transformation = bytearray([
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    ])

    encrypted = bytearray()
    
    for i in range(0, len(buf), 4):
        data_to_encrypt32 = ((buf[i] << 24) & 0xFF000000) | ((buf[i + 1] << 16) & 0xFF0000) | ((buf[i + 2] << 8) & 0xFF00) | (buf[i + 3] & 0xFF)
        
        for ki in range(4):
            word_to_generate_index = (data_to_encrypt32) & 0xFFFF
            word_to_be_encrypted = data_to_encrypt32 >> 16
            key_value =  key_to_generate_index[(ki<<1)]<<8 | key_to_generate_index[(ki<<1)+1]
            index = (word_to_generate_index ^ (key_value)) &0xFFFFFFFF
            index = (index + (index << 16) ) & 0xFFFFFFFF
            encryption_key = 0
            
            for n in range(4):
                encryption_key += index_transformation[(index >> (n * 4)) & 0x1F] << (n * 4)
            encryption_key = ((encryption_key >> 3) + (encryption_key << 13)) & 0xFFFF
            data_to_encrypt32 = ((encryption_key ^ word_to_be_encrypted) + (word_to_generate_index << 16)) & 0xFFFFFFFF
        
        data_to_encrypt32 = ((data_to_encrypt32 >> 16) + (data_to_encrypt32 << 16)) & 0xFFFFFFFF
                
        encrypted.append((data_to_encrypt32 >> 24) & 0xFF)
        encrypted.append((data_to_encrypt32 >> 16) & 0xFF)
        encrypted.append((data_to_encrypt32 >> 8) & 0xFF)
        encrypted.append(data_to_encrypt32 & 0xFF)
        
    
    return encrypted
    
def find_keys_sequences(file1_path, ffff_value, ecu_type,sequence_length=8):
    matching_sequences = []

    with open(file1_path, 'rb') as file1:
        sequence1 = bytearray(file1.read(sequence_length))

        #rearrange bytes for Denso style key
        if (ecu_type == 1):
            byte_temp=bytearray(8);
            byte_temp[0]=sequence1[6];
            byte_temp[6]=sequence1[0];
            byte_temp[1]=sequence1[7];
            byte_temp[7]=sequence1[1];
            byte_temp[2]=sequence1[4];
            byte_temp[4]=sequence1[2];
            byte_temp[3]=sequence1[5];
            byte_temp[5]=sequence1[3];        
            sequence1=byte_temp;
        
        #try to decrypt
        while sequence1:
            file1.seek(-sequence_length, 1)  # Move file1's pointer back to start of current sequence
            ret_val = int.from_bytes(subaru_denso_calculate_32bit_payload(ffff_value, sequence1))
            if ret_val == 0xffffffff :
                print("0xffffffff value found with key ",hex(int.from_bytes(sequence1))," at ",hex(file1.tell()))
                return
            file1.seek(1, 1)  # Slide window of sequence1 by 1 byte
            sequence1 = file1.read(sequence_length)

            if len(sequence1) < sequence_length:
                print("end of file reached")
                return

            #rearrange bytes for Denso style key
            if ecu_type ==1:
                byte_temp=bytearray(8);
                byte_temp[0]=sequence1[6];
                byte_temp[6]=sequence1[0];
                byte_temp[1]=sequence1[7];
                byte_temp[7]=sequence1[1];
                byte_temp[2]=sequence1[4];
                byte_temp[4]=sequence1[2];
                byte_temp[3]=sequence1[5];
                byte_temp[5]=sequence1[3];
                sequence1=byte_temp;


if len(sys.argv) != 3:
    print("Usage: educated_guess.py <decrypted_file_path> <encrypted_file_path>")
    sys.exit(1)

decrypted_file_path = sys.argv[1]
encrypted_file_path = sys.argv[2]

ffff_encrypted=0xFFFFFFFF;
zero_encrypted=0x00000000;

most_common_sequences_decrypted = find_most_common_sequences(decrypted_file_path)
print("Most common sequences, ",decrypted_file_path)
for sequence, count in most_common_sequences_decrypted:
    print(f"Sequence: {hex(int.from_bytes(sequence))}, Count: {count}")
    
most_common_sequences_encrypted = find_most_common_sequences(encrypted_file_path)
print("Most common sequences, ",encrypted_file_path)
for sequence, count in most_common_sequences_encrypted:
    print(f"Sequence: {hex(int.from_bytes(sequence))}, Count: {count}")
ffff_encrypted=most_common_sequences_encrypted[0][0]
zero_encrypted=most_common_sequences_encrypted[1][0]

denso_flag = 0
search_text = "DENSO"
find_text_in_binary_file(decrypted_file_path,"search_text")
for index in find_text_in_binary_file(decrypted_file_path, search_text):
    print(f"Found '{search_text}' at index {index} in the file. Setting ECU Type.")
    denso_flag = 1

find_keys_sequences(decrypted_file_path, ffff_encrypted,denso_flag)

print("All Done")