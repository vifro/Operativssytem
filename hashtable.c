#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hashtable.h>


#define DEFINE_HASHTABLE(name, bits)				\
		struct hlist_head name[1 << (bits)] =		\
					{ [0 ... ((1 << (bits)) - 1)] = HLIST_HEAD_INIT } 

static struct mystruct {
	int data;
	struct hlist_node my_hash_list;
}


static void addElement(char* name, int data) {
	
	struct mystruct name = {
		.data = data
		.my_hash_list = 0 /* Initialized when added to hashtable */
	}

	hash_add(a, &name.next, name.data);
	int bkt;
	hash_for_each(a, bkt, current, next)  

	return 0;
}


static int myHTable_init(void) {
	pr_info("Module is loaded\n");
	DEFINE_HASHTABLE(a,3);
	addElement( "first", 10);
	
	return 0;
}



static void myHTable_exit(void){
	pr_info("exiting the module");
}

module_init(myHTable_init);
module_exit(myHTable_exit);

MODULE_LICENSE("GPL"); 
