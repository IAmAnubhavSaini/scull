/* Copyrighting or licensing it doesn't make sense to me, ask psankar.
 * ----------------------------------------------------------------------------
 * Purpose: scull device/driver as sprinkled in 3rd chapter of O'REILLY's LDD.
 * ----------------------------------------------------------------------------
 * Written by : Sankar P @ https://github.com/psankar/scull
 * Modified by: Anubhav S @ https://github.com/IAmAnubhavSaini/scull
 * ----------------------------------------------------------------------------
 * I was going to write it for myself, then "don't reinvent wheel" kicked in, 
 * and I found that psankar already had solved this problem. So I forked!
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define FIRST_MINOR_NUMBER 0
#define COUNT_OF_CONTIGOUS_DEVICES 4

/* Device that we will use */
dev_t my_scull_dev;

/* cdev structure that we will use to add/remove the device to the kernel */
struct cdev cdev;

/* The core datastructure that will hold the data for our device */
char *my_scull_dev_data = NULL;

int helloworld_driver_open(struct inode *inode, struct file *filep)
{
	return 0;
}

int helloworld_driver_release(struct inode *inode, struct file *filep)
{
	return 0;
}

ssize_t helloworld_driver_read(struct file * filep, char *buff, size_t count, loff_t * offp)
{
	int device_data_length;
	device_data_length = strlen(my_scull_dev_data);

	/* No more data to read. */
	if (*offp >= device_data_length)
		return 0;

	/* We copy either the full data or the number of bytes
	 * asked from userspace, depending on whatever is the smallest.
	 */
	if ((count + *offp) > device_data_length)
		count = device_data_length;

	/* function to copy kernel space buffer to user space */
	if (copy_to_user(buff, my_scull_dev_data, count) != 0) {
		printk(KERN_ALERT "Kernel Space to User Space copy failure\n");
		return -EFAULT;
	}

	/* Increment the offset to the number of bytes read */
	*offp += count;

	/* Return the number of bytes copied to the user space */
	return count;
}

ssize_t helloworld_driver_write(struct file * filep, const char *buff, size_t count, loff_t * offp)
{
	/* Free the previouosly stored data */
	if (my_scull_dev_data)
		kfree(my_scull_dev_data);

	/* Allocate new memory for holding the new data */
	my_scull_dev_data = kmalloc((count * (sizeof(char *))), GFP_KERNEL);

	/* function to copy user space buffer to kernel space */
	if (copy_from_user(my_scull_dev_data, buff, count) != 0) {
		printk(KERN_ALERT "User Space to Kernel Space copy failure\n");
		return -EFAULT;
	}

	/* Since our device is an array, we need to do this to terminate the string.
	 * Last character will get overwritten by a \0. Incase of an actual file,
	 * we will probably update the file size, IIUC.
	 */
	my_scull_dev_data [count] = '\0';

	/* Return the number of bytes actually written. */
	return count;
}

/* Extending the file operations to meet our needs */
struct file_operations helloworld_driver_fops = {
	.owner = THIS_MODULE,
	.open = helloworld_driver_open,
	.release = helloworld_driver_release,
	.read = helloworld_driver_read,
	.write = helloworld_driver_write,
};

/* Initialization Function for the kernel module */
static int helloworld_driver_init(void)
{
	int err;

	printk(KERN_INFO "Hello World from SCULL\n");

	/* Allocate a dynamic device number for your driver. If you want to hardcode a fixed major number for your 
	 * driver, you should use different APIs 'register_chrdev_region', 'MKDEV'. But the following is better. */
	if (!alloc_chrdev_region(&my_scull_dev, FIRST_MINOR_NUMBER, COUNT_OF_CONTIGOUS_DEVICES, "HelloWorldDriver")) {
		printk(KERN_INFO "Device Number successfully allocated.\n");
		/* If you do a cat /proc/devices you should be able to find our Driver registered. */
		cdev_init(&cdev, &helloworld_driver_fops);
		cdev.owner = THIS_MODULE;
		err = cdev_add(&cdev, my_scull_dev, COUNT_OF_CONTIGOUS_DEVICES);
		if (err)
			printk(KERN_NOTICE "Error [%d] adding HelloWorldDriver", err);

		/* 100 below is an approximation of the number of chars in the initial string */
		my_scull_dev_data = kmalloc(((100 * sizeof(char *))), GFP_KERNEL);
		/* the initial string, so that the first cat on our device won't be empty */
		strcpy(my_scull_dev_data, "Let there be peace and happiness");

	} else
		printk(KERN_ALERT "HelloWorldDriver could not get a Device number\n");

	return 0;
}

/* Unloading function for the kernel module */
static void helloworld_driver_exit(void)
{
	cdev_del(&cdev);

	/* Free the device number when the driver is no longer available */
	unregister_chrdev_region(my_scull_dev, COUNT_OF_CONTIGOUS_DEVICES);

	printk(KERN_INFO "Goodbye World\n");
}

module_init(helloworld_driver_init);
module_exit(helloworld_driver_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Sankar P");
MODULE_DESCRIPTION("A Simple Character Device Driver");
