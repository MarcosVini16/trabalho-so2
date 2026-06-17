// include/position_ioctl.h
#ifndef POSITION_IOCTL_H
#define POSITION_IOCTL_H

#include <linux/ioctl.h>

#define POSITION_MAGIC  'P'
#define POSITION_GET_QUADRANT _IOR(POSITION_MAGIC, 1, unsigned char)
#define POSITION_DEVICE_NAME  "position"
#define POSITION_DEVICE_PATH  "/dev/" POSITION_DEVICE_NAME

#endif