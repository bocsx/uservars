
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
	char *value;
	struct proc_dir_entry *pde;
	struct list_head list;
};

LIST_HEAD(var_list);
DEFINE_MUTEX(var_mutex);

struct proc_dir_entry *uservars_pde;
struct proc_dir_entry *command_pde;
struct proc_dir_entry *createdir_pde;
struct proc_dir_entry *createvar_pde;
struct proc_dir_entry *delete_pde;
struct proc_dir_entry *list_pde;

#define ourmin(a,b) (((a)<(b)) ? (a) : (b))

void uv_print_proc_dir_entry(struct proc_dir_entry *pde)
{

	printk(KERN_WARNING "pde->low_ino: %u", pde->low_ino);
	printk(KERN_WARNING "pde->data: %s", (char *) pde->data);
	printk(KERN_WARNING "pde->name: %s", pde->name);

}

//void print_var(struct var *v)
//{
//	printk(KERN_WARNING "inode: %lu\tname: %s\tvalue: %s", v->inode, v->name, v->value);
//}

unsigned long uv_strip_name(char *name, unsigned long len)
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

char *uv_base_name( char *name, unsigned long len)
{
	char *c;
	c=name + len - 1;

	while (*c != '/' && name <= c)
		c--;
	return ++c;
}

void uv_dir_name( char *name, unsigned long len)
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

struct var *uv_find_by_inode(unsigned long inode)
{
	struct var *v;

	mutex_lock(&var_mutex);
	list_for_each_entry(v, &var_list, list) {
		if (v->inode == inode) {
			mutex_unlock(&var_mutex);
			return v;
		}
	}
	mutex_unlock(&var_mutex);

	return NULL;
}

struct var *uv_find_by_name(char *name)
{
	struct var *v;

	mutex_lock(&var_mutex);
	list_for_each_entry(v, &var_list, list) {
		if (!strcmp(v->name, name)) {
			mutex_unlock(&var_mutex);
			return v;
		}
	}
	mutex_unlock(&var_mutex);

	return NULL;
}

ssize_t uv_proc_write(struct file *filp,const char *buf,size_t count,loff_t *offp)
{
	struct var *variable;

	//printk(KERN_WARNING "filp->f_inode->i_ino: %lu", filp->f_inode->i_ino);

	variable=uv_find_by_inode(filp->f_inode->i_ino);
	if (variable) {
		mutex_lock(&var_mutex);
		if(variable->value != NULL)
			kfree(variable->value);
		variable->value = kmalloc(count+1, GFP_KERNEL);
		copy_from_user(variable->value, buf, count);
		variable->value[count]='\0';
		mutex_unlock(&var_mutex);
	}

	return count;
}


