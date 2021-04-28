#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/pagemap.h> /* PAGE_CACHE_SIZE */
#include <linux/fs.h>	   /* This is where libfs stuff is declared */

#include <linux/mutex.h>
#include <linux/net.h>
#include <net/sock.h>
#include <linux/tcp.h>
#include <linux/in.h>
#include <asm/uaccess.h>
#include <linux/socket.h>
#include <linux/slab.h>
#include <linux/uuid.h>

#include <asm/atomic.h>
#include <asm/uaccess.h> /* copy_to_user */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wooyme");

#define LFS_MAGIC 0x19920342

#define TMPSIZE 20

#define PORT 2325
#define POOL_SIZE 1
#define UUID_LEN 36
static const char IP[] = {127,0,0,1,'\0'};
static struct mutex socket_mutex_arr[POOL_SIZE];
static struct socket *socket_arr[POOL_SIZE] = {0};
static u32 create_address(u8 *ip){
    u32 addr = 0;
    int i;

    for(i=0; i<4; i++){
        addr += ip[i];
        if(i==3)
            break;
        addr <<= 8;
    }
    return addr;
}
static int init_socket(int index){
	struct sockaddr_in saddr;
	int ret;
	ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &socket_arr[index]);
    if(ret < 0){
        printk("Error: %d while creating first socket", ret);
        goto err;
    }
	memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    saddr.sin_addr.s_addr = htonl(create_address(IP));
	ret = socket_arr[index]->ops->connect(socket_arr[index], (struct sockaddr *)&saddr\
                        , sizeof(saddr), O_RDWR);
    if(ret && (ret != -EINPROGRESS)){
        printk("Error: %d while connecting using conn", ret);
        goto err;
    }
	return 0;
err:
	return -1;
}

static int init_mutex(int index){
	mutex_init(&socket_mutex_arr[index]);
	return 0;
}

static int tcp_client_send(struct socket *sock, const char *buf, const size_t length,unsigned long flags){
        struct msghdr msg;
        struct kvec vec;
        int len, written = 0, left = length;
        mm_segment_t oldmm;

        msg.msg_name    = 0;
        msg.msg_namelen = 0;

        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags   = flags;

        oldmm = get_fs(); set_fs(KERNEL_DS);
repeat_send:

        vec.iov_len = left;
        vec.iov_base = (char *)buf + written;

        len = kernel_sendmsg(sock, &msg, &vec, left, left);
        if((len == -ERESTARTSYS) || (!(flags & MSG_DONTWAIT) &&\
                                (len == -EAGAIN)))
                goto repeat_send;
        if(len > 0)
        {
                written += len;
                left -= len;
                if(left)
                        goto repeat_send;
        }
        set_fs(oldmm);
        return written ? written:len;
}
int tcp_client_receive(struct socket *sock, char *str,int max_size,unsigned long flags){
        struct msghdr msg;
        struct kvec vec;
        int len;
		mm_segment_t oldmm;
        msg.msg_name    = 0;
        msg.msg_namelen = 0;

        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags   = flags;

        vec.iov_len = max_size;
        vec.iov_base = str;
read_again:
        len = kernel_recvmsg(sock, &msg, &vec, 1, max_size, MSG_WAITALL);

        if(len == -EAGAIN || len == -ERESTARTSYS)
        {
                printk("error while reading: %d", len);

                goto read_again;
        }
        return len;
}
#define I 0
#define C 1
#define Q 2
#define W 3
struct kila_create{
	char *uuid;
	char *name;
	int length;
};
struct kila_query{
	char *uuid;
	size_t length;
	loff_t offset;
};
struct kila_write{
	char *uuid;
	char *buf;
	size_t length;
	loff_t offset;
};
static int socket_write(struct kila_write *k_write){
	int i,index;
	get_random_bytes(&i,1);
	index = i%POOL_SIZE;
	struct mutex *socket_mutex = &socket_mutex_arr[index];
	mutex_lock(socket_mutex);
	struct socket *sock = socket_arr[index];
	size_t res = 0;
	int total_len = 1+UUID_LEN+sizeof(loff_t)+sizeof(size_t)+k_write->length;
	char buf[total_len];
	buf[0] = W;
	memcpy(buf+1,k_write->uuid,UUID_LEN);
	memcpy(buf+1+UUID_LEN,&k_write->offset,sizeof(loff_t));
	memcpy(buf+1+UUID_LEN+sizeof(loff_t),&k_write->length,sizeof(size_t));
	memcpy(buf+1+UUID_LEN+sizeof(loff_t)+sizeof(size_t),k_write->buf,k_write->length);
	tcp_client_send(sock,buf,total_len,MSG_DONTWAIT);
    tcp_client_receive(sock, (char *)&res,sizeof(size_t), MSG_WAITALL);
	mutex_unlock(socket_mutex);
	return res;
}
static int socket_query(struct kila_query *k_query,char *recv){
	int i,index;
	get_random_bytes(&i,1);
	index = i%POOL_SIZE;
	struct mutex *socket_mutex = &socket_mutex_arr[index];
	mutex_lock(socket_mutex);
	struct socket *sock = socket_arr[index];
	int total_len =1+UUID_LEN+sizeof(size_t)+sizeof(loff_t);
	char buf[total_len];
	buf[0] = Q;
	memcpy(buf+1,k_query->uuid,UUID_LEN);
	memcpy(buf+1+UUID_LEN,&k_query->offset,sizeof(loff_t));
	memcpy(buf+1+UUID_LEN+sizeof(loff_t),&k_query->length,sizeof(size_t));
	tcp_client_send(sock,buf,total_len,MSG_DONTWAIT);
    tcp_client_receive(sock, recv,k_query->length, MSG_WAITALL);
	mutex_unlock(socket_mutex);
	return 0;
}
static int socket_create(struct kila_create *k_create){
	int i,index;
	get_random_bytes(&i,1);
	index = i%POOL_SIZE;
	struct mutex *socket_mutex = &socket_mutex_arr[index];
	mutex_lock(socket_mutex);
	struct socket *sock = socket_arr[index];
	int res = 0;
	int total_len =1+UUID_LEN+sizeof(size_t)+k_create->length;
	char buf[total_len];
	buf[0] = C;
	memcpy(buf+1,k_create->uuid,UUID_LEN);
	memcpy(buf+1+UUID_LEN,&k_create->length,sizeof(size_t));
	memcpy(buf+1+UUID_LEN+sizeof(size_t),k_create->name,k_create->length);
	tcp_client_send(sock,buf,total_len,MSG_DONTWAIT);
    tcp_client_receive(sock, (char *)&res,sizeof(int), MSG_WAITALL);
	mutex_unlock(socket_mutex);
	return res;
}

