#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h> 
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/ktime.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("YCKiM");
MODULE_DESCRIPTION("Raspberry Pi GPIO LED Device Module");

#define BCM_IO_BASE   (0xFE000000)
#define GPIO_BASE           (BCM_IO_BASE + 0x200000)
#define GPIO_DEVICE "gpioPWM"
#define GPIO_MAJOR 200
#define GPIO_MINOR 0
#define CM 58000

static int IRQ_GPIO27 = 63;
static unsigned int Auto = 1;
ktime_t st, dist;

struct S_GPIO_REGS
{
    uint32_t GPFSEL[6];
    uint32_t Reserved0;
    uint32_t GPSET[2];
    uint32_t Reserved1;
    uint32_t GPCLR[2];
    uint32_t Reserved2;
    uint32_t GPLEV[2];
    uint32_t Reserved3;
    uint32_t GPEDS[2];
    uint32_t Reserved4;
    uint32_t GPREN[2];
    uint32_t Reserved5;
    uint32_t GPFEN[2];
    uint32_t Reserved6;
    uint32_t GPHEN[2];
    uint32_t Reserved7;
    uint32_t GPLEN[2];
    uint32_t Reserved8;
    uint32_t GPAREN[2];
    uint32_t Reserved9;
    uint32_t GPAFEN[2];
    uint32_t Reserved10;
    uint32_t GPPUD;
    uint32_t GPPUDCLK[2];
    uint32_t Reserved11[4];
} *gpio_regs;

typedef enum {
    GPIO_02 = 2,
    GPIO_03 = 3,
    GPIO_04 = 4,
    GPIO_05 = 5,
    GPIO_06 = 6,
    GPIO_07 = 7,
    GPIO_08 = 8,
    GPIO_09 = 9,
    GPIO_10 = 10,
    GPIO_11 = 11,
    GPIO_12 = 12,
    GPIO_13 = 13,
    GPIO_14 = 14,
    GPIO_15 = 15,
    GPIO_16 = 16,
    GPIO_17 = 17,
    GPIO_18 = 18,
    GPIO_19 = 19,
    GPIO_20 = 20,
    GPIO_21 = 21,
    GPIO_22 = 22,
    GPIO_23 = 23,
    GPIO_24 = 24,
    GPIO_25 = 25,
    GPIO_26 = 26,
    GPIO_27 = 27
} GPIO;

typedef enum {
    PULL_NONE = 0,
    PULL_DOWN = 1,
    PULL_UP = 2
} PUD;

typedef enum {
    GPIO_INPUT = 0b000,
    GPIO_OUTPUT = 0b001,
    GPIO_ALT_FUNC0 = 0b100,
    GPIO_ALT_FUNC1 = 0b101,
    GPIO_ALT_FUNC2 = 0b110,
    GPIO_ALT_FUNC3 = 0b111,
    GPIO_ALT_FUNC4 = 0b011,
    GPIO_ALT_FUNC5 = 0b010,
} FSEL;

void SetGPIOFunction(GPIO pin, FSEL code)
{
    int regIndex = pin / 10;
    int bit = (pin % 10) * 3;

    unsigned oldValue = gpio_regs->GPFSEL[regIndex];
    unsigned mask = 0b111 << bit;

    gpio_regs->GPFSEL[regIndex] = (oldValue & ~mask) | ((code << bit) & mask);
}

#define PWM_BASE (BCM_IO_BASE + 0x20C000)
#define PWM_CLK_BASE (BCM_IO_BASE + 0x101000)
#define PWMCLK_CTL  40
#define PWMCLK_DIV  41

struct S_PWM_REGS
{
    uint32_t CTL;
    uint32_t STA;
    uint32_t DMAC;
    uint32_t reserved0;
    uint32_t RNG1;
    uint32_t DAT1;
    uint32_t FIF1;
    uint32_t reserved1;
    uint32_t RNG2;
    uint32_t DAT2;
} *pwm_regs;

struct S_PWM_CTL {
    unsigned PWEN1 : 1;
    unsigned MODE1 : 1;
    unsigned RPTL1 : 1;
    unsigned SBIT1 : 1;
    unsigned POLA1 : 1;
    unsigned USEF1 : 1;
    unsigned CLRF1 : 1;
    unsigned MSEN1 : 1;
    unsigned PWEN2 : 1;
    unsigned MODE2 : 1;
    unsigned RPTL2 : 1;
    unsigned SBIT2 : 1;
    unsigned POLA2 : 1;
    unsigned USEF2 : 1;
    unsigned Reserved1 : 1;
    unsigned MSEN2 : 1;
    unsigned Reserved2 : 16;
} volatile *pwm_ctl;

struct S_PWM_STA {
    unsigned FULL1 : 1;
    unsigned EMPT1 : 1;
    unsigned WERR1 : 1;
    unsigned RERR1 : 1;
    unsigned GAPO1 : 1;
    unsigned GAPO2 : 1;
    unsigned GAPO3 : 1;
    unsigned GAPO4 : 1;
    unsigned BERR : 1;
    unsigned STA1 : 1;
    unsigned STA2 : 1;
    unsigned STA3 : 1;
    unsigned STA4 : 1;
    unsigned Reserved : 19;
} volatile *pwm_sta;

