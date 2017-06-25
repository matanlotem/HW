#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>


#define MAJOR_NUM 245
#define DEVICE_RANGE_NAME "message_slot"
#define DEVICE_FILE_NAME "simple_message_slot"
#define BUFFER_LEN 128
#define NUM_CHANNELS 4
#define IOCTL_SET_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

// userspace error mesages
#define SENDER_USAGE_ERROR "usage: message_sender <channel_index> <message>\n"
#define READER_USAGE_ERROR "usage: message_reader <channel_index> <message>\n"
#define DEVICE_OPEN_ERROR "Error opening device: %s\n"
#define SET_CHANNEL_ERROR "Error setting device channel: %s\n"
#define DEVICE_WRITE_ERROR "Error writing to device: %s\n"
#define DEVICE_READ_ERROR "Error reading from device: %s\n"

// userspace messages
#define SENDER_SUCCESS_MSG "%d bytes written to channel %d\n"
#define READER_SUCCESS_MSG "Read channel %d:\n"


#endif
