#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>

static struct task_struct *task;
static int frequency = 1; // Частота в секундах

module_param(frequency, int, 0);
MODULE_PARM_DESC(frequency, "Frequency of messages in seconds");

static int thread_fn(void *data)
{
    while (!kthread_should_stop())
    {
        printk(KERN_INFO "Hello from kernel thread! Frequency: %d secondsn", frequency);
        ssleep(frequency); // Задержка в секундах
    }
    return 0;
}

static int __init my_module_init(void)
{
    printk(KERN_INFO "Module loaded with frequency: %d secondsn", frequency);
    task = kthread_run(thread_fn, NULL, "my_kthread");
    if (IS_ERR(task))
    {
        printk(KERN_ERR "Failed to create kernel threadn");
        return PTR_ERR(task);
    }
    return 0;
}

static void __exit my_module_exit(void)
{
    if (task)
    {
        kthread_stop(task);
    }
    printk(KERN_INFO "Module unloadedn");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
