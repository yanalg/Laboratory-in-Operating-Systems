/* chat.c: Example char device module.
 *
 */
/* Kernel Programming */
#define MODULE
#define LINUX
#define __KERNEL__

#include <linux/kernel.h>  	
#include <linux/module.h>
#include <linux/time.h>
#include <linux/fs.h>       		
#include <asm/uaccess.h>
#include <linux/errno.h>  
#include <asm/segment.h>
#include <asm/current.h>

#include "chat.h"

#define MY_DEVICE "chat"
#define MAX_ROOMS_NUM 256 //MINOR is [0,255]
#define T_MESSAGE_SIZE sizeof(struct message_t)

MODULE_AUTHOR("Anonymous");
MODULE_LICENSE("GPL");

/* globals */
int my_major = 0; /* will hold the major # of my device driver */
struct timeval tv; /* Used to get the current time */


static struct chat_system chat_system;

struct file_operations my_fops = {
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
    .ioctl = my_ioctl,
    .llseek = my_llseek
};

int init_module(void)
{
    // This function is called when inserting the module using insmod

    my_major = register_chrdev(my_major, MY_DEVICE, &my_fops);
    int i;

    if (my_major < 0)
    {
	    //printk(KERN_WARNING "can't get dynamic major\n");
	    return my_major;
    }

    INIT_LIST_HEAD(&chat_system.room_list);

    // Create multiple chat rooms with unique minor numbers
    for (i = 0; i < MAX_ROOMS_NUM; ++i){
        struct chat_room *room = create_chat_room(i);
        if (!room) {
            //printk(KERN_WARNING "Failed to create chat room for minor number %d\n", i);
            return -EFAULT; // Error allocating memory for chat room
        }
    }
    //
    // do_init();
    //
    //memset(rooms_list, 0, sizeof(rooms_list));
    //printk(KERN_INFO "Device initialized successfully\n");

    return 0;
}


void cleanup_module(void)
{
    // This function is called when removing the module using rmmod
    struct list_head *pos, *tmp;
    struct chat_room *room;
    list_for_each_safe(pos, tmp, &chat_system.room_list) {
        room = list_entry(pos, struct chat_room, room_list); 
        // Remove room from the list
        list_del(pos);
        // Free room structure
        kfree(room);
    } 

    unregister_chrdev(my_major, MY_DEVICE);

    //
    // do clean_up();
    //
    return;
}


int my_open(struct inode *inode, struct file *filp)
{
    // handle open
    //getting the room number
    if (!inode) {
        //printk(KERN_ERR "Invalid inode pointer\n");
        return -EFAULT; // Return an error code indicating invalid argument
    }

    //printk(KERN_INFO "IN MY OPEN\n");
    unsigned int room_index = MINOR(inode->i_rdev);
    //check if the room already exists
    struct chat_room *new_room = find_chat_room(room_index);
    if(!new_room){
        //printk(KERN_ERR "Chat room not found for minor number %u\n", room_index);
        return -EFAULT;
    }else{
        // Room already exists, increment member count
        new_room->members_count++;
    }
    // Store the chat room pointer in filp->private_data
    filp->private_data = new_room;
    // Initialize read position for this process
    filp->f_pos = 0;
    return 0;
    
}


int my_release(struct inode *inode, struct file *filp)
{
    // handle file closing
    struct chat_room *room = filp->private_data;
    if(room->members_count > 1 ){
        room->members_count--;
    }else{
        if(room->num_messages){
            // Free the message nodes
            struct list_head *pos, *q;
            list_for_each_safe(pos, q, &room->messages) {
                struct message_node *msg_node = list_entry(pos, struct message_node, list);
                list_del(pos);
                kfree(msg_node);
            }
        }
        room->members_count = 0;
        room->num_messages = 0;      
    }
    filp->private_data = NULL;
    return 0;
}

