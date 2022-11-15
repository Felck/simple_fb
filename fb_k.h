#include <linux/ioctl.h>
#include <linux/types.h>

typedef struct {
  __u8 offset;
  __u8 length;
} color_field;

typedef struct {
  char *base;
  __u32 size;
  __u16 width;
  __u16 height;
  __u16 line_length;
  __u16 bpp; // bits per pixel
  color_field red;
  color_field green;
  color_field blue;
  color_field trans;
} fb_info;

#define GET_INFO _IOR(0xF1, 0, fb_info)
