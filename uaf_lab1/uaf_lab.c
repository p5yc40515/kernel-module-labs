#include <linux/module.h>  // for module macros and functions
#include <linux/kernel.h>  // for printk and other kernel functions
#include <linux/init.h>  // for module initialization and cleanup
#include <linux/slab.h>  // for kmalloc and kfree
#include <linux/miscdevice.h> // for misc device registration
#include <linux/fs.h> // for file operations
#include <linux/uaccess.h> // for copy_to_user and copy_from_user
#include <linux/ioctl.h> // for ioctl definitions
#include <linux/delay.h> // for msleep

struct uaf_object {
    char *buffer;
    int length;
};

static struct uaf_object *obj;


#define USE_CMD _IO('u', 1) // Define ioctl command for using the object
#define FREE_CMD _IO('u', 2) // Define ioctl command for freeing the object

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    // Implement ioctl handling logic here
    if (cmd  == USE_CMD) {
        
        struct uaf_object *local_obj = obj; // Store the current pointer value in a local variable
        
        if (!local_obj) {
            printk(KERN_ERR "uaf_object is NULL \n");
            return -EINVAL;
        }
        
        char *local_buf = local_obj->buffer; // Store the current buffer pointer value in a local variable

        if (!local_buf) {
            printk(KERN_ERR "Buffer is still NULL \n");
            return -EINVAL;
        }
        msleep(5000);

        printk(KERN_INFO "USE_CMD received, buffer content: %s\n", local_buf);


    } else if (cmd == FREE_CMD) {
        if (obj) {
            if (obj->buffer) {
                kfree(obj->buffer);
                obj->buffer = NULL;
            }
            kfree(obj);
            obj = NULL;
            printk(KERN_INFO "FREE_CMD ioctl received, object freed\n");
        } else {
            printk(KERN_ERR "uaf_object is already NULL \n");
            return -EINVAL;
        }
    } else {
        printk(KERN_ERR "Invalid ioctl command \n");
        return -EINVAL;
    }
    return 0;
}
static const struct file_operations uaf_fops = {
    .owner = THIS_MODULE,
    .read = NULL, // No read operation
    .write = NULL, // No write operation
    .unlocked_ioctl = my_ioctl, // Implement ioctl handler
};

static struct miscdevice uaf_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "uaf_lab",
    .fops = &uaf_fops,
};

static int __init uaf_lab_init(void) {

    int ret = misc_register(&uaf_misc_device);
    if (ret) {
        printk(KERN_ERR "Failed to register misc device\n");
        return ret;
    }

    obj = kmalloc(sizeof(struct uaf_object), GFP_KERNEL);
    if (!obj) {
        printk(KERN_ERR "Failed to allocate memory for uaf_object\n");
        misc_deregister(&uaf_misc_device);
        return -ENOMEM;
    }
    obj->length = 0;
    obj->buffer = NULL;

    obj->buffer = kmalloc(128, GFP_KERNEL);
    if (!obj->buffer) {
        printk(KERN_ERR "Failed to allocate memory for buffer\n");
        kfree(obj);
        obj = NULL;
        misc_deregister(&uaf_misc_device);
        return -ENOMEM;
    }

    obj->buffer[0] = 'H';
    obj->buffer[1] = 'e';
    obj->buffer[2] = 'l';
    obj->buffer[3] = 'l';
    obj->buffer[4] = 'o';
    obj->buffer[5] = '\0';
    obj->length = 5;

   
    printk(KERN_INFO "uaf_lab1 module loaded\n");
    return 0;
}

static void __exit uaf_lab_exit(void) {

    if (obj) {
        if (obj->buffer) {
            kfree(obj->buffer);
        }
        kfree(obj);
        obj = NULL;
    }
    misc_deregister(&uaf_misc_device);
    printk(KERN_INFO "uaf_lab1 module unloaded\n");
}

module_init(uaf_lab_init);
module_exit(uaf_lab_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Use-After-Free Lab Module");
MODULE_AUTHOR("P5y");