ssize_t my_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    //
    // Do read operation.
    // Return number of bytes read.
    //printk(KERN_INFO "IN MY READ\n");
    //check arguments
    if(!buf){
        return -EFAULT;
    }
    // Retrieve the chat room pointer from filp->private_data
    struct chat_room *room = filp->private_data; //see if we need to check argument *room
    // Check if the chat room pointer is valid
    if (!room){
        return -EFAULT;
    }
    struct message_node *msg_node;
    ssize_t bytes_written = 0;
    int num_messages_requseted = (count/T_MESSAGE_SIZE);
    //get my position in the list
    loff_t pos = *f_pos;
    loff_t pos_in_messages = ((int)pos/T_MESSAGE_SIZE);
    //printk(KERN_INFO "MY READING POS: %d\n", pos_in_messages);
    // Check if the message list is empty 
    if (list_empty(&room->messages)) {
        //printk(KERN_INFO "MY LIST IS EMPTY\n");
        return 0; // zero bytes read
    } 
   
        
    // loop over message list to find the starting position
    struct list_head *pos_node, *tmp;
    list_for_each_safe(pos_node, tmp, &room->messages){
        //printk(KERN_INFO "Inside the loop\n");
        if (pos_in_messages == 0) {
            msg_node = list_entry(pos_node, struct message_node, list);
            //printk(KERN_INFO "I am Writing message: %s\n", msg_node->message.message);
            //printk(KERN_INFO "GONNA COPY TO USER\n");
            if (copy_to_user(buf + bytes_written, &msg_node->message, sizeof(struct message_t))) {
                //update the file position
                *f_pos += bytes_written;
                //printk(KERN_INFO "COPY TO USER FAILED\n");
                return -EBADF; // Copy failed
            }
            bytes_written += T_MESSAGE_SIZE;
            num_messages_requseted--;
            if(num_messages_requseted == 0){
                break;
            }

        }else{
            pos_in_messages--;
        }
    }
    //update the file position
    *f_pos += bytes_written;
    // Return number of bytes read
    //printk(KERN_INFO "READING DONE\n");
    return bytes_written;

}

ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos){
    //printk(KERN_INFO "Entered my_write\n");
    // Retrieve the chat room pointer from filp->private_data
    struct chat_room *room = (struct chat_room *)filp->private_data;
    struct message_node *new_msg_node;
    ssize_t bytes_written = 0;
    char *kernel_buf;

    // Checking arguments
    if (!room){
        return -EFAULT;
    }
    if(!buf){
        return -EFAULT;
    }
    int msg_len = strnlen_user(buf,count) - 1;
    //printk(KERN_INFO "msg len is %d\n", msg_len);
    if (msg_len < 0 || msg_len > MAX_MESSAGE_LENGTH) {
        return -ENOSPC;
    }
    // Allocate kernel buffer for message
    kernel_buf = kmalloc(msg_len, GFP_KERNEL);
    if(!kernel_buf){
        return -EFAULT;
    }
    // Copy message from user space to kernel buffer
    int ret = copy_from_user(kernel_buf, buf, msg_len);
    if (ret) {
        kfree(kernel_buf);
        return -EBADF; // Error copying message from user space
    }
    //printk(KERN_INFO " gonna adding msg  %s\n", kernel_buf);
    
    // Create a new message node
    new_msg_node = kmalloc(sizeof(struct message_node), GFP_KERNEL);
    if (!new_msg_node) {
        kfree(kernel_buf);
        return -EFAULT; // Error allocating memory for new message node
    }
    new_msg_node->message.pid = getpid();
    new_msg_node->message.timestamp = gettime();
    memcpy(new_msg_node->message.message, kernel_buf, msg_len);
    //printk(KERN_INFO "added msg  %s\n", new_msg_node->message.message);
    if (msg_len < MAX_MESSAGE_LENGTH) {
        new_msg_node->message.message[msg_len] = '\0'; // NULL terminate the string
    }

    // Add the new message node to the end of the message list
    list_add_tail(&new_msg_node->list, &room->messages);
    room->num_messages++;
    // Update the number of bytes written
    bytes_written = count;

    // Free kernel buffer for message
    kfree(kernel_buf);

    return bytes_written;
}

int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    //printk(KERN_INFO "Entered my_ioctl\n");
    if (!filp->private_data) {
        //printk(KERN_ERR "my_ioctl: filp->private_data is null\n");
        return -EINVAL; // Return an error code for null pointer
    }   
    struct chat_room *room = filp->private_data;
    int unread_count = 0;
    loff_t pos = filp->f_pos;
    switch(cmd)
    {
        case COUNT_UNREAD:
            //
            // handle 
            //
            // Check if the message list is empty
            if (!room->num_messages) {
                //printk(KERN_INFO "my_ioctl: No messages to read\n");
                return 0; //no messages to be read
            }
            //find position in the list.
            //struct list_head *pos_node = &room->messages;
            int pos_in_messages = ((int)pos/ T_MESSAGE_SIZE);
            unread_count = room->num_messages - pos_in_messages;
            if(unread_count < 0){
                //printk(KERN_INFO "my_ioctl: Unread_count is negative\n");
                
            }
            
            //printk(KERN_INFO "my_ioctl: Unread message count: %d\n", unread_count);
            return unread_count;
            break;
        default:
            //printk(KERN_ERR "my_ioctl: Unsupported command: %u\n", cmd);
            return -ENOTTY;
    }
    //printk(KERN_ERR "my_ioctl: Unexpected code path\n");
    return -EINVAL;

}

loff_t my_llseek(struct file *filp, loff_t offset, int whence)
{
    struct chat_room *room = filp->private_data;
    if (!room){
        return -EINVAL;
    }
    loff_t new_pos;
    int offset_in_messages;
    //
    // Change f_pos field in filp according to offset and whence.
    //
    // Calculate the new position based on the current position and the offset
    switch (whence) {
        case SEEK_SET:
            
            offset_in_messages = (((int)offset)/T_MESSAGE_SIZE);
            if(offset_in_messages < 0){
                return -EINVAL;
            }
            //check in offset is in limits
            if(offset_in_messages > room->num_messages){
                //the new position should be the last node
                new_pos = (room->num_messages)*T_MESSAGE_SIZE;
            }else{
                new_pos = offset;
            }
            filp->f_pos = new_pos;
            return  filp->f_pos;
            
            break;
        default:
            return -EINVAL; // Invalid argument
    }
    return -EINVAL;
}

time_t gettime() {
    do_gettimeofday(&tv);
    return tv.tv_sec;
}

pid_t getpid() {
    return current->pid;
}

struct chat_room *create_chat_room(int room_index) {
    struct chat_room *room = kmalloc(sizeof(struct chat_room), GFP_KERNEL);
    if (!room) {
        return NULL; // Memory allocation failed
    }
    room->room_index = room_index;
    INIT_LIST_HEAD(&room->messages);
    room->num_messages = 0;
    room->members_count = 0;
    // Add the new room to the list
    //INIT_LIST_HEAD(&room->room_list);
    list_add_tail(&room->room_list, &chat_system.room_list);

    return room;
}

struct chat_room *find_chat_room(int room_index) {
    struct list_head *pos, *tmp;
    struct chat_room *room;
    //printk(KERN_INFO "find_chat_room: searching for room %d\n",room_index);
    list_for_each_safe(pos, tmp, &chat_system.room_list) {
        room = list_entry(pos, struct chat_room, room_list);
        //printk(KERN_INFO "find_chat_room: checking room with index %d\n",room->room_index);
        if (room->room_index == room_index) {
            return room;
        }
    }
    return NULL; // Room not found
}





