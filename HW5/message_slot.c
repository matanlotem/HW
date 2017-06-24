#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

#define SUCCESS 0
#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 120
#define DEVICE_FILE_NAME "simple_message_slot"

static int major;

static int device_open(struct inode *inode, struct file *file) {
	printk("device_open(%p)\n", file);
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
	unregister_chrdev(major, DEVICE_RANGE_NAME);
}

module_init(message_slot_init);
module_exit(message_slot_cleanup);
