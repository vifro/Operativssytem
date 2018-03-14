#include <linux/string.h>
#include <linux/slab.h>

#include "keyvalue.h"

struct rhashtable kvstore;
struct rhashtable_params kvstore_params;

/*
 * kvstore_init()
 * Initializes the keyvalue hashtable.
 */
bool kvstore_init(void)
{
	int initRes;

	/* Clear the structure before doing stuff, just in case. */
	memset(&kvstore_params, 0, sizeof(struct rhashtable_params));

	/* key_len = Key length in bytes
	 * key_offset = Key structure offset. */
	kvstore_params.key_len = sizeof(int);
	kvstore_params.key_offset = offsetof(struct kvstore_object, key);

	/* head_offset = Head node structure offset. */
	kvstore_params.head_offset = offsetof(struct kvstore_object, head);

	/* Now initialize the hashtable structure once and for all. */
	initRes = rhashtable_init(&kvstore, &kvstore_params);

	if(initRes == -EINVAL || initRes == -ENOMEM)
	{
		/* If we fail somehow, don't make things worse */
		return false;
	}

	/* If we succeed, return to the module function. */
	return true;
}

/*
 * kvstore_exit()
 * Cleanup function, deallocates hashtable-related stuff.
 */
void kvstore_exit(void)
{
	struct rhashtable_iter iter;
	struct kvstore_object * obj;

	/* Walk through the hashtable and get rid of old elements. */

	rhashtable_walk_enter(&kvstore, &iter);

	obj = ERR_PTR(rhashtable_walk_start(&iter));
	if(!IS_ERR(obj))
	{
		while((obj = rhashtable_walk_next(&iter)) && !IS_ERR(obj))
		{
			kfree(obj->value);

			rhashtable_remove_fast(&kvstore, &obj->head, kvstore_params);
			kfree(obj);
		}

		rhashtable_walk_stop(&iter);
	}

	rhashtable_walk_exit(&iter);

	rhashtable_destroy(&kvstore);
}

/*
 * kvstore_get()
 * Retrieves the data block for an element with a given key.
 *
 * Returns NULL if the given key can't be found.
 */
char * kvstore_get(const char * key)
{
	struct kvstore_object * obj = rhashtable_lookup(&kvstore, key, kvstore_params);

	if(!obj)
	{
		// TODO report error here
		return NULL;
	}

	return obj->value;
}

/*
 * kvstore_insert()
 * Inserts a given value with a given key of a given length.
 */
void kvstore_insert(const char * key, const void * value, const int len)
{
	struct kvstore_object * obj = rhashtable_lookup_fast(&kvstore, key, kvstore_params);

	/* If the key already exists, just replace the value and move on. */
	if(obj)
	{
		kfree(obj->value);

		obj->value = kmalloc(len, GFP_KERNEL);
		memcpy(obj->value, value, len);

		return;
	}

	obj = kmalloc(sizeof(struct kvstore_object), GFP_KERNEL);

	/* Try to create the element, but don't do anything if it fails. */

	if(!obj)
	{
		// TODO report error here
		return;
	}

	/* Copy the data since we can't rely on userspace being sane.
	 * Again, bail out if something were to break somehow. */

	obj->value = kmalloc(len, GFP_KERNEL);
	if(!obj->value)
	{
		// TODO also report error here
		return;
	}

	memcpy(obj->value, value, len);

	rhashtable_insert_fast(&kvstore, &obj->head, kvstore_params);
}

/*
 * kvstore_remove()
 * Removes a value with a given key.
 *
 * Does nothing if the key does not have a corresponding value.
 */
void kvstore_remove(const char * key)
{
	struct kvstore_object * obj = rhashtable_lookup(&kvstore, key, kvstore_params);

	if(obj)
	{
		kfree(obj->value);

		rhashtable_remove_fast(&kvstore, &obj->head, kvstore_params);
		kfree(obj);
	}
}
