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
	{ 0x6dcd7881, "class_destroy" },
	{ 0x6d597694, "device_destroy" },
	{ 0x8adef804, "cdev_del" },
	{ 0xc60796c9, "device_create" },
	{ 0x34d76c42, "__class_create" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x80d78e54, "cdev_add" },
	{ 0xd46f48e8, "cdev_init" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0x37ff4c06, "copy_from_user_overflow" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xb742fd7, "simple_strtol" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x50eedeb8, "printk" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "44874A7ABF8EB43388E74DB");
