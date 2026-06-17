// kernel/position.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/random.h>
#include <linux/jiffies.h>

#include "position_ioctl.h"

#define INTERVALO_MS 4000
#define INTERVALO_JIFFIES msecs_to_jiffies(INTERVALO_MS)

static uint8_t quadrante = 0;
static unsigned long last_update;

module_param(quadrante, byte, 0444);
MODULE_PARM_DESC(quadrante, "Quadrante inicial (0-3)");

static long position_ioctl(struct file *file, unsigned int cmd,
                            unsigned long arg)
{
    switch (cmd) {
    case POSITION_GET_QUADRANT: {
        uint8_t q;

        // sorteia novo quadrante se passou o cooldown
        if (last_update == 0 ||
            time_after(jiffies, last_update + INTERVALO_JIFFIES)) {
            quadrante = (uint8_t) get_random_u32_below(4);
            last_update = jiffies;
        }
        q = quadrante;

        if (copy_to_user((uint8_t __user *)arg, &q, sizeof(q)))
            return -EFAULT;
        return 0;
    }
    default:
        return -ENOTTY; // erro padrão pra ioctl desconhecido
    }
}

static const struct file_operations position_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = position_ioctl,
};

static struct miscdevice position_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = POSITION_DEVICE_NAME,
    .fops  = &position_fops,
    .mode  = 0444,  // leitura para todos
};

static int __init mod_init(void)
{
    int ret = misc_register(&position_misc);
    if (ret) {
        pr_err("position: misc_register falhou: %d\n", ret);
        return ret;
    }
    last_update = 0; // força sorteio na primeira leitura
    pr_info("position: registrado em /dev/%s\n", POSITION_DEVICE_NAME);
    return 0;
}

static void __exit mod_exit(void)
{
    misc_deregister(&position_misc);
    pr_info("position: removido\n");
}

module_init(mod_init);
module_exit(mod_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Quadrante simulado, sorteado sob demanda com cooldown");