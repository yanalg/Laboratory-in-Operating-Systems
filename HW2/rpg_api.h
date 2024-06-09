#include <stdio.h>
#include <errno.h>
#include <sys/types.h>



/* create rpg_stat srtuct */
struct rpg_stats {
	int cclass;
	int level;
	int party_size;
	int fighter_levels;
	int mage_levels;
};


/*rpg_create wrapper function*/
int rpg_create_character(int cclass){
	int res;
	__asm__
	(
		"pushl %%eax;"
		"pushl %%ebx;"
		"movl $243, %%eax;"
		"movl %1, %%ebx;"
		"int $0x80;"
		"movl %%eax,%0;"
		"popl %%ebx;"
		"popl %%eax;"
		: "=m" (res)
		: "m" (cclass)
	);
	if (res < 0)
	{
		errno = -res;
		res = -1;
	}
	return res;
}


/*rpg_fight wrapper function*/
int rpg_fight(int type , int level){
	int res;
	__asm__
	(
		"pushl %%eax;"
		"pushl %%ebx;"
		"pushl %%ecx;"
		"movl $244, %%eax;"
		"movl %1, %%ebx;"
		"movl %2, %%ecx;"
		"int $0x80;"
		"movl %%eax,%0;"
		"popl %%ecx;"
		"popl %%ebx;"
		"popl %%eax;"
		: "=m" (res)
		: "m" (type), "m" (level)
	);
	if (res < 0)
	{
		errno = -res;
		res = -1;
	}
	return res;
	
}

/*rpg_stat wrapper function*/
int rpg_get_stats(struct rpg_stats* stats){
	int res;
	__asm__
	(
		"pushl %%eax;"
		"pushl %%ebx;"
		"movl $245, %%eax;"
		"movl %1, %%ebx;"
		"int $0x80;"
		"movl %%eax,%0;"
		"popl %%ebx;"
		"popl %%eax;"
		: "=m" (res)
		: "m" (stats)
	);
	if (res < 0)
	{
		errno = -res;
		res = -1;
	}
	return res;
	
}

/*rpg_join wrapper function*/
int rpg_join(pid_t player){
	int res;
	__asm__
	(
		"pushl %%eax;"
		"pushl %%ebx;"
		"movl $246, %%eax;"
		"movl %1, %%ebx;"
		"int $0x80;"
		"movl %%eax,%0;"
		"popl %%ebx;"
		"popl %%eax;"
		: "=m" (res)
		: "m" (player)
	);
	if (res < 0)
	{
		errno = -res;
		res = -1;
	}
	return res;	
}













