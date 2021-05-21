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
	{ 0xc79d2779, "module_layout" },
	{ 0x489f415d, "kernel_sendmsg" },
	{ 0x7aa1756e, "kvfree" },
	{ 0x754d539c, "strlen" },
	{ 0x79aa04a2, "get_random_bytes" },
	{ 0x1d92fa08, "simple_lookup" },
	{ 0xe4beccad, "sock_release" },
	{ 0xa6ebe15d, "generic_delete_inode" },
	{ 0xd9b85ef6, "lockref_get" },
	{ 0xfd8853e3, "dput" },
	{ 0x3096be16, "names_cachep" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0x88825aa1, "d_add" },
	{ 0x8af0bb4f, "mount_nodev" },
	{ 0xb44ad4b3, "_copy_to_user" },
	{ 0xfb578fc5, "memset" },
	{ 0xc62005e4, "kill_litter_super" },
	{ 0x4e0ecf27, "current_task" },
	{ 0x977f511b, "__mutex_init" },
	{ 0xc5850110, "printk" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0x7f7d557e, "set_nlink" },
	{ 0xa681fe88, "generate_random_uuid" },
	{ 0x40ed52d8, "dentry_path_raw" },
	{ 0x599fb41c, "kvmalloc_node" },
	{ 0x9c48574b, "simple_getattr" },
	{ 0x80299be8, "kmem_cache_alloc" },
	{ 0x58bcd4ff, "simple_dir_operations" },
	{ 0xd9e46c2b, "simple_setattr" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xe4492359, "kernel_recvmsg" },
	{ 0xa407be02, "register_filesystem" },
	{ 0xe953b21f, "get_next_ino" },
	{ 0x13da636d, "iput" },
	{ 0x69acdf38, "memcpy" },
	{ 0xf1e72618, "current_time" },
	{ 0xb7734e3a, "sock_create" },
	{ 0xa04a8ab9, "d_make_root" },
	{ 0x5b4537ed, "simple_statfs" },
	{ 0xe0f89817, "d_alloc_name" },
	{ 0x3fe9fbf1, "unregister_filesystem" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x257c1a8d, "new_inode" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x8b1f721f, "d_instantiate" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x1a7ee8f7, "inode_init_owner" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "E2F9AC0A927CC58954C50E4");
