#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "message_slot.h"

MODULE_LICENSE("GPL");

#define SUCCESS 0

#define ERR_PREFIX "MessageSlot ERROR: "
#define ALLOCATION_ERROR ERR_PREFIX "Allocation error!\n"
#define DEVICE_OPEN_ERROR ERR_PREFIX "Error opening device\n"
#define DEVICE_NOT_FOUND ERR_PREFIX "Device not found\n"
#define DEVICE_NULL_ARGS ERR_PREFIX "NULL arguments passed\n"
#define CHANNEL_NOT_SET ERR_PREFIX "Channel not set\n"
#define INVALID_IOCTL "Undefined IOCTL\n"
#define INVALID_CHANNEL_INDEX "Channel index out of range\n"


/* ********** ********** ********** ********** ********** ********** ********** */
/* ********** ********** ********** DEVICE_DATA ********* ********** ********** */
/* ********** ********** ********** ********** ********** ********** ********** */

typedef struct device_data_t {
	int ino;
	struct device_data_t* next;
	char* channel;
	char* all_channels[NUM_CHANNELS];
} device_data;

// device list
static device_data* dev_list = NULL;
static device_data* dev_list_last = NULL;

// device list methods
static int add_device_data(struct file *file);
static device_data* get_device_data(struct file *file);
static void destroy_device_data(device_data* dev);
static void destroy_device_list(void);


static int add_device_data(struct file *file) {
	// create device data
	device_data* dev = (device_data*) kmalloc(sizeof(struct device_data_t),GFP_KERNEL);
	if (!dev) {
		printk(ALLOCATION_ERROR);
		return -1;
	}

	// set paramaters
	dev->ino = file->f_inode->i_ino;
	dev->next = NULL;
	dev->channel = NULL;

	// allocate channels
	for (int i=0; i<NUM_CHANNELS; i++) {
		dev->all_channels[i] = (char*) kmalloc(sizeof(char)*BUFFER_LEN, GFP_KERNEL);
		if (!dev->all_channels[i]) { // allocation error
			printk(ALLOCATION_ERROR);
			destroy_device_data(dev);
			return -1;
		}
		memset(dev->all_channels[i], 0, sizeof(char)*BUFFER_LEN);
	}

	// place at end of list
	if (!dev_list)
		dev_list = dev;
	else
		dev_list_last->next = dev;
	dev_list_last = dev;

	return 0;
}

static device_data* get_device_data(struct file *file) {
	if (dev_list) {
		// scan all devices and return if ino is found
		device_data *dev = dev_list;
		while (dev->next != NULL) {
			if (dev->ino == file->f_inode->i_ino)
				return dev;
			dev = dev->next;
		}
	}
	// ino not found
	return NULL;
}


static void destroy_device_data(device_data* dev) {
	if (dev) {
		// free channels
		for (int i=0; i<NUM_CHANNELS; i++)
			if (dev->all_channels[i])
				kfree(dev->all_channels[i]);
		// free device data
		kfree(dev);
	}
}

static void destroy_device_list(void) {
	device_data *next, *dev = dev_list;
	while (dev != NULL) {
		next = dev->next;
		destroy_device_data(dev);
		dev = next;
	}
}


/* ********** ********** ********** ********** ********** ********** ********** */
/* ********** ********** ******** MODULE METHODS ******** ********** ********** */
/* ********** ********** ********** ********** ********** ********** ********** */
static int major;

static int device_open(struct inode *inode, struct file *file) {
	printk("Device Open(%p)\n", file);
	// check arguments
	if (!file) {
		printk(DEVICE_NULL_ARGS);
		return -EINVAL;
	}

	// check if device data exists and if not create it
	if (!get_device_data(file)) {
		if (add_device_data(file) != 0) {
			printk(DEVICE_OPEN_ERROR);
			return -ERANGE;
		}
	}

	return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file) {
	return SUCCESS;
}


static ssize_t device_read(struct file *file, char __user * buffer, size_t length, loff_t * offset) {
	// check arguments
	if (!file || !buffer) {
		printk(DEVICE_NULL_ARGS);
		return -EINVAL;
	}
	// find device data
	device_data *dev = get_device_data(file);
	if (!dev) {
		printk(DEVICE_NOT_FOUND);
		return -ERANGE;
	}
	// check channel is set
	if (!(dev->channel)) {
		printk(CHANNEL_NOT_SET);
		return -ERANGE;
	}

	// write data to user buffer
    int i;
    for (i=0; i<BUFFER_LEN && i<length; i++)
    	put_user(dev->channel[i], buffer + i);
    return SUCCESS;
}

static ssize_t device_write(struct file *file, const char __user * buffer, size_t length, loff_t * offset) {
	// check arguments
	if (!file || !buffer) {
		printk(DEVICE_NULL_ARGS);
		return -EINVAL;
	}
	// find device data
	device_data *dev = get_device_data(file);
	if (!dev) {
		printk(DEVICE_NOT_FOUND);
		return -ERANGE;
	}
	// check channel is set
	if (!(dev->channel)) {
		printk(CHANNEL_NOT_SET);
		return -ERANGE;
	}

	// read data from user buffer
	int i;
	for (i=0; i<BUFFER_LEN; i++) {
		if (i<length) // read data from user buffer
			get_user(dev->channel[i], buffer + i);
		else // pad with zeros
			dev->channel[i] = 0;
	}
	return SUCCESS;
}

static long device_ioctl(struct file* file, unsigned int ioctl_num, unsigned long ioctl_param) {
	// check arguments
	if (!file) {
		printk(DEVICE_NULL_ARGS);
		return -EINVAL;
	}
	// find device data
	device_data *dev = get_device_data(file);
	if (!dev) {
		printk(DEVICE_NOT_FOUND);
		return -ERANGE;
	}

	if (ioctl_num != IOCTL_SET_BUFFER) {
		printk(INVALID_IOCTL);
		return -EINVAL;
	}

	if (ioctl_param >= 0 && ioctl_param < NUM_CHANNELS) {
		dev->channel = dev->all_channels[ioctl_param];
		return SUCCESS;
	}
	else {
		printk(INVALID_CHANNEL_INDEX);
		return -EINVAL;
	}

}

struct file_operations Fops = {
    .read = device_read,
    .write = device_write,
	.unlocked_ioctl= device_ioctl,
    .open = device_open,
    .release = device_release
};

static int __init message_slot_init(void) {
	printk("Loading message_slot\n");

	////major = register_chrdev(0, DEVICE_RANGE_NAME, &Fops);
	major = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);
	if (major < 0) {
		printk(KERN_ALERT " registering device failed: %d\n", major);
		return -ERANGE;
	}

	return SUCCESS;
}

static void __exit message_slot_cleanup(void) {
	printk("Unloading message_slot\n");
	destroy_device_list();
	////unregister_chrdev(major, DEVICE_RANGE_NAME);
	unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

module_init(message_slot_init);
module_exit(message_slot_cleanup);
