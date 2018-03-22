#include <linux/string.h>
#include <linux/slab.h>

#include "keyvalue.h"

struct rhashtable kvs;
struct rhashtable_params kvs_params =
{
	.head_offset = offsetof(struct kvs_object, node),
	.key_offset = offsetof(struct kvs_object, key),
	.key_len = FIELD_SIZEOF(struct kvs_object, key),
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
			pr_info("[kvs_exit] - removing element with key %d and value %s... (%d bytes)", obj->key, obj->value, obj->value_len);
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
 * 
 *
 *
 */
void kvs_get_storage_info(char* finalstring)
{
	char temp[100];
	
	struct rhashtable_iter it;
	struct kvs_object * obj;
	memset(temp, 0, sizeof(temp));
	

	rhashtable_walk_enter(&kvs, &it);
	
	obj = ERR_PTR(rhashtable_walk_start(&it));
	if(!IS_ERR(obj))
	{
		while((obj = rhashtable_walk_next(&it)) && !IS_ERR(obj))
		{
			sprintf(temp, "key: %d & value: %s\n",obj->key , obj->value);
			strcat(finalstring, temp);
		}

		rhashtable_walk_stop(&it);
	}

	rhashtable_walk_exit(&it);
	
}

/*
 * kvs_get()
 * Retrieves the data block for an element with a given key.
 *
 * Returns NULL if the given key can't be found.
 */
char * kvs_get(const char * key)
{
	struct kvs_object * obj;
	int key_hash = jhash(key, strlen(key), 0);

	obj = rhashtable_lookup_fast(&kvs, &key_hash, kvs_params);
	if(obj == NULL)
	{
		pr_info("[kvs_get] - unable to find element with key %d", key_hash);
		return NULL;
	}

	return obj->value;
}

/*
 * kvs_insert()
 * Inserts a given value with a given key of a given length.
 */
int kvs_insert(const char * key, const void * value, const int value_len)
{
	struct kvs_object * obj, * old_obj;

	/* Get the container object ready. */

	obj = kzalloc(sizeof(struct kvs_object), GFP_KERNEL);
	if(!obj)
	{
		pr_warn("[kvs_insert] - unable to allocate KVS object!");
		return -1;
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
		return -1;
	}

	/* Hash the key string and insert the element. */

	obj->key = jhash(key, strlen(key), 0);

	old_obj = rhashtable_insert_slow(&kvs, &obj->key, &obj->node);
	if(old_obj != NULL)
	{
		kfree(old_obj->value);
		old_obj->value = obj->value;

		kfree(obj);

		pr_info("[kvs_insert] - replacing object with hash %d...", old_obj->key);
		return -1;
	}

	pr_info("[kvs_insert] - inserting object with hash %d...", obj->key);
	return 0;
}

/*
 * kvs_remove()
 * Removes a value with a given key.
 *
 * Does nothing if the key does not have a corresponding value.
 */
void kvs_remove(const char * key)
{
	struct kvs_object * obj;
	int key_hash = jhash(key, strlen(key), 0);

	obj = rhashtable_lookup_fast(&kvs, &key_hash, kvs_params);
	if(obj)
	{
		pr_info("[kvs_remove - removing element with key %s, hash %d... (%d bytes)", key, key_hash, obj->value_len);
		rhashtable_remove_fast(&kvs, &obj->node, kvs_params);

		kfree(obj->value);
		kfree(obj);
	}
}
