# OpenCL bruteforcer

To achieve maximum speed, one need to choose right `GLOBAL_WI` and `LOCAL_WG` value. They both must be power of 2. Start from `256` for both parameters and first increase `GLOBAL_WI` until interval between keys becomes about 1.5 seconds. If one has only one GPU, system may become unstable. Decrease `GLOBAL_WI` value in this case.

If you have several GPUs, you can set `GLOBAL_WI` very high, because the more is the better. But this may cause GPU driver instability and timeouts.
