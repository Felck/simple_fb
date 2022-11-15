#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/screen_info.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include "fb_k.h"

static dev_t dev = 0;
static struct class *fb_class;
static struct cdev fb_cdev;

static fb_info info;

static int fb_open(struct inode *inode, struct file *file)
{
  return 0;
}

static int fb_release(struct inode *inode, struct file *file)
{
  return 0;
}

static ssize_t fb_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
  char *buffer, *src;
  int c, err = 0, cnt = 0;

  if (!info.base)
    return -ENODEV;

  if (*ppos > info.size)
    return 0;

  if (len > info.size)
    len = info.size;

  if (len + *ppos > info.size)
    len = info.size - *ppos;

  buffer = kmalloc((len > PAGE_SIZE) ? PAGE_SIZE : len, GFP_KERNEL);
  if (!buffer)
    return -ENOMEM;

  src = info.base + *ppos;

  while (len) {
    c = (len > PAGE_SIZE) ? PAGE_SIZE : len;

    // copy from io
    memcpy_fromio(buffer, src, c);

    // copy to user
    if (copy_to_user(buf, buffer, c)) {
      err = -EFAULT;
      break;
    }
    *ppos += c;
    src += c;
    buf += c;
    cnt += c;
    len -= c;
  }

  kfree(buffer);

  return err ? err : cnt;
}

static ssize_t fb_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
  char *buffer, *dst;
  int c, err = 0, cnt = 0;

  if (!info.base)
    return -ENODEV;

  if (*ppos > info.size)
    return -EFBIG;

  if (len > info.size) {
    err = -EFBIG;
    len = info.size;
  }

  if (len + *ppos > info.size) {
    if (!err)
      err = -ENOSPC;

    len = info.size - *ppos;
  }

  buffer = kmalloc((len > PAGE_SIZE) ? PAGE_SIZE : len, GFP_KERNEL);
  if (!buffer)
    return -ENOMEM;

  dst = info.base + *ppos;

  while (len) {
    c = (len > PAGE_SIZE) ? PAGE_SIZE : len;

    // copy from user
    if (copy_from_user(buffer, buf, c)) {
      err = -EFAULT;
      break;
    }
    //copy to io
    memcpy_toio(dst, buffer, c);

    *ppos += c;
    dst += c;
    buf += c;
    cnt += c;
    len -= c;
  }

  kfree(buffer);

  pr_info("%u\n", cnt);

  return err ? err : cnt;
}

static int fb_mmap(struct file *file, struct vm_area_struct *vma)
{
  vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
  return vm_iomap_memory(vma, screen_info.lfb_base, info.size);
}

static long fb_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  switch (cmd) {
    case GET_INFO:
      if (copy_to_user((fb_info*)arg, &info, sizeof(fb_info))) {
        return -EFAULT;
      }
      break;
    default:
      return -ENOTTY;
  }
  return 0;
}

static const struct file_operations fops =
{
  .owner          = THIS_MODULE,
  .open           = fb_open,
  .release        = fb_release,
  .read           = fb_read,
  .write          = fb_write,
  .mmap           = fb_mmap,
  .unlocked_ioctl = fb_ioctl
};

static int fb_probe(void) {
  __u32 size = screen_info.lfb_height * screen_info.lfb_linelength;
  if (size % PAGE_SIZE)
    size += PAGE_SIZE - (size % PAGE_SIZE);
  info.size = size;

  if((info.base = memremap(screen_info.lfb_base, size, MEMREMAP_WB)) < 0) {
    pr_err("memremap error");
  }


  info.width        = screen_info.lfb_width;
  info.height       = screen_info.lfb_height;
  info.line_length  = screen_info.lfb_linelength;
  info.bpp          = screen_info.lfb_depth;
  // color fields
  info.red.offset   = screen_info.red_pos;
  info.red.length   = screen_info.red_size;
  info.green.offset = screen_info.green_pos;
  info.green.length = screen_info.green_size;
  info.blue.offset  = screen_info.blue_pos;
  info.blue.length  = screen_info.blue_size;
  info.trans.offset = screen_info.rsvd_pos;
  info.trans.length = screen_info.rsvd_size;

  return 0;
}

static int fb_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static int fb_init(void)
{
  // allocate major number
  if (alloc_chrdev_region(&dev, 0, 1, "simplefb") < 0) {
    pr_err("Cannot allocate major number\n");
    return -1;
  }

  // add character device
  cdev_init(&fb_cdev, &fops);
  if (cdev_add(&fb_cdev, dev, 1) < 0) {
    pr_err("Cannot add the device to the system\n");
    goto err_cdev;
  }

  // create struct class
  if ((fb_class = class_create(THIS_MODULE, "fb_class")) == NULL) {
    pr_err("Cannot create struct class\n");
    goto err_class;
  }
  fb_class->dev_uevent = fb_uevent;

  // create device file
  if (device_create(fb_class, NULL, dev, NULL, "fb_device") == NULL) {
    pr_err("Cannot create device\n");
    goto err_device;
  }

  if (fb_probe() < 0) {
    pr_err("Cannot init framebuffer\n");
    goto err_probe;
  }

  pr_info("simplefb inserted\n");
  return 0;

  // error handling
err_probe:
  device_destroy(fb_class, dev);
err_device:
  class_destroy(fb_class);
err_class:
  cdev_del(&fb_cdev);
err_cdev:
  unregister_chrdev_region(dev, 1);
  return -1;
}

static void fb_exit(void)
{
  device_destroy(fb_class, dev);
  class_destroy(fb_class);
  cdev_del(&fb_cdev);
  unregister_chrdev_region(dev, 1);
  printk(KERN_INFO "sipmlefb removed\n");
  return;
}

module_init(fb_init);
module_exit(fb_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");
MODULE_AUTHOR("Felix Eckardt");
MODULE_DESCRIPTION("Simple framebuffer module");
