#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
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
__used __section(__versions) = {
	{ 0xdd8f8694, "module_layout" },
	{ 0x37669270, "single_release" },
	{ 0x40970142, "seq_read" },
	{ 0x1be63b22, "seq_lseek" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x9cfebac4, "remove_proc_entry" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0x24ff48a4, "proc_create" },
	{ 0x75c6269a, "proc_mkdir" },
	{ 0xc5850110, "printk" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0x2d3385d3, "system_wq" },
	{ 0xa570351f, "init_task" },
	{ 0xb19a5453, "__per_cpu_offset" },
	{ 0xb58aeaab, "kernel_cpustat" },
	{ 0x375cb97e, "seq_printf" },
	{ 0x1d8f45ba, "single_open" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x5ab904eb, "pv_ops" },
	{ 0xdbf17652, "_raw_spin_lock" },
	{ 0x40c7247c, "si_meminfo" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "6C7A11AF821D26B898C6BC6");