volatile unsigned int* pwm_clk_regs;

void pwm_frequency(uint32_t divi) {

    *(pwm_clk_regs + PWMCLK_CTL) = 0x5A000020;

    pwm_ctl->PWEN1 = 0;
    pwm_ctl->PWEN2 = 0;
    udelay(10);

    *(pwm_clk_regs + PWMCLK_DIV) = 0x5A000000 | (divi << 12);
    *(pwm_clk_regs + PWMCLK_CTL) = 0x5A000011;
}

void set_up_pwm_channels(void) {
    pwm_ctl->MODE1 = 0;
    pwm_ctl->RPTL1 = 0;
    pwm_ctl->SBIT1 = 0;
    pwm_ctl->POLA1 = 0;
    pwm_ctl->USEF1 = 0;
    pwm_ctl->CLRF1 = 1;
    pwm_ctl->MSEN1 = 1;

    pwm_ctl->MODE2 = 0;
    pwm_ctl->RPTL2 = 0;
    pwm_ctl->SBIT2 = 0;
    pwm_ctl->POLA2 = 0;
    pwm_ctl->USEF2 = 0;
    pwm_ctl->MSEN2 = 1;
}

void pwm_ratio_c1(unsigned n, unsigned m) {

    pwm_ctl->PWEN1 = 0;

    pwm_regs->RNG1 = m;
    pwm_regs->DAT1 = n;

    if (!pwm_sta->STA1) {
        if (pwm_sta->RERR1) pwm_sta->RERR1 = 1;
        if (pwm_sta->WERR1) pwm_sta->WERR1 = 1;
        if (pwm_sta->BERR) pwm_sta->BERR = 1;
    }
    udelay(10);

    pwm_ctl->PWEN1 = 1;
}

void pwm_ratio_c2(unsigned n, unsigned m) {

    pwm_ctl->PWEN2 = 0;

    pwm_regs->RNG2 = m;
    pwm_regs->DAT2 = n;

    if (!pwm_sta->STA2) {
        if (pwm_sta->RERR1) pwm_sta->RERR1 = 1;
        if (pwm_sta->WERR1) pwm_sta->WERR1 = 1;
        if (pwm_sta->BERR) pwm_sta->BERR = 1;
    }
    udelay(10);

    pwm_ctl->PWEN2 = 1;
}

static char msg[BLOCK_SIZE] = {0, 0,};
GPIO L[2] = {GPIO_12, GPIO_18};
GPIO R[2] = {GPIO_13, GPIO_19};
FSEL FB[2] = {GPIO_ALT_FUNC0, GPIO_ALT_FUNC5};
struct cdev gpio_cdev;
dev_t gpio_dev;
struct class* gpio_class;

static int gpio_open(struct inode *inod, struct file *fil);
static ssize_t gpio_read(struct file *inode, char *buff, size_t len, loff_t *off);
static ssize_t gpio_write(struct file *inode, const char *buff, size_t len, loff_t *off);
static int gpio_close(struct inode *inod, struct file *fil);
static long gpio_ioctl(struct file* inode, unsigned int cmd, unsigned long arg);

static struct file_operations gpio_fops = {
	.owner = THIS_MODULE,
	.read = gpio_read,
	.write = gpio_write,
	.open = gpio_open,
	.release = gpio_close,
    .unlocked_ioctl = gpio_ioctl
};

static irqreturn_t det_edge(int irq, void *data){
    int state = ((gpio_regs->GPLEV[0])&(1<<27)) > 0;
    if(state){
        st = ktime_get();
    }
    else{
        dist = ktime_get() - st;
        //printk("cur dist is %lld\n", dist);
        if(dist < 30*CM){
            pwm_ratio_c2(0, 1000);
            pwm_ratio_c1(0, 1000);
            Auto = 0;
        }
        else Auto = 1;
    }
    
    return IRQ_HANDLED;
}


