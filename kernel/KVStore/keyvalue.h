#ifndef HASHTABLESTORE_H
#define HASHTABLESTORE_H

#include <linux/rhashtable.h>

/*
 * Keyvalue-related structures and variables.
 */

struct kvstore_object
{
	int key;
	struct rhash_head head;

	char * value;
	int value_len;
};

/*
 * Keyvalue-related functions.
 */

bool kvstore_init(void);
void kvstore_exit(void);

char * kvstore_get(const char * key);

void kvstore_insert(const char * key, const void * value, const int len);
void kvstore_remove(const char * key);

#endif
