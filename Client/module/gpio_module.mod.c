#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x85ccbf3a, "module_layout" },
	{ 0xf7592ac1, "cdev_del" },
	{ 0x5dd764a1, "class_destroy" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xedc03953, "iounmap" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x7afd5932, "gpiod_to_irq" },
	{ 0x464b110d, "gpio_to_desc" },
	{ 0x1d37eeed, "ioremap" },
	{ 0xbd28273d, "device_create" },
	{ 0xd3c2e8b5, "__class_create" },
	{ 0x13af9f3a, "cdev_add" },
	{ 0xe7f3de63, "cdev_init" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0xb43f9365, "ktime_get" },
	{ 0x8e865d3c, "arm_delay_ops" },
	{ 0x5f754e5a, "memset" },
	{ 0xae353d77, "arm_copy_from_user" },
	{ 0xa9cb2c6f, "try_module_get" },
	{ 0xe1907f54, "module_put" },
	{ 0x92997ed8, "_printk" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "9B9039C85EDA6B7A6775D06");