static int uv_proc_show(struct seq_file *m, void *v)
{
	struct var *variable;

	variable=uv_find_by_inode(m->file->f_inode->i_ino);
	if (variable) {
		mutex_lock(&var_mutex);
		seq_printf(m, variable->value);
		mutex_unlock(&var_mutex);
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
	actual_len=uv_strip_name(variable->name, actual_len);
	bname=uv_base_name(variable->name, actual_len);

	strcpy(dname, variable->name);
	uv_dir_name(dname, actual_len);
	printk(KERN_WARNING "uv_create_var_entry - variable->name: >>%s<< bname: >>%s<<, dname: >>%s<<",variable->name, bname, dname);
	if (!strcmp(dname, ""))
		parent_dir=uservars_pde;
	else {
		parent=uv_find_by_name(dname);
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

	variable->value = NULL;
	variable->inode=variable->pde->low_ino;

	INIT_LIST_HEAD(&variable->list);
	mutex_lock(&var_mutex);
	list_add_tail( &variable->list, &var_list );
	mutex_unlock(&var_mutex);

	return count;
}

ssize_t uv_delete_write(struct file *filp,const char *buf,size_t count, loff_t *offp)
{
	char name[ VAR_NAME_SIZE ];
	struct var *variable;
	unsigned long actual_len = ourmin(count, VAR_NAME_SIZE-1);

	copy_from_user(name, buf, actual_len);
	name[actual_len]='\0';
	actual_len=uv_strip_name(name, actual_len);

	variable=uv_find_by_name(name);
	if (!variable) {
		printk(KERN_WARNING "%s doesn't exist, cannot delete", name);
		return -ENOMEM;
	}

	printk(KERN_WARNING "uv_delete_write - name: >>%s<< variable->name: >>%s<<, variable->inode: >>%lu<<", name, variable->name, variable->inode);

	proc_remove(variable->pde);
	mutex_lock(&var_mutex);
	list_del(&variable->list);
	mutex_unlock(&var_mutex);
	if(variable->value != NULL)
		kfree( variable->value );
	kfree( variable );

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

static int uv_list_show(struct seq_file *m, void *v)
{
	struct var *variable;

	mutex_lock(&var_mutex);
	list_for_each_entry( variable, &var_list, list ) {
		if(variable->value != NULL)
			seq_printf(m, "%lu %s(%li): %s\n", variable->inode, variable->name, strlen(variable->value), variable->value);
		else
			seq_printf(m, "%lu %s:\n", variable->inode, variable->name);
		// uv_print_proc_dir_entry( variable->pde );
	}
	mutex_unlock(&var_mutex);

	return 0;
}

static int uv_list_open(struct inode *inode, struct  file *file)
{
	return single_open(file, uv_list_show, NULL);
}

static const struct file_operations create_dir_fops = {
	.owner = THIS_MODULE,
	.write = uv_create_dir_write,
};

static const struct file_operations create_var_fops = {
	.owner = THIS_MODULE,
	.write = uv_create_var_write,
};

static const struct file_operations delete_fops = {
	.owner = THIS_MODULE,
	.write = uv_delete_write,
};

static const struct file_operations list_fops = {
	.owner = THIS_MODULE,
	.open = uv_list_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int proc_init (void)
{
	char *name;

	mutex_init(&var_mutex);

	name="uservars";
	uservars_pde=proc_mkdir(name, NULL);
	if(!uservars_pde) {
		printk(KERN_WARNING "Error creating proc entry %s", name);
		return -ENOMEM;
	}
	name="command";
	command_pde=proc_mkdir(name, uservars_pde);
	if(!command_pde) {
		printk(KERN_WARNING "Error creating proc entry %s", name);
		return -ENOMEM;
	}
	name="create_dir";
	createdir_pde=proc_create(name, 0666, command_pde, &create_dir_fops);
	if(!createdir_pde) {
		printk(KERN_WARNING "Error creating proc entry %s", name);
		return -ENOMEM;
	}
	name="create_var";
	createvar_pde=proc_create(name, 0666, command_pde, &create_var_fops);
	if(!createvar_pde) {
		printk(KERN_WARNING "Error creating proc entry %s", name);
		return -ENOMEM;
	}
	name="delete";
	delete_pde=proc_create(name, 0666, command_pde, &delete_fops);
	if(!delete_pde) {
		printk(KERN_WARNING "Error creating proc entry %s", name);
		return -ENOMEM;
	}
	name="list";
	list_pde=proc_create(name, 0666, command_pde, &list_fops);
	if(!list_pde) {
		printk(KERN_WARNING "Error creating proc entry %s", name);
		return -ENOMEM;
	}
	//printk(KERN_WARNING "proc_init - low_ino: %u", createdir_pde->low_ino);
	//uv_print_proc_dir_entry(createdir_pde);
	
	return 0;
}

void proc_cleanup(void)
{
	struct var *v, *next;

	mutex_lock(&var_mutex);
	list_for_each_entry_safe_reverse(v, next, &var_list, list) {
		proc_remove(v->pde);
		list_del(&v->list);
		if(v->value != NULL)
			kfree(v->value);
		kfree( v );

	}
	mutex_unlock(&var_mutex);

	proc_remove(createdir_pde);
	proc_remove(createvar_pde);
	proc_remove(delete_pde);
	proc_remove(list_pde);
	proc_remove(command_pde);
	proc_remove(uservars_pde);
}


MODULE_LICENSE("GPL"); 
module_init(proc_init);
module_exit(proc_cleanup);
