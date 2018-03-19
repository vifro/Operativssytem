#include <linux/string.h>
#include <linux/slab.h>

#include "keyvalue.h"

struct rhashtable kvs;
struct rhashtable_params kvs_params =
{
	.head_offset = offsetof(struct kvs_object, node),
	.key_offset = offsetof(struct kvs_object, hash),
	.key_len = FIELD_SIZEOF(struct kvs_object, hash),
};

/*
 * kvs_init()
 * Initializes the keyvalue hashtable.
 */
bool kvs_init(void)
{
	int initRes = rhashtable_init(&kvs, &kvs_params);

	if(initRes == -EINVAL || initRes == -ENOMEM)
	{
		/* If we fail somehow, don't make things worse */
		return false;
	}

	/* If we succeed, return to the module function. */
	return true;
}

/*
 * kvs_exit()
 * Cleanup function, deallocates hashtable-related stuff.
 */
void kvs_exit(void)
{
	struct rhashtable_iter it;
	struct kvs_object * obj;

	/* Walk through the hashtable and get rid of old elements. */

	rhashtable_walk_enter(&kvs, &it);

	obj = ERR_PTR(rhashtable_walk_start(&it));
	if(!IS_ERR(obj))
	{
		while((obj = rhashtable_walk_next(&it)) && !IS_ERR(obj))
		{
			pr_info("[kvs_exit] - removing element with hash %d and value %s... (%d bytes)", obj->hash, obj->value, obj->value_len);
			rhashtable_remove_fast(&kvs, &obj->node, kvs_params);

			kfree(obj->value);
			kfree(obj);
		}

		rhashtable_walk_stop(&it);
	}

	rhashtable_walk_exit(&it);
	rhashtable_destroy(&kvs);
}

/*
 * kvs_get()
 * Retrieves the data block for an element with a given key.
 *
 * Returns NULL if the given key can't be found.
 */
char * kvs_get(const char * key)
{


	return NULL;
}

/*
 * kvs_insert()
 * Inserts a given value with a given key of a given length.
 */
void kvs_insert(const char * key, const void * value, const int value_len)
{
	struct kvs_object * obj;

	/* Get the container object ready. */

	obj = kzalloc(sizeof(struct kvs_object), GFP_KERNEL);
	if(!obj)
	{
		pr_warn("[kvs_insert] - unable to allocate KVS object!");
		return;
	}

	obj->value = kzalloc(value_len, GFP_KERNEL);
	if(obj->value)
	{
		memcpy(obj->value, value, value_len);
		obj->value_len = value_len;
	}
	else
	{
		pr_warn("[kvs_insert] - unable to allocate value buffer!");
	}

	rhashtable_insert_fast(&kvs, &obj->node, kvs_params);
	pr_info("[kvs_insert] - inserting object with hash %d...", obj->hash);
}

/*
 * kvs_remove()
 * Removes a value with a given key.
 *
 * Does nothing if the key does not have a corresponding value.
 */
void kvs_remove(const char * key)
{
	
}

/*
 * kvs_hash()
 * Internal hash function, do not use.
 */
u32 kvs_hash(const void * data, u32 len, u32 seed)
{
	const struct kvs_object * obj = data;

	return jhash(obj->key, strlen((char *)obj->key), seed);
}
