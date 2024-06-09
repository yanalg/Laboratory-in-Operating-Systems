#ifndef _RPG_FUNCS_H_
#define _RPG_FUNCS_H_

#include <linux/list.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/slab.h> 
#include <asm-i386/uaccess.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <asm/current.h>
#include <linux/fs.h>







struct rpg_stats{
	int cclass;
	int level;
	int party_size;
	int fighter_levels;
	int mage_levels;
};






/* define functions*/
int has_character(struct task_struct *pros);
int sys_rpg_create_character(int cclass);
int sys_rpg_fight(int type , int level);
int calc_strength(int type, struct task_struct * leader);
int sys_rpg_get_stats(struct rpg_stats* stats);
int get_cclass(struct task_struct* leader,pid_t pid);
int get_player_level(struct task_struct* leader,pid_t pid);
int sys_rpg_join(pid_t player);
void moving_list(struct task_struct* source,struct task_struct* new);
void update_leader(struct task_struct* dest);
void move_my_node(pid_t my_id,struct task_struct* old_leader,struct task_struct* new_leader);
int rpg_fork(struct task_struct* son);
int rpg_exit(struct task_struct* proc);



/* create player struct*/
struct player {
	pid_t player_pid;
	int player_level;
	int cclass;
	struct list_head my_list;
};



#endif

