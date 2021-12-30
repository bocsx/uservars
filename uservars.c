
/*
 *  uservars.c kernel module
 *
 *  Provide a simple interface for linux programs/scripts to use (create/read/write/delete) /proc kernel variables.
 *
 *  Author: Csaba Bodnar <bodnar.csabi@gmail.com>
 *
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include<linux/sched.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/string.h>

// Please include this from your kernel source as they moved proc_dir_entry from proc_fs.h to here :(
#include "/usr/src/linux-source-4.15.0/fs/proc/internal.h"

#define VAR_NAME_SIZE (512)
#define VAR_VALUE_SIZE (512)

struct var {
	unsigned long inode;
	char name[VAR_NAME_SIZE];
	char value[VAR_VALUE_SIZE];
	struct proc_dir_entry *pde;
	struct list_head list;
};

LIST_HEAD(var_list);

struct proc_dir_entry *uservars_dir;
struct proc_dir_entry *system_dir;
struct proc_dir_entry *createdir_file;


#define ourmin(a,b) (((a)<(b)) ? (a) : (b))

void print_proc_dir_entry(struct proc_dir_entry *pde)
{

	printk(KERN_WARNING "pde->low_ino: %u", pde->low_ino);
	printk(KERN_WARNING "pde->data: %s", (char *) pde->data);
	printk(KERN_WARNING "pde->name: %s", pde->name);

}

void print_var(struct var *v)
{
	printk(KERN_WARNING "inode: %lu\tname: %s\tvalue: %s", v->inode, v->name, v->value);
}

unsigned long strip_name(char *name, unsigned long len)
{
	char *c;

	if (name[0] == '\0')
		return 0;
	while (name[len-1] == '/' || name[len-1] == ' ') {
		name[len-1]='\0';
		len--;
	}
	while (name[0] == '/' || name[0] == ' ') {
		//shift left
		c=name;
		while(*c != '\0') {
			//*c++=*c;
			c[0]=c[1];
			c++;
		}
		len--;
	}
	return len;
}

char *base_name( char *name, unsigned long len)
{
	char *c;
	c=name + len - 1;

	while (*c != '/' && name <= c)
		c--;
	return ++c;
}

void dir_name( char *name, unsigned long len)
{
	char *c;
/*
	c=name + len - 1;

	while (*c != '/' && name <= c)
		c--;
	c++;
	*c='\0';
*/
	c=strrchr(name, '/');
	if (c == NULL)
		*name='\0';
	else
		*c='\0';
}

struct var *find_by_inode(unsigned long inode)
{
	struct var *v;

	list_for_each_entry(v, &var_list, list) {
		if (v->inode == inode) {
			return v;
		}
	}
	return NULL;
}

struct var *find_by_name(char *name)
{
	struct var *v;

	list_for_each_entry(v, &var_list, list) {
		if (!strcmp(v->name, name)) {
			return v;
		}
	}
	return NULL;
}

ssize_t uv_proc_write(struct file *filp,const char *buf,size_t count,loff_t *offp)
{
	struct var *variable;

	unsigned long actual_len = ourmin(count, VAR_VALUE_SIZE-1);

	//printk(KERN_WARNING "filp->f_inode->i_ino: %lu", filp->f_inode->i_ino);

	variable=find_by_inode(filp->f_inode->i_ino);
	if (variable) {
		copy_from_user(variable->value, buf, actual_len);
		variable->value[actual_len]='\0';
	}

	return count;
}


static int uv_proc_show(struct seq_file *m, void *v)
{
	struct var *variable;

	variable=find_by_inode(m->file->f_inode->i_ino);
	if (variable) {
		seq_printf(m, variable->value);
		return 0;
	}

	return 0;
}

static int uv_proc_open(struct inode *inode, struct  file *file)
{
	printk(KERN_WARNING "uv_proc_open - inode: %lu", inode->i_ino);
	printk(KERN_WARNING "uv_proc_open - file->f_inode->i_ino: %lu", file->f_inode->i_ino);

	return single_open(file, uv_proc_show, NULL);
}

