
/*
 * Module for handling netlink requests on specific BUS.
 *
 * Author: Victor From <c13vfm@cs.umu.se>
 * 
 *
 *
 */


#include "myheader.h"
#include "TLV/tlv.h"
#include "KVStore/keyvalue.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <net/netlink.h>
#include <asm/types.h>
#include <linux/string.h>
#include <linux/skbuff.h>
#include <linux/printk.h> 
#include <linux/kobject.h> 
#include <linux/sysfs.h> 
#include <linux/init.h> 
#include <linux/fs.h> 


/*
* To avoid possible overlaps:
* Remember to check that netlink bus is
* available and not used by other subsystems.
*/

#define INT8_SIZE   1
#define INT16_SIZE  2
#define INT32_SIZE  4

#ifndef NETLINK_USER 
#define NETLINK_USER 31
#endif

#ifndef MAX_PAYLOAD 
#define MAX_PAYLOAD 512
#endif

unsigned char *ptr;
struct sock *nl_sock = NULL;


/* ------------------------------ sysfs----------------------------*/

#ifndef MESS_SIZE_MAX
#define MESS_SIZE_MAX 256
#endif

/*
 * This module shows how to create a simple subdirectory in sysfs called
 * /sys/kernel/kobject-example  In that directory, 3 files are created:
 * "foo", "baz", and "bar".  If an integer is written to these files, it can be
 * later read out of it.
 */

static char kw_info[MESS_SIZE_MAX] = "hello\n";
static int container_size = 6;

static struct kobject *kw_kobj;

/*
 * The "foo" file where a static variable is read from and written to.
 */
static ssize_t foo_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{	
	return scnprintf(buf, PAGE_SIZE, "%s", kw_info);
	//return strncpy(buf, kw_info, container_size);;
}

static ssize_t foo_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	container_size = min( (size_t)MESS_SIZE_MAX, count);
	strncpy(kw_info, buf, container_size);
	sysfs_notify(kw_kobj, NULL, "kw_info");
	return count;
}

/* Sysfs attributes cannot be world-writable. */
static struct kobj_attribute kw_attribute =
	__ATTR(kw_info, 0664, foo_show, foo_store);



/*
 * Create a group of attributes so that we can create and destroy them all
 * at once.
 */
