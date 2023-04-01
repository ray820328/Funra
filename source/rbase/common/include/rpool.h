#ifndef RAY_POOL_H
#define RAY_POOL_H

#include "rmemory.h"
#include "rlog.h"
#include "rlist.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


#define rpool_t(TYPE) rpool_##TYPE##_t
#define rpool_create(TYPE) rpool_##TYPE##_create()
#define rpool_destroy(TYPE, POOL) rpool_##TYPE##_destroy(POOL)
#define rpool_get(TYPE, POOL) rpool_##TYPE##_get(POOL)
#define rpool_free(TYPE, OBJ, POOL) rpool_##TYPE##_free(OBJ, POOL)

#define rdefine_pool_name(T) data_##T##_pool
#define rdefine_pool(T, size_init, size_adjust) rpool_init(T, size_init, size_adjust)
#define rdeclare_pool(T) rpool_t(T)* rdefine_pool_name(T)
#define rget_pool(T) rdefine_pool_name(T)
#define rcreate_pool(T) rpool_create(T)
#define rdestroy_pool(T) \
        do { \
            rpool_destroy(T, rdefine_pool_name(T)); \
            rdefine_pool_name(T) = NULL; \
        } while(0)

#define rpool_new_data(T) rpool_get(T, rget_pool(T))
#define rpool_free_data(T, data) \
        do { \
            rpool_free(T, data, rget_pool(T)); \
            data = NULL; \
        } while(0)

#define rpool_get_capacity(T) (rget_pool(T) == NULL ? 0 : rget_pool(T)->capacity)
#define rpool_get_free_count(T) (rget_pool(T) == NULL ? 0 : rget_pool(T)->total_free)

typedef void (*rpool_type_travel_block_func)(void(*do_action)(void* item));
typedef void (*rpool_type_destroy_pool_func)(void* pool);

typedef struct rdata_pool_block {
    uint64_t used_bits;//0:free 1:used
    unsigned int data_total_size;
    char data_buffer[0];
} rdata_pool_block;

typedef struct rdata_pool {
    unsigned int size_elem;
    unsigned int init_elems;
    unsigned int adjust_elems;
    rlist_t* full_list;
    rlist_t* free_list;
} rdata_pool;

typedef struct rpool_chain_node_t {
    void* pool_self;
    rpool_type_travel_block_func rpool_travel_block_func;
    rpool_type_destroy_pool_func rpool_destroy_pool_func;
    struct rpool_chain_node_t* prev;
    struct rpool_chain_node_t* next;
} rpool_chain_node_t;

extern rpool_chain_node_t* rpool_chain;

#define rpool_declare(TYPE)\
    typedef union rpool_##TYPE##_item_u rpool_##TYPE##_item_t; \
    union rpool_##TYPE##_item_u { \
        rpool_##TYPE##_item_t* next; \
        TYPE data; \
    }; \
    typedef struct rpool_block_##TYPE##_t { \
        int total_count; \
        int free_count; \
        int64_t item_first; \
        int64_t item_last; \
        struct rpool_block_##TYPE##_t* next_block; \
        struct rpool_block_##TYPE##_t* prev_block; \
        rpool_##TYPE##_item_t* head; \
        rpool_##TYPE##_item_t* items; \
    } rpool_block_##TYPE##_t; \
    typedef struct rpool_##TYPE##_t { \
        int64_t capacity; \
        int64_t total_free; \
        rpool_block_##TYPE##_t* free_head_block; \
    } rpool_##TYPE##_t; \
    rpool_##TYPE##_t* rpool_##TYPE##_create(); \
    void rpool_##TYPE##_destroy(rpool_##TYPE##_t* pool); \
    TYPE* rpool_##TYPE##_get(rpool_##TYPE##_t* pool); \
    int rpool_##TYPE##_free(TYPE* data, rpool_##TYPE##_t* pool)

