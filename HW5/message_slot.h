#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>


#define MAJOR_NUM 245
#define DEVICE_RANGE_NAME "message_slot"
#define DEVICE_FILE_NAME "simple_message_slot"
#define BUFFER_LEN 120
#define NUM_CHANNELS 4
#define IOCTL_SET_BUFFER _IOW(MAJOR_NUM, 0, unsigned long)


#endif
