//#include <stdlib.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <asm/ioctl.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");

#define DEVICE_NAME "my_dev"
#define RESET 0
#define SET_LEN 1
#define MSG_BUFFER_LEN 10

static char msg_buffer[MSG_BUFFER_LEN] = "";

static struct cdev my_cdev;

static DECLARE_WAIT_QUEUE_HEAD(wq);

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static ssize_t device_ioctl(struct file *, unsigned int, unsigned long);
//static void set_len_buffer(unsigned long);

static dev_t major_num;
static int device_open_count = 0;

//static int MSG_BUFFER_LEN = 10;
//static char * msg_buffer = malloc(MSG_BUFFER_LEN * sizeof(char));

static int read_ptr = 0;
static int write_ptr = 0;

struct file_operations file_ops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
	.unlocked_ioctl = device_ioctl

};
/*
static void set_len_buffer(unsigned long length){
	MSG_BUFFER_LEN = length;
	for (int i = 0; i < MSG_BUFFER_LEN; i++){
		msg_buffer[i] = 'A';
	}
	msg_buffer[MSG_BUFFER_LEN] = '\0';
	printk(KERN_INFO "New msg_buffer = %s\n", msg_buffer);
}
*/
static ssize_t device_read(struct file *flip, char *buffer, size_t len, loff_t * offset){
	int bytes_read = 0;
	
	printk(KERN_INFO "Inside read\n");
	
	wait_event_interruptible(wq, write_ptr != read_ptr);
	
	if (msg_buffer[read_ptr] != '\0'){
		while(len && (msg_buffer[read_ptr] != '\0')) {
			put_user(msg_buffer[read_ptr], buffer++);
			len--;
			bytes_read++;
			read_ptr = (read_ptr + 1) % MSG_BUFFER_LEN;
		}
	} else {
		printk(KERN_INFO "Buffer is empty!!!\n");	
	}
	
	printk(KERN_INFO "Woken up from reading\n");
	
	return bytes_read;
}

static ssize_t device_write(struct file *flip, const char *buffer, size_t len, loff_t * offset){
	int bytes_write = 0;
	
	if((write_ptr == read_ptr) && (flip->f_flags & O_NONBLOCK)){
		printk(KERN_INFO "Inside write\n");
		
		while (len) {
			get_user(msg_buffer[write_ptr], buffer++);
			len--;
			bytes_write++;
			write_ptr = (write_ptr + 1) % MSG_BUFFER_LEN;
		}
		
		printk(KERN_INFO "Sheduling out inside write\n");
		
		wake_up_interruptible(&wq);
	} else {
		printk(KERN_INFO "Blocking!");
	}
	
	
	return bytes_write;
}

static ssize_t device_ioctl(struct file *flip, unsigned int cmd, unsigned long arg){
	switch (cmd){
		case RESET:
			for (int i = 0; i < MSG_BUFFER_LEN; i++){
				msg_buffer[i] = '\0';
				read_ptr = 0;
				write_ptr = 0;
				
				
			}
			printk(KERN_INFO "Buffer was reset\n");
			break;
		case SET_LEN:
			//#define MSG_BUFFER_LEN arg
			printk(KERN_INFO "Buffers's length was set\n");
			break;
		default:
			printk(KERN_INFO "Command is unknown!\n");
			return -EINVAL;
	}
	return 0;
	
}

static int device_open(struct inode *inode, struct file *file) {
	if (device_open_count) {
		return -EBUSY;
	}
	
	device_open_count++;
	try_module_get(THIS_MODULE);
	return 0;
}

static int device_release(struct inode *inode, struct file *file) {
	
	device_open_count--;
	module_put(THIS_MODULE);
	return 0;
}

static int __init dev_init(void){
	major_num = register_chrdev(0, "my_dev", &file_ops);
	if (major_num < 0){
		printk(KERN_ALERT "Registering char device failed with %d\n", major_num);
		return major_num;
	}
	
	cdev_init(&my_cdev, &file_ops);
	
	if(cdev_add(&my_cdev, major_num, 1) < 0){
		unregister_chrdev(major_num, DEVICE_NAME);
		return -1;
	}
	
	printk(KERN_INFO "I was assigned major number %d", major_num);
	
	return 0;
}

static void __exit dev_exit(void){
	cdev_del(&my_cdev);
	unregister_chrdev(major_num, DEVICE_NAME);
	printk(KERN_INFO "Goodbye world 1.\n");
	//free(msg_buffer);
	
}

module_init(dev_init);
module_exit(dev_exit);
