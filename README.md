# SSM FlashWrite PAK Tools

This software made in educational purpose. Using in other purpose is prohibited. Don't flash any files produced by these scripts.

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

ECU Hitachi key: `0xF50E 0x973C 0x77F4 0x14CA`

TCU Hitachi key: `0x6587 0x4492 0xa8b4 0x7bf2`

WA12212970WWW key: '0x7c03 0x9312 0x2962 0x78f1'

## crypt-bf

Single-threaded key bruteforcer

## crypt-bf-mt

Multithreaded key bruteforcer

* `crypt-bf-mt.exe` binary

* `crypt-bf-mt.ini` config file

## crypt-bf-ocl

* `crypt-bf-ocl.exe` runs on host

* `crypt-bf-ocl.cl` is compiled by `crypt-bf-ocl.exe` and runs on OpenCL device (GPU)

* `crypt-bf-ocl.ini` config file

To build you'll need OCL SDK. Get it here https://github.com/GPUOpen-LibrariesAndSDKs/OCL-SDK
