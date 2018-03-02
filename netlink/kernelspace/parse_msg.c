#include "myheader.h"
#include <linux/kernel.h>

int do_something( unsigned int receivning_pid, int option) {
	pr_info("In do_something, calling nl_send_msg\n");	
	return nl_send_msg(receivning_pid, option);
}