static int socket_init(void){

	return 0;
}
struct kila_file {
    size_t k_size;
	char uuid[UUID_LEN+1];
};

static struct inode *lfs_make_inode(struct super_block *sb, int mode, const struct file_operations *fops){
	struct inode *inode;
	inode = new_inode(sb);
	if (!inode)
	{
		return NULL;
	}
	inode->i_mode = mode;
	inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
	inode->i_fop = fops;
	inode->i_ino = get_next_ino();
	return inode;
}


static int lfs_open(struct inode *inode, struct file *filp){
	filp->private_data = inode->i_private;
	return 0;
}

static ssize_t lfs_read_file(struct file *filp, char *buf,
							 size_t size, loff_t *offset){
	struct kila_file *k_file = (struct kila_file *)filp->private_data;
	size_t avaliable = k_file->k_size - *offset>size?size:k_file->k_size-*offset;
	if(avaliable<=0){
		return 0;
	}
	char tmp[512];
	struct kila_query k_query;
	int pointer = 0;
	int t_size = 0;
fuck:
	t_size = avaliable-pointer>512?512:avaliable-pointer;
	k_query.uuid = k_file->uuid;
	k_query.length = t_size;
	k_query.offset = *offset+pointer;
	socket_query(&k_query,tmp);
	if (copy_to_user(buf+pointer, tmp, t_size))
	 	return -EFAULT;
	if(avaliable-pointer>512){
		pointer+=t_size;
		goto fuck;
	}
	*offset += pointer+t_size;
	return pointer+t_size;
}


static ssize_t lfs_write_file(struct file *filp, const char *buf,
							  size_t size, loff_t *offset){
	struct kila_file *k_file = (struct kila_file *)filp->private_data;
	struct inode * inode = filp->f_inode;
	char tmp[size];
	memset(tmp, 0, size);
	if (copy_from_user(tmp, buf, size))
		return -EFAULT;
	struct kila_write k_write;
	k_write.uuid = k_file->uuid;
	k_write.buf = tmp;
	k_write.length = size;
	k_write.offset = *offset;
	int res = socket_write(&k_write);
	k_file->k_size = res;
	inode->i_size = res;
	return size;
}

static struct file_operations lfs_file_ops = {
	.open = lfs_open,
	.read = lfs_read_file,
	.write = lfs_write_file,
};

