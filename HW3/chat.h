#ifndef _CHAT_H_
#define _CHAT_H_

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/slab.h>


#define MY_MAGIC 'r'
#define COUNT_UNREAD _IO(MY_MAGIC, 0)

#define MAX_MESSAGE_LENGTH 100

#define SEEK_SET 0

//
// Function prototypes
//
int my_open(struct inode *inode, struct file *filp);

int my_release(struct inode *inode, struct file *filp);

ssize_t my_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);

ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

loff_t my_llseek(struct file *, loff_t, int);

time_t gettime();

pid_t getpid();

struct chat_room *find_chat_room(int room_index);

struct chat_room *create_chat_room(int room_index);


struct message_t {
    pid_t pid;
    time_t timestamp;
    char message[MAX_MESSAGE_LENGTH];
};


struct chat_room {
    int room_index;
    struct list_head messages; //linked list to hold all the messages
    struct list_head room_list; // linked list for chat rooms
    int num_messages;
    int members_count;
};

struct message_node{
    struct message_t message;
    struct list_head list;
};

struct chat_system {
    struct list_head room_list; // Linked list to hold all chat rooms
};



#endif
