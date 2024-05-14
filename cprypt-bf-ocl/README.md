# OpenCL bruteforcer

## Setup

All settings are in `crypt_bf_ocl.ini` file.

Usually OpenCL device has platform number `0` and device number `0`, so you don't have to change these settings.

Run `info.exe` to see device info, run `gpu-settings-detect.exe` to check if OpenCL works.

OpenCL software consists of two parts - host and device. Host part runs on CPU and controls device part. Device part is built every time you run `crypt_bf_ocl.exe`. To edit data to bruteforce, open `crypt_bf.cl` with text editor, go to `try_key_1` kernel and edit `*_buf` variables initialization.

## Preformance tuning

To achieve maximum speed, one need to choose right `GLOBAL_WI` and `LOCAL_WG` value. They both must be power of 2. Start from `256` for both parameters and first increase `GLOBAL_WI` until interval between keys becomes about 1.5 seconds. If one has only one GPU, system may become unstable. Decrease `GLOBAL_WI` value in this case.

If you have several GPUs, you can set `GLOBAL_WI` very high, because the more is the better. But this may cause GPU driver instability and timeouts.
