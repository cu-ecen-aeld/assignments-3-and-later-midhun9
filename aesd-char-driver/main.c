/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
#include "aesdchar.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Midhun"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev *chardev;
	PDEBUG("open");
	/**
	 * TODO: handle open
	 */
	// store data into chardev
	chardev = container_of(inode->i_cdev, struct aesd_dev, cdev);
	//assign it into private data
	filp->private_data = chardev;
	return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
	 * TODO: handle release
	 */
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;
	struct aesd_dev *chardev = filp->private_data;
        struct aesd_buffer_entry *entry = NULL;
        size_t entry_offset = 0;
        size_t num_bytes_to_read = 0;
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle read
	 */

	if (mutex_lock_interruptible(&chardev->lock))
		return -ERESTARTSYS;
	
	entry = aesd_circular_buffer_find_entry_offset_for_fpos(&chardev->cb, *f_pos, &entry_offset);

	if(entry == NULL)
	{
		retval = 0;
		goto out;
	}

	num_bytes_to_read = entry->size - entry_offset;
	if(num_bytes_to_read > count)
	{
		num_bytes_to_read = count;
	}

	if((copy_to_user(buf, entry->buffptr + entry_offset, num_bytes_to_read))) 
	{
		retval = -EFAULT;
		goto out;
	}

	*f_pos += num_bytes_to_read;
	
	mutex_unlock(&chardev->lock);
	return num_bytes_to_read;
out:
	mutex_unlock(&chardev->lock);
	return retval;

}


ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = -ENOMEM;
	struct aesd_dev *chardev = filp->private_data;
	const char *buffer_add = NULL;
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle write
	 */
	if(mutex_lock_interruptible(&chardev->lock))
		return -ERESTARTSYS;

	if(chardev->zerosize == false)
	{
		chardev->entry_local.buffptr = kzalloc(count, GFP_KERNEL); // enters only once at the start
		chardev->zerosize = true;
	}
	else
	{
		chardev->entry_local.buffptr = krealloc(chardev->entry_local.buffptr, chardev->entry_local.size + count, GFP_KERNEL);
	}


	if(chardev->entry_local.buffptr == 0)
       	{
                mutex_unlock(&chardev->lock);
                return retval;
        }

	if(copy_from_user((void *)&chardev->entry_local.buffptr[chardev->entry_local.size], buf, count))
        {
                retval = -EFAULT;
                mutex_unlock(&chardev->lock);
                return retval;
        }
	
	chardev->entry_local.size += count;

	if(strchr(chardev->entry_local.buffptr, '\n') != 0)
	{

		buffer_add = aesd_circular_buffer_add_entry(&chardev->cb, &chardev->entry_local);
		if(buffer_add != NULL)
			kfree(buffer_add);
		
		chardev->entry_local.buffptr = 0;
		chardev->entry_local.size = 0;
	}
	
	retval = count;
	mutex_unlock(&chardev->lock);
	return retval;

}

struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}



int aesd_init_module(void)
{
	dev_t dev = 0;
	int result;
	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");
	aesd_major = MAJOR(dev);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	/**
	 * TODO: initialize the AESD specific portion of the device
	 */
	mutex_init(&aesd_device.lock);

	result = aesd_setup_cdev(&aesd_device);

	if( result ) {
		unregister_chrdev_region(dev, 1);
	}
	return result;

}

void aesd_cleanup_module(void)
{
	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 */
	aesd_circular_buffer_clean(&aesd_device.cb);

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
