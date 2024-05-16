import sys
import argparse

VERSION = 2024.0516

def substitute_nibble(nibble):
    """
    Substitute a single nibble using the specified substitution table.
    """
    substitution_table = {
        0x0: 0x6,
        0x1: 0x1,
        0x2: 0x0,
        0x3: 0x3,
        0x4: 0xA,
        0x5: 0x5,
        0x6: 0x4,
        0x7: 0x7,
        0x8: 0xE,
        0x9: 0x9,
        0xA: 0x8,
        0xB: 0xB,
        0xC: 0x2,
        0xD: 0xD,
        0xE: 0xC,
        0xF: 0xF
    }
    return substitution_table[nibble]

def flip_bits(byte):
    """
    Flip the highest bit (7th position) and the second lowest bit (1st position) of a byte.
    """
    # XOR with 0b10000010 to flip the highest bit and the second lowest bit
    byte = byte ^ 0b01000101
    high_nibble = substitute_nibble((byte >> 4) & 0xF)  # Extract the high nibble

    return (high_nibble << 4) | (byte & 0xF )

def process_file(input_path, output_path):
    """
    Read a binary file, flip the specified bits of each byte, and write to a new file.
    """
    try:
        with open(input_path, 'rb') as file, open(output_path, 'wb') as output_file:
            file_position = 0
            while True:
                byte = file.read(1)
                if not byte:
                    break  # End of file
                if file_position >= 0x1000:
                    # Skip modification for these ranges
                    modified_byte = byte[0]
                # Offset bytes of > 0x20000
                # Flip the bits of the byte and write    
                else :
                    modified_byte = flip_bits(byte[0])
                output_file.write(bytes([modified_byte]))
                file_position += 1
    except FileNotFoundError:
        print(f"Error: The file {input_path} does not exist.")
    except Exception as e:
        print(f"An error occurred: {e}")

def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description='Flip bits for Denso kernel'
    )
    parserGroupOptions = parser.add_argument(
        '-i', '--input',
        metavar='<filename>',
        help='Input filename',
        required=True
    )
    parserGroupOptions = parser.add_argument(
        '-o', '--output',
        metavar='<filename>',
        help='Output filename',
        required=True
    )
    parserGroupOptions = parser.add_argument('--version',
                                            action='version',
                                            help='Print version number',
                                            version=f'{VERSION}'
    )

    args = parser.parse_args()

    input_file_path = args.input
    output_file_path = args.output
    process_file(input_file_path, output_file_path)

if __name__ == "__main__":
    main()