#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/kd.h>

#include "fb.h"

int main() {
  int fd, tty;
  fb_info info;
  char *fb;

  // open framebuffer
  if ((fd = open("/dev/fb_device", O_RDWR)) < 0) {
    printf("Cannot open /dev/fb_device\n");
    return 0;
  }

  // get screen info
  ioctl(fd, GET_INFO, &info);

  printf("base: %p\n", info.base);
  printf("size: %u\n", info.size);
  printf("width: %u\n", info.width);
  printf("height: %u\n", info.height);
  printf("line length: %u\n", info.line_length);
  // color fields
  printf("bpp: %u\n", info.bpp);
  printf("red off: %u\n", info.red.offset);
  printf("red length: %u\n", info.red.length);
  printf("green off: %u\n", info.green.offset);
  printf("green length: %u\n", info.green.length);
  printf("blue off: %u\n", info.blue.offset);
  printf("blue length: %u\n", info.blue.length);
  printf("trans off: %u\n", info.trans.offset);
  printf("trans length: %u\n", info.trans.length);

  // set tty to graphics mode
  if ((tty = open("/dev/tty", O_RDWR)) < 0) {
    printf("Cannot open /dev/tty\n");
    goto errfb;
  }
  ioctl(tty, KDSETMODE, KD_GRAPHICS);

  // mmap framebuffer
  fb = mmap(0, info.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  // draw
  if(info.bpp == 32) { // true color
    for(int y=0; y < info.height; y++) {
      for(int x=0; x < info.width; x++) {
        uint32_t pixel = y * info.line_length + x * info.bpp/8;
        fb[pixel + info.red.offset/8] = x % 255;
        fb[pixel + info.green.offset/8] = y % 255;
        fb[pixel + info.blue.offset/8] = 0;
        fb[pixel + info.trans.offset/8] = 0;
      }
    }
    sleep(5);
  } else if(info.bpp == 15 || info.bpp == 16) { // high color
    int bytes_per_pixel = (info.bpp + 7)/8;
    for(int y=0; y < info.height; y++) {
      for(int x=0; x < info.width; x++) {
        uint32_t pixel = y * info.line_length + x * bytes_per_pixel;
        uint16_t color = 0;
        color += (x % (1<<info.red.length)) << (info.bpp - info.red.offset);
        color += (y % (1<<info.green.length)) << (info.bpp - info.green.offset);
        uint16_t * p = (uint16_t*)(fb + pixel);
        *p = color;
      }
    }
    sleep(5);
  } else {
    printf("Unsupported color depth: %u\n", info.bpp);
  }

  // unmap framebuffer
  munmap(fb, info.size);

  // restore tty mode
  ioctl(tty, KDSETMODE, KD_TEXT);
  close(tty);

errfb:
  close(fd);

  return 0;
}
