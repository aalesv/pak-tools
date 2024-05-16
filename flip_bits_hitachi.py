import sys
import argparse

VERSION = 2024.0516


def flip_bits(byte):
    """
    Flip the highest bit (7th position) and the second lowest bit (1st position) of a byte.
    """
    # XOR with 0b10000010 to flip the highest bit and the second lowest bit
    return byte ^ 0b10000010

def process_file(input_path, output_path):
    """
    Read a binary file, flip the specified bits of each byte, and write to a new file.
    """
    try:
        with open(input_path, 'rb') as file, open(output_path, 'wb') as output_file:
            while True:
                # Read one byte at a time
                byte = file.read(1)
                if not byte:
                    break  # End of file
                # Flip the bits of the byte
                modified_byte = flip_bits(byte[0])
                # Write the modified byte to the output file
                output_file.write(bytes([modified_byte]))
    except FileNotFoundError:
        print(f"Error: The file {input_path} does not exist.")
    except Exception as e:
        print(f"An error occurred: {e}")

def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description='Flip bits for Hitachi ECU'
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