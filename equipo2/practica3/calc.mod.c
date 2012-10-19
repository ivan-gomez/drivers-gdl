#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x78c14ddb, "module_layout" },
	{ 0x8adef804, "cdev_del" },
	{ 0xd46f48e8, "cdev_init" },
	{ 0xd0d8621b, "strlen" },
	{ 0x6d597694, "device_destroy" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x91715312, "sprintf" },
	{ 0x50eedeb8, "printk" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0xb4390f9a, "mcount" },
	{ 0xc60796c9, "device_create" },
	{ 0x80d78e54, "cdev_add" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x37ff4c06, "copy_from_user_overflow" },
	{ 0x6dcd7881, "class_destroy" },
	{ 0xb742fd7, "simple_strtol" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x34d76c42, "__class_create" },
	{ 0x29537c9e, "alloc_chrdev_region" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "C60A8C25E2E1A381BEEC389");