static int gpio_open(struct inode *inod, struct file *file){
	printk(KERN_INFO "GPIO Device opened (%d/%d)\n", MAJOR(gpio_cdev.dev), MINOR(gpio_cdev.dev));
	try_module_get(THIS_MODULE);
	return 0;
}
static ssize_t gpio_read(struct file *inode, char *buff, size_t len, loff_t *off){
	if(msg[0]=='\0') return 0;
    if (BLOCK_SIZE < len) len = BLOCK_SIZE;
    signed int not_copied = copy_from_user(msg, buff, len);
    int N = (msg[0] - '0')*10 + msg[1] - '0';
	if(N > 1 && N < 28){
        return ((gpio_regs->GPLEV[0])&(1<<N)) > 0;
    }
    else return 0;
}
static ssize_t gpio_write(struct file* inode, const char* buff, size_t len, loff_t* off) {
    int not_copied;
    if (BLOCK_SIZE < len) len = BLOCK_SIZE;
    not_copied = copy_from_user(msg, buff, len);
    if(len == 3){
        int N = (msg[1] - '0')*10 + msg[2] - '0';
        if(N > 1 && N < 28) {
            if(msg[0] == 'L') {
                gpio_regs->GPCLR[0] |= (1 << N);
                SetGPIOFunction(N, GPIO_INPUT);}
            else if(msg[0] == 'H') {
                SetGPIOFunction(N, GPIO_OUTPUT);
                gpio_regs->GPSET[0] |= (1 << N);}
        }
    }
    else if(len == 2) {
        if(msg[0] == 'B' && msg[1] == 'B') Auto = 1;
        SetGPIOFunction(L[msg[0]=='B'], FB[msg[0]=='B']);
        SetGPIOFunction(R[msg[1]=='B'], FB[msg[1]=='B']);
        SetGPIOFunction(L[msg[0]!='B'], GPIO_INPUT);
        SetGPIOFunction(R[msg[1]!='B'], GPIO_INPUT);
    }//printk(KERN_INFO "GPIO Device(%d) write : %c\n", my_pwm_driver_major, msg[1]);
    return len - not_copied;
}
static int gpio_close(struct inode *inod, struct file *fil){
	printk(KERN_INFO "GPIO Device closed (%d/%d)\n", MAJOR(gpio_cdev.dev), MINOR(gpio_cdev.dev));
	module_put(THIS_MODULE);
	return 0;
}
static long gpio_ioctl(struct file* inode, unsigned int cmd, unsigned long arg) {
    if(cmd >> 31) pwm_ratio_c2((((~cmd) + 1)&2147483647)*Auto, 1000);
    else pwm_ratio_c1(cmd*Auto, 1000);
    return 0;
}

int __init init_module(void){
    printk(KERN_INFO "Hello module!\n");

    gpio_dev = MKDEV(GPIO_MAJOR, GPIO_MINOR);
    register_chrdev_region(gpio_dev, 1, GPIO_DEVICE);

    cdev_init(&gpio_cdev, &gpio_fops);
    gpio_cdev.owner = THIS_MODULE;

    if (cdev_add(&gpio_cdev, gpio_dev, 1) < 0) {
        printk(KERN_ALERT "cdev/-add failed\n");
        return -1;
    }
    if((gpio_class = class_create(THIS_MODULE, GPIO_DEVICE)) == NULL) {
    	printk(KERN_ALERT "class_add failed\n");
    	return -1;
    }
    if (device_create(gpio_class, NULL, gpio_dev, NULL, GPIO_DEVICE)==NULL) return -1;

    gpio_regs = (struct S_GPIO_REGS*)ioremap(GPIO_BASE, sizeof(struct S_GPIO_REGS));
    if (!gpio_regs)
    {
        goto fail_no_virt_mem;
    }
    pwm_regs = (struct S_PWM_REGS*)ioremap(PWM_BASE, sizeof(struct S_PWM_REGS));
    if (!pwm_regs)
    {
        goto fail_no_virt_mem;
    }
    pwm_ctl = (volatile struct S_PWM_CTL*)&pwm_regs->CTL;
    pwm_sta = (volatile struct S_PWM_STA*)&pwm_regs->STA;
    pwm_clk_regs = ioremap(PWM_CLK_BASE, 4096);
    if (!pwm_clk_regs)
    {
        goto fail_no_virt_mem;
    }
    SetGPIOFunction(GPIO_12, GPIO_INPUT);
    SetGPIOFunction(GPIO_18, GPIO_INPUT);
    SetGPIOFunction(GPIO_13, GPIO_INPUT);
    SetGPIOFunction(GPIO_19, GPIO_INPUT);
    set_up_pwm_channels();
    
    IRQ_GPIO27 = gpio_to_irq(27);
    request_irq(IRQ_GPIO27, det_edge, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "yckim", NULL);
    
    pwm_frequency(19);
    
    return 0;

fail_no_virt_mem:

    if (gpio_regs)
        iounmap(gpio_regs);
    if (pwm_regs)
        iounmap(pwm_regs);
    if (pwm_clk_regs)
        iounmap(pwm_clk_regs);

    unregister_chrdev_region(gpio_dev, 1);

    return -1;

}

void __exit cleanup_module(void)
{
    gpio_regs->GPCLR[12 / 32] |= (1 << (12 % 32));
    gpio_regs->GPCLR[13 / 32] |= (1 << (13 % 32));
    gpio_regs->GPCLR[18 / 32] |= (1 << (18 % 32));
    gpio_regs->GPCLR[19 / 32] |= (1 << (19 % 32));

    SetGPIOFunction(GPIO_12, GPIO_INPUT);
    SetGPIOFunction(GPIO_13, GPIO_INPUT);
    SetGPIOFunction(GPIO_18, GPIO_INPUT);
    SetGPIOFunction(GPIO_19, GPIO_INPUT);

    if (gpio_regs)
        iounmap(gpio_regs);
    if (pwm_regs)
        iounmap(pwm_regs);
    if (pwm_clk_regs)
        iounmap(pwm_clk_regs);

    printk(KERN_INFO "Goodbye module.\n");
    class_destroy(gpio_class);
    cdev_del(&gpio_cdev);
    unregister_chrdev_region(gpio_dev, 1);
}