static int lfs_create(struct inode *dir,struct dentry *dentry,umode_t mode,bool unknown){
	struct inode *inode;
	inode = lfs_make_inode(dir->i_sb,S_IFREG | 0777, &lfs_file_ops);
	int error = -ENOSPC;
    if (inode) {
		int path_len;
		char *buf,*path;
		buf = __getname();
		if(!buf){
			return -ENOMEM;
		}
		path = dentry_path_raw(dentry,buf,PATH_MAX);
		path_len = strlen(path);
		printk("Path: %s",path);
		struct kila_file *k_file = kvmalloc(sizeof(struct kila_file),GFP_KERNEL);
		uuid_t uuid = {0};
		generate_random_uuid(uuid.b);
		snprintf(k_file->uuid, UUID_LEN+1, "%pUb", &uuid);
		k_file->k_size = 0;
		inode->i_private = k_file;

		struct kila_create k_create;
		k_create.uuid = k_file->uuid;
		k_create.name = path;
		k_create.length = path_len;
		socket_create(&k_create);
        d_instantiate(dentry, inode);
        dget(dentry);
        error = 0;
    }
	return 0;
}

static struct inode_operations lfs_dir_inode_operations = {
	.lookup		= simple_lookup,
	.create     = lfs_create
};


const struct inode_operations lwfs_inode_operations = {
	.setattr = simple_setattr,
	.getattr = simple_getattr,
};

static struct dentry *lfs_create_file(struct super_block *sb,
									  struct dentry *dir, const char *name){
	struct dentry *dentry;
	struct inode *inode;
	dentry = d_alloc_name(dir, name);
	if (!dentry)
		goto out;
	inode = lfs_make_inode(sb, S_IFREG | 0777, &lfs_file_ops);
	if (!inode)
		goto out_dput;
	struct kila_file * k_file = kvmalloc(sizeof(struct kila_file),GFP_KERNEL);
	uuid_t uuid = {0};
	generate_random_uuid(uuid.b);
	snprintf(k_file->uuid, UUID_LEN+1, "%pUb", &uuid);
	k_file->k_size = 0;
	inode->i_private = k_file;

	d_add(dentry, inode);
	return dentry;
out_dput:
	dput(dentry);
out:
	return 0;
}


static struct dentry *lfs_create_dir(struct super_block *sb,
									 struct dentry *parent, const char *name){
	struct dentry *dentry;
	struct inode *inode;

	dentry = d_alloc_name(parent, name);
	if (!dentry)
		goto out;

	inode = lfs_make_inode(sb, S_IFDIR | 0777, &simple_dir_operations);
	if (!inode)
		goto out_dput;
	inode->i_op = &lfs_dir_inode_operations;

	d_add(dentry, inode);
	return dentry;

out_dput:
	dput(dentry);
out:
	return 0;
}

static void lfs_create_files(struct super_block *sb, struct dentry *root){
	struct dentry *subdir;

	lfs_create_file(sb, root, "counter");

	// atomic_set(&subcounter, 0);
	// subdir = lfs_create_dir(sb, root, "subdir");
	// if (subdir)
	// 	lfs_create_file(sb, subdir, "subcounter", &subcounter);
}


static struct super_operations lfs_s_ops = {
	.statfs = simple_statfs,
	.drop_inode = generic_delete_inode,
};

static int lfs_fill_super(struct super_block *sb, void *data, int silent){
	struct inode *root;
	struct dentry *root_dentry;

	sb->s_blocksize = VMACACHE_SIZE;
	sb->s_blocksize_bits = VMACACHE_SIZE;
	sb->s_magic = LFS_MAGIC;
	sb->s_op = &lfs_s_ops;

	root = lfs_make_inode(sb, S_IFDIR | 0777, &simple_dir_operations);
	inode_init_owner(root, NULL, S_IFDIR | 0777);
	if (!root)
		goto out;
	root->i_op = &lfs_dir_inode_operations;

	set_nlink(root, 2);
	root_dentry = d_make_root(root);
	if (!root_dentry)
		goto out_iput;

	lfs_create_files(sb, root_dentry);
	sb->s_root = root_dentry;
	return 0;

out_iput:
	iput(root);
out:
	return -ENOMEM;
}

static struct dentry *lfs_get_super(struct file_system_type *fst,
									int flags, const char *devname, void *data){
	printk("Size_t: %ld,Loff_t: %ld",sizeof(size_t),sizeof(loff_t));
	
	int i;
	for(i=0;i<POOL_SIZE;i++){
		init_mutex(i);
		init_socket(i);
	}
	return mount_nodev(fst, flags, data, lfs_fill_super);
}

static struct file_system_type lfs_type = {
	.owner = THIS_MODULE,
	.name = "lwnfs6",
	.mount = lfs_get_super,
	.kill_sb = kill_litter_super,
};

static int __init lfs_init(void){
	return register_filesystem(&lfs_type);
}

static void __exit lfs_exit(void){

	unregister_filesystem(&lfs_type);
	int i;
	for(i=0;i<POOL_SIZE;i++){
		if(socket_arr[i]!=0){
			sock_release(socket_arr[i]);
		}
	}
}

module_init(lfs_init);
module_exit(lfs_exit);