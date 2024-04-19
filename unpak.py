import argparse
import os
from bitstring import ConstBitStream, ReadError

def decode_binary_string(s: str):
    """
    Convert string from sequence of 0 and 1 to human readable string
    """
    return ''.join(chr(int(s[i*8:i*8+8],2)) for i in range(len(s)//8))

def read_length (cbt: ConstBitStream):
    """
    Read variable length bytes
    """
    l = cbt.read('uintle:16')
    if l == 0xFFFF:
        l = cbt.read('uintle:32')
    return l

def get_byte_pos(cbt: ConstBitStream):
    """
    Return current stream position in bytes as hex
    """
    return hex(int(cbt.bitpos/8))

def save_file(pak_filename: str, start_pos: int, length: int, out_filename: str):
    """
    Save specified data from stream to a file
    """
    with open(pak_filename, 'rb') as pak:
        pak.seek(start_pos)
        chunk = pak.read(length)
        with open(f'{out_filename}', 'wb') as output_file:
            output_file.write(chunk)

def num_count(count):
    """
    Numbers generator. Starts from 1 and executes for count times.
    """
    start = 1
    for i in range (start, start+count):
        yield i

VERSION=2024.0419-1

parser = argparse.ArgumentParser(
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    description='Unpack/list files from .PAK CArchive'
)
parserGroupOptions = parser.add_argument(
    '-i', '--input',
    metavar='<filename>',
    help='Input filename',
    required=True
)
parserGroupOptions = parser.add_argument(
    '-u', '--unpack',
    help='Unpack all files',
    action='store_true'
)
parserGroupOptions = parser.add_argument(
    '-d', '--dest-dir',
    metavar='<directory>',
    help='Destination directory',
)
parserGroupOptions = parser.add_argument(
    '--version',
    action='version',
    help='Print version number.',
    version=f'{VERSION}'
)

args = parser.parse_args()
UNPACK = args.unpack
HEADER_FILE_NAME = 'header.csv'

pak_file_name = args.input
pak_stream = ConstBitStream(filename=pak_file_name)
pak_dir = f'{pak_file_name}-unpacked' if args.dest_dir is None else args.dest_dir

#Check header
if pak_stream.read('uintle:32') != 0x101:
    print(f'File {pak_file_name} does not seem to be PAK or PK2 CArchive, exiting.')
    exit(1)

if UNPACK and not os.path.exists(pak_dir):
    os.makedirs(pak_dir)

#Unknown structure
unk_struct_len = pak_stream.read('uintle:16')

if UNPACK:
    header_file_name = rf'{pak_dir}\{HEADER_FILE_NAME}'
    print(f'Saving header to {header_file_name}\n')
    save_file(pak_file_name, int(pak_stream.bitpos/8), unk_struct_len, header_file_name)

pak_stream.bitpos += unk_struct_len*8
#Number of files?
num_files = pak_stream.read('uintle:16')
print (f'{pak_file_name} contains {num_files} file(s)')

last_struct = False

while not last_struct:
    #Check structure start header 'FF FF 00 00'
    if pak_stream.read('uintle:32') != 0xFFFF:
        p = get_byte_pos(pak_stream) - 8
        print(f'Unexpected header at {p}, exiting.')
        exit(1)
    #Next is struct name length
    str_len = read_length(pak_stream)
    pos = int(pak_stream.bitpos/8)
    data_struct_name = decode_binary_string(
        pak_stream.read(f'bin:{str_len*8}')
    )
    print(f'Found {data_struct_name} at {hex(pos)}')

    last_file = False

    #Add suffix to same file names
    suffix_generator = num_count(num_files)
    while not last_file:
        #File name
        file_name_len = pak_stream.read('uintle:8')
        file_name = decode_binary_string(
            pak_stream.read(f'bin:{file_name_len*8}')
        )
        #File data header '01 00 00 00'
        if pak_stream.read('uintle:32') != 1:
            p = get_byte_pos(pak_stream) - 8
            print(f'Unexpected header at {p}, exiting.')
            exit(1)
        #8 bytes at the end
        file_len = read_length(pak_stream) - 8
        print(f' Found {file_name} at {get_byte_pos(pak_stream)}, length {hex(file_len)}')
        if UNPACK:
            out_filename = rf'{pak_dir}\{file_name}'
            if os.path.exists(out_filename):
                out_filename = f'{out_filename}-{next(suffix_generator)}'
            save_file(pak_file_name, int(pak_stream.bitpos/8), file_len, out_filename)
            print(f'  Writing to {out_filename}')
        #Move to the end of file
        pak_stream.bitpos += file_len * 8
        #Checksum or something
        pak_stream.bitpos += 8 * 8
        #Check if there's another file, header '01 80'
        #It looks like last file marked with '{num_files} 80'
        try:
            c = pak_stream.peek('uintle:16')
        except (ReadError):
            last_file = True
            last_struct = True
        else:
            finish = 0x8000 + num_files
            last_file = c != 0x8001 and c != finish
        if last_file and last_struct:
            break
        if not last_file:
            pak_stream.bitpos += 2*8


