# simplefb - Simple Linux framebuffer device clone

This is a simple framebuffer character driver implementation. Supported operations on the created device file under `/dev/fb_device` are `read`, `write` and `mmap`. An IOCTL call `GET_INFO` returns framebuffer size, resolution, color depth and offsets.

## Run example

1. Enter grub console and type `videomode` to get the avialable video modes.
2. Set screen resolution and color depth in grub with `set gfxpayload=<videomode>`. E.g. `800x600x32`.
3. Disable DRI/KMS with the boot parameter `nomodeset`. (XServer won't be available.)
4. Boot and switch to TTY (Strg+Alt+F1).
5. Run
```
make
insmod fbmodule.ko
./fbtest
```


## Files

* *fb_k.h* - kernel space header
* *fbmodule.c* - module source
* *fb.h* - user space header
* *fbtest.c* - user space test program
