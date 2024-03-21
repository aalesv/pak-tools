import argparse
import csv

VERSION = 2024.0321
DB_NAME = 'C:\Program Files (x86)\Subaru\FlashWrite\Pack File Database_EC_AU_GE.csv'

parser = argparse.ArgumentParser(
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    description='Find PAK key by CAL ID, ECU part number or PAK number. Part of string can be entered.'
)
parserGroupMain = parser.add_mutually_exclusive_group(required=True)
parserGroupMain.add_argument('-c', '--cal-id',
                             metavar='<CAL ID>',
                             help='CAL ID of ROM.'
)
parserGroupMain.add_argument('-p','--part-number',
                             metavar='<part number>',
                             help='ECU part number'
)
parserGroupMain.add_argument('-k','--pak-number',
                             metavar='<pak number>',
                             help='PAK number'
)
parserGroupOptions = parser.add_argument('--csv-file',
                                         metavar='<filename>',
                                         help='CSV database file',
                                         default=DB_NAME
)
parserGroupOptions = parser.add_argument('--version',
                                         action='version',
                                         help='Print version number.',
                                         version=f'{VERSION}'
)

args = parser.parse_args()

cal_id = args.cal_id
part_number = args.part_number
pak_number = args.pak_number
if cal_id is not None:
    index = 8
    value = cal_id.upper()
elif part_number is not None:
    index = 4
    value = part_number.upper()
elif pak_number is not None:
    index = 5
    value = pak_number.upper()
db = args.csv_file
with open(db, encoding="utf16") as csvfile:
    csvreader = csv.reader(csvfile, delimiter=",")
    for row in csvreader:
        if value in row[index]:
            print(f'CALID: {row[8]}, Part_Number: {row[4]}, PAK: {row[5]}.pak, Key: {row[10]}')