static struct attribute *attrs[] = {
	&kw_attribute.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

/*
 * An unnamed attribute group will put all of the attributes directly in
 * the kobject directory.  If we specify a name, a subdirectory will be
 * created for the attributes with the directory being the name of the
 * attribute group.
 */
static struct attribute_group attr_group = {
	.attrs = attrs,
};


/* ------------------------ netlink functions ----------------------*/
/*
* Send back to user, given sequence number and the pid of process to be reached.
*
*/
int nl_send_msg(u32 rec_pid , int seqNr, char *databuf)
{
	struct sk_buff *skb;
	struct nlmsghdr *nl_hdr;
    unsigned char buffer[MAX_PAYLOAD] = {0}; //buffer used to construct message
	
	int err = 0;                        	 // err
    int pload_length = 0;               	 //length of tlv payload in bytes
    char temp_string[200];
    
    sprintf(temp_string, "%d", seqNr);
    pr_info("%s", temp_string);
	strcpy(kw_info, temp_string);
	sysfs_notify(kw_kobj, NULL, "kw_info");
	
	
    if(rec_pid == 0) {
        pr_err("Dont send to kernel or recieve from kernel!");
        return -1;
    }
       
	/* Allocate a new netlink message */
	skb = nlmsg_new(NLMSG_SPACE(MAX_PAYLOAD), GFP_KERNEL);

	if(!skb){
		pr_err("failed to allocate skb");
		return -1;
	}

    /* create payload and recieve length */

    memset(buffer, 0, sizeof(buffer));
    if(databuf != NULL)
    {
        memcpy(buffer, databuf, MAX_PAYLOAD);
        pload_length = strlen(buffer);
    }
    else
    {
        pload_length = create_tlv_message(1, buffer);
        if(pload_length <= 0) {
            pr_err("No message to send");
            return -1;
        }
    }

    /* Fill the header with data */
	nl_hdr = (struct nlmsghdr*)skb->data;
	nl_hdr->nlmsg_len = NLMSG_LENGTH(pload_length);
	nl_hdr->nlmsg_pid = 0;
	nl_hdr->nlmsg_flags = 0;
	nl_hdr->nlmsg_seq = seqNr;

	/* Add a netlink msg to an sk_bff */
	nl_hdr = nlmsg_put(skb, 0, seqNr, NLMSG_DONE, nl_hdr->nlmsg_len, 0);

	NETLINK_CB(skb).dst_group = 0; /* Unicast */
	NETLINK_CB(skb).portid = 0; /* from kernel */
	
	memcpy(NLMSG_DATA(nl_hdr), buffer, pload_length);

	err =  nlmsg_unicast(nl_sock, skb, rec_pid);
	if(err < 0) {
		pr_err("Failed to send data\n");
		return -1;
	}

	pr_info("Sent message to pid: %d with seq: %d\n", rec_pid, seqNr);
	return 0;
}


/*
* Callback function that 
*
*/
static void nl_recv_callback(struct sk_buff *skb) {
    struct nlmsghdr *nl_hdr;
    u32 pid;
    int seq;
    int buf_len;
    int err;
    unsigned char buffer[MAX_PAYLOAD]; // FIX the size

 
 	/* Receive nlmsghdr to get correct data */
    nl_hdr = nlmsg_hdr(skb); 
    

    nl_hdr=(struct nlmsghdr*)skb->data;
    pid = nl_hdr->nlmsg_pid;
    seq = nl_hdr->nlmsg_seq;

    /* Extract the buffer from payload */
    buf_len = NLMSG_PAYLOAD(nl_hdr, 0);

    memset(buffer, 0 , sizeof(buffer));
    memcpy(buffer, NLMSG_DATA(nl_hdr), buf_len);

    /* Parse the payload */
    err = parse_tlv_message(seq, pid, buffer, buf_len );
    if(err < 0)
        pr_info("nl_send_msg failed");
}


/**
* module init:
*			Set callback function to the one declared in this file.
*			Create a connection
*/
static int __init nlmodule_init(void) {
	int ret;
	struct netlink_kernel_cfg cfg = {
		.groups = 1,
		.input = nl_recv_callback,
	};
	

	/*
	 * Create a simple kobject with the name of "kobject_example",
	 * located under /sys/kernel/
	 *
	 * As this is a simple directory, no uevent will be sent to
	 * userspace.  That is why this function should not be used for
	 * any type of dynamic kobjects, where the name and number are
	 * not known ahead of time.
	 */
	kw_kobj = kobject_create_and_add("kobject_kw", kernel_kobj);
	if (!kw_kobj)
		return -ENOMEM;


	/* Create the files associated with this kobject */
	ret = sysfs_create_group(kw_kobj, &attr_group);
	if (ret ) {
		pr_err("Cant create sysfs_group");
		kobject_put(kw_kobj);
		netlink_kernel_release(nl_sock);
	}
	

	printk("Entering: %s\n", __FUNCTION__);
	nl_sock = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);

	if(!nl_sock) {
		pr_err("Error creating netlink socket");
		return -10;
	}

	if(!kvs_init())
	{
		pr_err("Failed to initialize KVS.");
		return -10;
	}

	pr_info("Successfully created netlink socket.");
	return ret;
}

/*
* exit module:
* 		Release socket.
*		close kw_store 
*		and decrement kw_obj counter.
*/
static void __exit nlmodule_exit(void) {
	netlink_kernel_release(nl_sock);
	kvs_exit();
	kobject_put(kw_kobj);
	pr_info("exiting the nl module");
}

module_init(nlmodule_init);
module_exit(nlmodule_exit);

MODULE_LICENSE("GPL");
