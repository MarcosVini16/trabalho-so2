#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define PROC_NAME "position"
#define INTERVALO_MS 4000

static struct proc_dir_entry *proc_entry;
static uint8_t quadrante = 0;

module_param(quadrante, byte, 0444);
MODULE_PARM_DESC(quadrante, "Quadrante inicial (0-3)");

static struct timer_list meu_timer;

static void timer_callback(struct timer_list *t)
{
    quadrante = (uint8_t) get_random_u32_below(4);
    mod_timer(&meu_timer, jiffies + msecs_to_jiffies(INTERVALO_MS));
}

static ssize_t proc_read(struct file *file, char __user *buf,
                          size_t count, loff_t *ppos)
{
    char tmp[8];
    int len;
    if (*ppos > 0) return 0;
    len = scnprintf(tmp, sizeof(tmp), "%u\n", quadrante);
    if (count < len) return -EINVAL;
    if (copy_to_user(buf, tmp, len)) return -EFAULT;
    *ppos = len;
    return len;
}

static const struct proc_ops proc_ops = {
    .proc_read = proc_read,
};

static int __init mod_init(void)
{
    proc_entry = proc_create(PROC_NAME, 0444, NULL, &proc_ops);
    if (!proc_entry) return -ENOMEM;
    timer_setup(&meu_timer, timer_callback, 0);
    mod_timer(&meu_timer, jiffies + msecs_to_jiffies(INTERVALO_MS));
    return 0;
}

static void __exit mod_exit(void)
{
    timer_delete_sync(&meu_timer);
    proc_remove(proc_entry);
}

module_init(mod_init);
module_exit(mod_exit);
MODULE_LICENSE("GPL");