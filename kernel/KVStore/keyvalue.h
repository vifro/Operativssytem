#ifndef KEYVALUE_H
#define KEYVALUE_H

#include <linux/rhashtable.h>

/*
 * Keyvalue-related structures and variables.
 */

struct kvs_object
{
	int key;
	struct rhash_head node;

	char * value;
	int value_len;
};

/*
 * Keyvalue-related functions.
 */

bool kvs_init(void);
void kvs_exit(void);

char * kvs_get(const char * key);
void kvs_get_storage_info(char *temp_string);

int kvs_insert(const char * key, const void * value, const int len);
void kvs_remove(const char * key);

#endif