#define rpool_init(TYPE, size_init, size_adjust) \
  rdeclare_pool(TYPE) = NULL; \
  static void rpool_travel_##TYPE##_info(void (*action_func)(void* item)) { \
    rinfo("pool "#TYPE": [%"PRId64", %"PRId64"]", rget_pool(TYPE)->capacity, rget_pool(TYPE)->total_free); \
    if (action_func != NULL) { \
        action_func(rget_pool(TYPE)); \
    } \
  } \
  static inline rpool_block_##TYPE##_t* rpool_##TYPE##_expand(rpool_##TYPE##_t* pool, int num) { \
    if (pool == NULL) { \
	    rinfo(#TYPE" rpool is NULL"); \
        return NULL; \
    } \
    rpool_block_##TYPE##_t* pool_block = raymalloc(sizeof(rpool_block_##TYPE##_t)); \
    if (pool_block == NULL) { \
		rerror(#TYPE" pool_block is NULL"); \
        return NULL; /* malloc failed */ \
    } \
    pool_block->items = raycmalloc_type(num, rpool_##TYPE##_item_t); \
    if (pool_block->items == NULL) { \
        rayfree(pool_block); \
        pool_block = NULL; \
		rerror(#TYPE" pool_block->items is NULL"); \
        return NULL; /* calloc failed */ \
    } \
    pool_block->head = &pool_block->items[0]; \
    for (size_t i = 0; i < num - 1; i++) { \
      pool_block->items[i].next = &pool_block->items[i + 1]; \
    } \
    pool_block->items[num - 1].next = NULL; \
    pool_block->total_count = num; \
    pool_block->free_count = num; \
    pool_block->item_first = (int64_t)pool_block->head; \
    pool_block->item_last = (int64_t)(&pool_block->items[num - 1]); \
    pool_block->next_block = NULL; \
    pool_block->prev_block = NULL; \
    pool->capacity += num; \
    pool->total_free += num; \
    rinfo("expand block "#TYPE"(%"PRId64", %d, %p->%p = [%"PRId64", %"PRId64"]) success, (%p)", \
		pool->capacity, num, pool_block, pool_block->items, pool_block->item_first, pool_block->item_last, pool); \
    return pool_block; \
  } \
  rpool_##TYPE##_t* rpool_##TYPE##_create() { \
    rpool_##TYPE##_t* pool = raymalloc(sizeof(rpool_##TYPE##_t)); \
    if (pool == NULL) { \
      return NULL; /* memory malloc failed */ \
    } \
    pool->capacity = 0; \
    pool->total_free = 0; \
    pool->free_head_block = NULL; \
    rpool_block_##TYPE##_t* pool_block = rpool_##TYPE##_expand(pool, size_init); \
    if (pool_block == NULL) { \
        rayfree(pool); \
        pool = NULL; \
        return NULL; \
    } \
    pool->free_head_block = pool_block; \
    rpool_chain_node_t* chain_node = raymalloc(sizeof(rpool_chain_node_t)); \
    if (chain_node == NULL) { \
        rinfo("create rpool "#TYPE"(%d, %d) failed add to chain, (%p)", size_init, size_adjust, pool); \
        return NULL; \
    } \
    chain_node->next = rpool_chain->next; \
    chain_node->prev = rpool_chain; \
    chain_node->pool_self = pool; \
    chain_node->rpool_travel_block_func = (rpool_type_travel_block_func)rpool_travel_##TYPE##_info; \
    chain_node->rpool_destroy_pool_func = (rpool_type_destroy_pool_func)rpool_##TYPE##_destroy; \
    rpool_chain->next = chain_node; \
    rinfo("create rpool "#TYPE"(%d, %d) success, (%p)", size_init, size_adjust, pool); \
    return pool; \
  } \
  void rpool_##TYPE##_destroy(rpool_##TYPE##_t* pool) { \
    pool = pool == NULL ? rget_pool(TYPE) : pool; \
    if (pool == NULL || rget_pool(TYPE) == NULL) { \
        rinfo("destroy "#TYPE"(%d, %d), rpool is NULL, (%p)", size_init, size_adjust, pool); \
        return; \
    } \
    rpool_block_##TYPE##_t* pool_block = pool->free_head_block; \
    while (pool_block != NULL) { \
		rpool_block_##TYPE##_t* next_block = pool_block->next_block; \
        if (next_block != NULL) { \
            next_block->prev_block = pool_block->prev_block; \
        } \
        rayfree(pool_block->items); \
        rayfree(pool_block); \
        pool_block = next_block; \
    } \
	if (rpool_chain != NULL) { \
		rpool_chain_node_t* chain_node_temp = rpool_chain; \
		while ((chain_node_temp = chain_node_temp->next) != NULL) { \
			if (chain_node_temp->pool_self == pool) { \
				if (chain_node_temp->prev != NULL) { \
					chain_node_temp->prev->next = chain_node_temp->next; \
				} else { \
					rpool_chain = chain_node_temp->next; \
				} \
				if (chain_node_temp->next != NULL) { \
					chain_node_temp->next->prev = chain_node_temp->prev; \
				} \
				rayfree(chain_node_temp); \
				chain_node_temp = NULL; \
				break; \
			} \
		} \
	} \
    rinfo("destroy rpool "#TYPE"(%d, %d) success, (%p)", size_init, size_adjust, pool); \
    rayfree(pool); \
    rget_pool(TYPE) = NULL; \
  } \
  TYPE* rpool_##TYPE##_get(rpool_##TYPE##_t* pool) { \
    if (rget_pool(TYPE) == NULL) { \
        rerror("get from "#TYPE"(%d, %d), rpool is NULL, (%p)", size_init, size_adjust, pool); \
        return NULL; \
    } \
    rpool_block_##TYPE##_t* pool_block = pool->free_head_block; \
    rpool_##TYPE##_item_t* item = pool_block->head; \
    if(item == NULL) { \
        while (pool_block->next_block != NULL) { \
            pool_block = pool_block->next_block; \
            item = pool_block->head; \
            if(item != NULL) \
                break; \
        } \
        if(item == NULL) { \
            pool_block = rpool_##TYPE##_expand(pool, size_adjust); \
            if (pool_block == NULL) { \
                rinfo("malloc from rpool "#TYPE"(%d, %d) failed.", size_init, size_adjust); \
                return NULL; \
            } \
            rpool_block_##TYPE##_t* block_second = pool->free_head_block->next_block; \
            pool->free_head_block->next_block = pool_block;/*空的插入头部后面*/ \
            if (block_second != NULL) { \
                block_second->prev_block = pool_block; \
            } \
            pool_block->prev_block = pool->free_head_block; \
            pool_block->next_block = block_second; \
            item = pool_block->head; \
        } \
    } \
    pool_block->head = item->next; \
    pool_block->free_count -= 1; \
    pool->total_free -= 1; \
    /** rinfo("malloc, "#TYPE"(%p)", item); **/ \
    return &(item->data); \
  } \
  int rpool_##TYPE##_free(TYPE* data, rpool_##TYPE##_t* pool) { \
    if (rget_pool(TYPE) == NULL) { \
        rerror("free to "#TYPE"(%d, %d), rpool is NULL, (%p)", size_init, size_adjust, pool); \
        return -1; \
    } \
    rpool_block_##TYPE##_t* pool_block = pool->free_head_block; \
    rpool_##TYPE##_item_t* item_data = (rpool_##TYPE##_item_t*)data;\
	/** rinfo("free from pool "#TYPE" block[%p, [%p], %p), %s", pool_block->items, data, (pool_block->items + pool_block->total_count), __FUNCTION__); **/ \
    if((item_data < pool_block->items) || (item_data >= (pool_block->items + pool_block->total_count))) { \
        while ((pool_block = pool_block->next_block) != NULL) { \
			/** rinfo("free to "#TYPE" pool block(%d,%d) [%p, {%p}, %p)", \
				pool_block->total_count, pool_block->free_count, pool_block->items, data, (pool_block->items + pool_block->total_count)); **/ \
            if((item_data >= pool_block->items) && (item_data < (pool_block->items + pool_block->total_count))) { \
                break; \
            } \
        } \
        if (pool_block == NULL) { \
            rerror("free to pool "#TYPE"(%d, %d) failed, %p", size_init, size_adjust, data); \
            return -1; \
        } \
    } \
    item_data->next = pool_block->head; \
    pool_block->head = item_data; \
    pool_block->free_count++; \
    pool->total_free++; \
    /** rinfo("free success: (%d,%d) [%p, {%p}, %p)", \
			pool_block->total_count, pool_block->free_count, pool_block->items, data, pool_block->items + pool_block->total_count); **/ \
    if (pool->free_head_block != pool_block) { \
        if (pool->total_free > 2 * pool_block->free_count) { \
            if (unlikely(pool_block->total_count == pool_block->free_count)) { \
                rinfo("free block: "#TYPE"(%"PRId64", %"PRId64") [%p, %d]", pool->capacity, pool->total_free, pool_block, pool_block->total_count); \
                pool->capacity -= pool_block->total_count; \
                pool->total_free -= pool_block->total_count; \
                pool_block->prev_block->next_block = pool_block->next_block; \
                if (pool_block->next_block != NULL) { \
                    pool_block->next_block->prev_block = pool_block->prev_block; \
                } \
                rayfree(pool_block->items); \
                rayfree(pool_block); \
/**        } else { **if ((pool->capacity - pool->total_free) < (pool->capacity / RAY_PROFILER_POOL_SHRINK)) { **\
                if (pool_block = pool_block->next_block) { \
                    if (pool_block->prev_block->free_count > pool_block->free_count) { \
                        ** rinfo("change block: "#TYPE"(%"PRId64", %"PRId64") [%p, %d] ", pool->capacity, pool->total_free, pool_block, pool_block->total_count); ** \
                        rpool_block_##TYPE##_t* b1 = pool_block->prev_block; \
                        b1->next_block = pool_block->next_block; \
                        pool_block->next_block = b1; \
                        pool_block->prev_block = b1->prev_block; \
                        b1->prev_block = pool_block; \
                    } \
                } \
**/        } \
        } \
    } \
    return rcode_ok; \
  } \
  rattribute_unused(static TYPE* malloc_##TYPE##_data (size_t size) { \
    return rdata_new(TYPE); \
  }) \
  rattribute_unused(static void free_##TYPE##_data (TYPE* data) { \
    rdata_free(TYPE, data); \
  })


#define pool_block_item_count 64
#define dada_from_block(block, T, datasize, index) (T)(((rdata_pool_block*)(block))->data_buffer + index * datasize * sizeof(char))
#define dada_check_block(block, ptr) (ptr && ((int64_t)(ptr - ((rdata_pool_block*)(block))->data_buffer) < ((rdata_pool_block*)(block))->data_total_size))
#define dada_free_flag_block(block, T, index) ((rdata_pool_block*)(block))->used_bits = (((rdata_pool_block*)(block))->used_bits & (~(1ull << index)))
#define dada_used_flag_block(block, T, index) ((rdata_pool_block*)(block))->used_bits = (((rdata_pool_block*)(block))->used_bits | (1ull << index))
#define dada_full_block(block) (~(((rdata_pool_block*)(block))->used_bits) == 0)
#define dada_empty_block(block) ((((rdata_pool_block*)(block))->used_bits) == 0)
#define data_index_block(block, used_bits, index) \
        do { \
            index = rtools_startindex1(used_bits); \
            index += 1; \
            if (index > pool_block_item_count - 1) { \
                    index = rtools_endindex1(used_bits); \
                    index -= 1; \
            } \
        } while(0)

#define rmacro_concat(PRE, NEXT0) PRE##NEXT0
//ELE_SIZE不小于8字节，需要至少放下一个指针空间用于初始化
#define def_ranonymous_pool(ELE_SIZE)	\
  typedef struct ralloc_anon_##ELE_SIZE##_t { \
    char data[ELE_SIZE]; \
  } ralloc_anon_##ELE_SIZE##_t; \
  static struct rdeclare_pool(ralloc_anon_##ELE_SIZE##_t)
  /* rdefine_pool(ralloc_anon_##ELE_SIZE##_t, size_init, size_adjust) 外面有对齐*/

#define rpool_define_global() \
extern rpool_chain_node_t* rpool_chain; \
rpool_chain_node_t* rpool_chain = NULL \

#define rpool_init_global() \
if (rpool_chain == NULL) { \
    rpool_chain = raymalloc(sizeof(rpool_chain_node_t)); \
    rpool_chain->next = NULL; \
    rpool_chain->prev = NULL; \
    rpool_chain->rpool_travel_block_func = NULL; \
    rpool_chain->rpool_travel_block_func = NULL; \
}

#define rpool_uninit_global() \
extern rpool_chain_node_t* rpool_chain; \
if (rpool_chain != NULL) { \
    rpool_chain_node_t* chain_node_temp = NULL; \
    while ((chain_node_temp = rpool_chain->next) != NULL) { \
        rinfo("rpool_chain must be NULL."); \
        rpool_chain->next = chain_node_temp->next; \
        if (chain_node_temp->rpool_destroy_pool_func != NULL) { \
            chain_node_temp->rpool_destroy_pool_func(chain_node_temp->pool_self); \
        } \
        rayfree(chain_node_temp); \
        chain_node_temp = NULL; \
    } \
    rayfree(rpool_chain); \
    rpool_chain = NULL; \
    rinfo("rpool_chain finished."); \
}



#ifdef __cplusplus
}
#endif //__cplusplus

#endif//RAY_POOL_H