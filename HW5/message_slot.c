#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
//#include <linux/string.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#define SUCCESS 0
#define DEVICE_RANGE_NAME "message_slot"
#define DEVICE_FILE_NAME "simple_message_slot"
#define BUFFER_LEN 120
#define NUM_BUFFERS 4

#define ERR_PREFIX "MessageSlot ERROR: "
#define ALLOCATION_ERROR ERR_PREFIX "Allocation error!\n"
#define DEVICE_OPEN_ERROR ERR_PREFIX "Error opening device\n"
#define DEVICE_NOT_FOUND ERR_PREFIX "Device not found\n"



/* ********** ********** ********** ********** ********** ********** ********** */
/* ********** ********** ********** DEVICE_DATA ********* ********** ********** */
/* ********** ********** ********** ********** ********** ********** ********** */
typedef struct device_data_t {
	int ino;
	struct device_data_t* next;
	char* buffer;
	char* all_buffers[NUM_BUFFERS];
} device_data;

static device_data* dev_list = NULL;
static device_data* dev_list_last = NULL;

int add_device_data(int ino);
device_data* get_device_data(int ino);
void destroy_device_data(device_data* dev);
void destroy_device_list(void);


int add_device_data(int ino) {
	// create device data
	device_data* dev = (device_data*) kmalloc(sizeof(struct device_data_t),GFP_KERNEL);
	if (!dev) {
		printk(ALLOCATION_ERROR);
		return -1;
	}

	// set paramaters
	dev->ino = ino;
	dev->next = NULL;
	dev->buffer = NULL;

	// allocate buffers
	for (int i=0; i<NUM_BUFFERS; i++) {
		dev->all_buffers[i] = (char*) kmalloc(sizeof(char) * BUFFER_LEN,GFP_KERNEL);
		if (!dev->all_buffers[i]) { // allocation error
			printk(ALLOCATION_ERROR);
			destroy_device_data(dev);
			return -1;
		}
	}

	// place at end of list
	if (!dev_list)
		dev_list = dev;
	else
		dev_list_last->next = dev;
	dev_list_last = dev;

	return 0;
}

device_data* get_device_data(int ino) {
	if (dev_list) {
		// scan all devices and return if ino is found
		device_data *dev = dev_list;
		while (dev->next != NULL) {
			if (dev->ino == ino)
				return dev;
			dev = dev->next;
		}
	}
	// ino not found
	return NULL;
}


void destroy_device_data(device_data* dev) {
	if (dev) {
		// free buffers
		for (int i=0; i<NUM_BUFFERS; i++)
			if (dev->all_buffers[i])
				kfree(dev->all_buffers[i]);
		// free device data
		kfree(dev);
	}
}

void destroy_device_list(void) {
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
	int ino = file->f_inode->i_ino;
	if (!get_device_data(ino)) {
		if (add_device_data(ino) != 0) {
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
    //printk("device_read(%p,%d) - operation not supported yet (last written - %s)\n", file, length, Message);

    return 0;
}

static ssize_t device_write(struct file *file, const char __user * buffer, size_t length, loff_t * offset) {
	return 0;
}

/*static long device_ioctl(struct file*   file, unsigned int ioctl_num, unsigned long  ioctl_param) {
	return 0;
}*/

struct file_operations Fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};

static int __init message_slot_init(void) {
	printk("Loading message_slot\n");
	major = register_chrdev(0, DEVICE_RANGE_NAME, &Fops);

	if (major < 0) {
		printk(KERN_ALERT " registering device failed: %d\n", major);
	}
	return 0;
}

static void __exit message_slot_cleanup(void) {
	printk("Unloading message_slot\n");
	destroy_device_list();
	unregister_chrdev(major, DEVICE_RANGE_NAME);
}

module_init(message_slot_init);
module_exit(message_slot_cleanup);
