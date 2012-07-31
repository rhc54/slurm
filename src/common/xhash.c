#include "xhash.h"

#include <xmalloc.h>

#include "uthash/uthash.h"

#if 0
/* undefine default allocators */
#undef uthash_malloc
#undef uthash_free

/* re-define them using slurm's ones */
#define uthash_malloc(sz) xmalloc(sz)
#define uthash_free(ptr, sz) xfree(ptr)
#endif

/*
 * FIXME:
 * A pre-malloced array of xhash_item_t could be associated to
 * the xhash_table in order to speed up the xhash_add function.
 * The default array size could be something like 10% of the
 * provided table_size (hash table are commonly defined larger
 * than necessary to avoid shared keys and usage of linked list)
 */

typedef struct xhash_item_st {
	void*		item;    /* user item                               */
	const char*	key;     /* cached key calculated by user function, */
                                 /* needed by uthash                        */
	uint32_t	keysize; /* cached key size                         */
	UT_hash_handle	hh;      /* make this structure hashable by uthash  */
} xhash_item_t;

struct xhash_st {
	xhash_item_t*	ht;       /* hash table                          */
	uint32_t	count;    /* user items count                    */
	xhash_idfunc_t	identify; /* function returning a unique str key */
};

static xhash_item_t* xhash_find(xhash_t* table, const char* key)
{
	xhash_item_t* hash_item = NULL;
	uint32_t      key_size  = 0;
	if (!table || !key) return NULL;
	key_size = strlen(key);
	HASH_FIND(hh, table->ht, key, key_size, hash_item);
	return hash_item;
}

void* xhash_get(xhash_t* table, const char* key)
{
	xhash_item_t* item = xhash_find(table, key);
	if (!item) return NULL;
	return item->item;
}

xhash_t* xhash_init(xhash_idfunc_t idfunc,
                xhash_hashfunc_t hashfunc,
                uint32_t table_size)
{
	xhash_t* table = NULL;
	if (!idfunc) return NULL;
	table = (xhash_t*)xmalloc(sizeof(xhash_t));
	table->ht = NULL; /* required by uthash */
	table->count = 0;
	table->identify = idfunc;
	return table;
}

void* xhash_add(xhash_t* table, void* item)
{
	xhash_item_t* hash_item = NULL;
	if (!table || !item) return NULL;
	hash_item          = (xhash_item_t*)xmalloc(sizeof(xhash_item_t));
	hash_item->item    = item;
	hash_item->key     = table->identify(item);
	hash_item->keysize = strlen(hash_item->key);
	HASH_ADD_KEYPTR(hh, table->ht, hash_item->key,
			hash_item->keysize, hash_item);
	++table->count;
	return hash_item->item;
}

void xhash_delete(xhash_t* table, const char* key)
{
	xhash_item_t* item = xhash_find(table, key);
	if (!item) return;
	HASH_DELETE(hh, table->ht, item);
	xfree(item);
	--table->count;
}

uint32_t xhash_count(xhash_t* table)
{
	if (!table) return 0;
	return table->count;
}

void xhash_walk(xhash_t* table,
		void (*callback)(void* item, void* arg),
		void* arg)
{
	xhash_item_t* current_item = NULL;
	xhash_item_t* tmp = NULL;
	if (!table || !callback) return;
	HASH_ITER(hh, table->ht, current_item, tmp) {
		callback(current_item->item, arg);
	}
}

void xhash_free(xhash_t* table)
{
	xhash_item_t* current_item = NULL;
	xhash_item_t* tmp = NULL;
	if (!table) return;
	HASH_ITER(hh, table->ht, current_item, tmp) {
		HASH_DEL(table->ht, current_item);
		xfree(current_item);
	}
	xfree(table);
}

