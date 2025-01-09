#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");


static void my_timer_function(struct timer_list *t);

static int global_var = 0;
static struct timer_list my_timer;
static bool timer_running = false;
static dev_t dev = 0;
static struct class *dev_class;
static struct cdev cdev;
static int Major, ret;
static struct device *dev_ptr;
static int my_char_driver_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "My char driver opened\n");
    return 0;
}

static int my_char_driver_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "My char driver released\n");
    return 0;
}

static ssize_t my_char_driver_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    int value = global_var;
    return copy_to_user(buf, &value, sizeof(value)) ? -EFAULT : sizeof(value);
}

static ssize_t my_char_driver_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    int value;
    if (copy_from_user(&value, buf, sizeof(value)))
        return -EFAULT;
    global_var = value;
    printk(KERN_INFO "Global variable set to %d\n", global_var);
    return count;
}


static void my_timer_function(struct timer_list *t) {
    global_var++;
    printk(KERN_INFO "Timer fired! Global variable incremented to %d\n", global_var);
    mod_timer(t, jiffies + msecs_to_jiffies(1000)); // Fire again in 1 second
}

static ssize_t start_timer_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    if (!timer_running) {
        timer_setup(&my_timer, my_timer_function, 0);
        mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));
        timer_running = true;
        printk(KERN_INFO "Timer started\n");
    }
    return count;
}

static ssize_t stop_timer_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    if (timer_running) {
        del_timer_sync(&my_timer);
        timer_running = false;
        printk(KERN_INFO "Timer stopped\n");
    }
    return count;
}

static ssize_t reset_var_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    global_var = 0;
    printk(KERN_INFO "Global variable reset to 0\n");
    return count;
}

static DEVICE_ATTR(start_timer, 0200, NULL, start_timer_store);
static DEVICE_ATTR(stop_timer, 0200, NULL, stop_timer_store);
static DEVICE_ATTR(reset_var, 0200, NULL, reset_var_store);


static const struct file_operations my_char_driver_fops = {
    .owner = THIS_MODULE,
    .open = my_char_driver_open,
    .release = my_char_driver_release,
    .read = my_char_driver_read,
    .write = my_char_driver_write,
};

static int __init my_char_driver_init(void) {
    Major = register_chrdev(0, "new_dev_1", &my_char_driver_fops);
    if (Major < 0){
        printk(KERN_ALERT "Failed");
        return Major;
    }
    printk(KERN_INFO "%d", Major);
    dev = MKDEV(Major, 0);
    dev_class = class_create("new_dev_1");
    if (IS_ERR(dev_class))
    {
        printk(KERN_ERR "Класс полетел по наклонной\n");
        unregister_chrdev(0, "new_dev_1");
        return dev_class;
    }
    dev_ptr = device_create(dev_class, NULL, dev, NULL, "my_char_dev");
    if (IS_ERR(dev_ptr)) {
        printk(KERN_ERR "Failed to allocate char device region\n");
        class_destroy(dev_class);
        return dev_ptr;
    }

    cdev_init(&cdev, &my_char_driver_fops);
    ret = cdev_add(&cdev, dev, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to add char device\n");
        unregister_chrdev(Major, "new_dev_1");
        return ret;
    }

    ret = device_create_file(dev_ptr, &dev_attr_start_timer);
    if (ret < 0) {
        printk(KERN_ERR "Failed to create sysfs entry for start_timer\n");
        device_destroy(dev_class, dev);
        cdev_del(&cdev);
        unregister_chrdev(Major, "new_dev_1");
        return ret;
    }
    ret = device_create_file(dev_ptr, &dev_attr_stop_timer);
    if (ret < 0) {
        printk(KERN_ERR "Failed to create sysfs entry for stop_timer\n");
        device_remove_file(ret, &dev_attr_start_timer);
        device_destroy(dev_class, dev);
        cdev_del(&cdev);
        unregister_chrdev(Major, "new_dev_1");
        return ret;
    }
    ret = device_create_file(dev_ptr, &dev_attr_reset_var);
    if (ret < 0) {
        printk(KERN_ERR "Failed to create sysfs entry for reset_var\n");
        device_remove_file(ret, &dev_attr_stop_timer);
        device_remove_file(ret, &dev_attr_start_timer);
        device_destroy(dev_class, dev);
        cdev_del(&cdev);
        unregister_chrdev(Major, "new_dev_1");
        return ret;
    }

    printk(KERN_INFO "My char driver initialized\n");
    return 0;
}

static void __exit my_char_driver_exit(void) {
    if(timer_running) del_timer_sync(&my_timer);
    device_remove_file(NULL, &dev_attr_start_timer);
    device_remove_file(NULL, &dev_attr_stop_timer);
    device_remove_file(NULL, &dev_attr_reset_var);
    device_destroy(dev_class, dev);
    cdev_del(&cdev);
    unregister_chrdev(Major, "new_dev_1");
    printk(KERN_INFO "My char driver exited\n");
}

module_init(my_char_driver_init);
module_exit(my_char_driver_exit);
