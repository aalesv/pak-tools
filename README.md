# SSM FlashWrite PAK Tools

Incomplete support. Can extract, decrypt, convert to encrypted binary without protected area. Don't flash any files produced with this scripts.

## Howto

* Find a key by Cal ID, by ECU part number or by PAK:

`get_pak_key.py -c <YOUR_CAL_ID>`

`get_pak_key.py -p <PART_NUMBER>`

`get_pak_key.py -k <PAK_NUMBER>`

* Unpack PAK:

`unpak.py -i <PAK_FILE_NAME> -u`

* Decypt

`rc2decode.exe <unpacked_file> <key> <decrypted_file>`

To decrypt `header.csv` use key `CsvKey`

At this step decrypted file should contain data in Mototrola S-Record format

* Create BIN file

`srec2bin.exe <new_bin_file_name> -M <size_in_mbytes> -s <decrypted_file>`

At this step you'll get `new_bin_file_name` encrypted with kind a swap bit cipher.

* Decrypt it

`crypt.exe <infile> <key_word0> <key_word1> <key_word2> <key_word3> [<outfile>]`

Some known decryption keys (place words in reverse order to encrypt):

ECU Denso CAN key: `0x92A0 0xE282 0x32C0 0xC85B`

ECU Denso key: `0x6E86 0xF513 0xCE22 0x7856`

ECU Hitachi key: `0x14CA 0x77F4 0x973C 0xF50E`

TCU Hitachi key: `0x6587 0x4492 0xa8b4 0x7bf2`