static const struct file_operations proc_fops = {
	.owner = THIS_MODULE,
	.write = uv_proc_write,
	.open = uv_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

ssize_t uv_create_var_entry(struct file *filp,const char *buf,size_t count, int is_dir)
{
	struct var *variable, *parent;
	struct proc_dir_entry *parent_dir;
	unsigned long actual_len = ourmin(count, VAR_NAME_SIZE-1);
	char *bname;
	char dname[ VAR_NAME_SIZE ];

	variable = kmalloc(sizeof(struct var), GFP_KERNEL);
	
	copy_from_user(variable->name, buf, actual_len);
	variable->name[actual_len]='\0';
	actual_len=strip_name(variable->name, actual_len);
	bname=base_name(variable->name, actual_len);

	strcpy(dname, variable->name);
	dir_name(dname, actual_len);
	printk(KERN_WARNING "variable->name: >>%s<< bname: >>%s<<, dname: >>%s<<",variable->name, bname, dname);
	if (!strcmp(dname, ""))
		parent_dir=uservars_dir;
	else {
		parent=find_by_name(dname);
		if (!parent) {
			printk(KERN_WARNING "No parent directory %s", dname);
			kfree( variable );
			return -ENOMEM;
		}
		parent_dir=parent->pde;
	}

	if (is_dir) {
		variable->pde = proc_mkdir(bname, parent_dir);
		if(!variable->pde) {
			printk(KERN_WARNING "Error creating proc entry %s", variable->name);
			kfree( variable );
			return -ENOMEM;
		}
	} else {
		variable->pde=proc_create(bname, 0666, parent_dir, &proc_fops);
		if(!variable->pde) {
			printk(KERN_WARNING "Error creating proc entry %s", variable->name);
			kfree( variable );
			return -ENOMEM;
		}
	}

	variable->value[0] = '\0';
	variable->inode=variable->pde->low_ino;

	INIT_LIST_HEAD(&variable->list);
	list_add_tail( &variable->list, &var_list );

	return count;
}

ssize_t uv_create_dir_write(struct file *filp,const char *buf,size_t count,loff_t *offp)
{
	return uv_create_var_entry(filp, buf, count, true);
}

ssize_t uv_create_var_write(struct file *filp,const char *buf,size_t count,loff_t *offp)
{
	return uv_create_var_entry(filp, buf, count, false);
}

static int uv_list_var_show(struct seq_file *m, void *v)
{
	struct var *variable;

	list_for_each_entry( variable, &var_list, list ) {
		seq_printf(m, "%lu %s: %s\n", variable->inode, variable->name, variable->value);
		// print_proc_dir_entry( variable->pde );
	}

	return 0;
}

static int uv_list_var_open(struct inode *inode, struct  file *file)
{
	return single_open(file, uv_list_var_show, NULL);
}

static const struct file_operations create_dir_fops = {
	.owner = THIS_MODULE,
	.write = uv_create_dir_write,
};

static const struct file_operations create_var_fops = {
	.owner = THIS_MODULE,
	.write = uv_create_var_write,
};

static const struct file_operations list_var_fops = {
	.owner = THIS_MODULE,
	.open = uv_list_var_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int proc_init (void)
{

	uservars_dir = proc_mkdir("uservars", NULL);
	if(!uservars_dir) {
		printk(KERN_INFO "Error creating proc entry");
		return -ENOMEM;
	}
	print_proc_dir_entry(uservars_dir);
	system_dir = proc_mkdir("system", uservars_dir);
	if(!system_dir) {
		printk(KERN_INFO "Error creating proc entry");
		return -ENOMEM;
	}
	print_proc_dir_entry(system_dir);

	// Should check the output of this too
	createdir_file = proc_create("create_dir", 0644, system_dir, &create_dir_fops);
	printk(KERN_WARNING "proc_init - low_ino: %u", createdir_file->low_ino);
	print_proc_dir_entry(createdir_file);
	proc_create("delete_dir", 0644, system_dir, &proc_fops);
	proc_create("create_var", 0644, system_dir, &create_var_fops);
	proc_create("delete_var", 0644, system_dir, &proc_fops);
	proc_create("list_var", 0644, system_dir, &list_var_fops);

	return 0;
}

void proc_cleanup(void)
{

	remove_proc_entry("create_dir", system_dir);
	remove_proc_entry("delete_dir", system_dir);
	remove_proc_entry("create_var", system_dir);
	remove_proc_entry("delete_var", system_dir);
	proc_remove(system_dir);
	proc_remove(uservars_dir);
}


MODULE_LICENSE("GPL"); 
module_init(proc_init);
module_exit(proc_cleanup);
