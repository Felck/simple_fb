#include <stdint.h>
#include <sys/ioctl.h>

typedef struct {
  uint8_t offset;
  uint8_t length;
} color_field;

typedef struct {
  char *base;
  uint32_t size;
  uint16_t width;
  uint16_t height;
  uint16_t line_length;
  uint16_t bpp; // bits per pixel
  color_field red;
  color_field green;
  color_field blue;
  color_field trans;
} fb_info;

#define GET_INFO _IOR(0xF1, 0, fb_info)
