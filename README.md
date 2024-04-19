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

`srec2bin.exe <new_bin_fil_name> -M <size_in_mbytes> -s <decrypted_file>`

At this step you'll get `new_bin_fil_name` encrypted with kind a swap bit cipher.

It cannot be decrypted at the moment.