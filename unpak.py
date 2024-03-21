#It will be rude
import argparse
from bitstring import ConstBitStream

VERSION=2024.0321

#This precedes file record
MOT_LABEL = bytearray([0x1, 0x0, 0x0, 0x0, 0xFF, 0xFF])
MOT_LABEL_BIT_LEN = len(MOT_LABEL)*8

parser = argparse.ArgumentParser(
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    description='Unpacks .MOT files from .PAK CArchive'
)
parserGroupOptions = parser.add_argument(
    '-i', '--input',
    metavar='<filename>',
    help='Input filename',
    required=True
)
parserGroupOptions = parser.add_argument(
    '--version',
    action='version',
    help='Print version number.',
    version=f'{VERSION}'
)


args = parser.parse_args()

pak_file_name = args.input
pak_stream = ConstBitStream(filename=pak_file_name)
archived_files = pak_stream.findall(MOT_LABEL)
for f in archived_files:
    pak_stream.bitpos = f + MOT_LABEL_BIT_LEN
    cur_file_len = pak_stream.read('uintle:32')
    pos = int(pak_stream.bitpos/8)
    hex_pos = hex(pos)
    ends = pos + cur_file_len - 8
    print(f'Found at {hex_pos}, len {hex(cur_file_len)}, ends at {hex(ends)}')
    with open(pak_file_name, 'rb') as pak:
        pak.seek(pos)
        #Length bytes are included
        chunk = pak.read(cur_file_len - 8)
        print(f'Writing to {hex_pos}.mot.txt')
        with open(f'{hex_pos}.mot.txt', 'wb') as output_file:
            output_file.write(chunk)
