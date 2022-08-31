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
#define rpool_get_free_count(T) (rget_pool(T) == NULL ? 0 : rget_pool(T)->totalFree)

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
    union rpool_##TYPE##_item_u \
    { \
        rpool_##TYPE##_item_t* next; \
        TYPE data; \
    }; \
    typedef struct rpool_block_##TYPE##_t \
    { \
        int totalCount; \
        int freeCount; \
        int64_t itemFirst; \
        int64_t itemLast; \
        struct rpool_block_##TYPE##_t* nextBlock; \
        struct rpool_block_##TYPE##_t* prevBlock; \
        rpool_##TYPE##_item_t* head; \
        rpool_##TYPE##_item_t* items; \
    } rpool_block_##TYPE##_t; \
    typedef struct rpool_##TYPE##_t \
    { \
        int64_t capacity; \
        int64_t totalFree; \
        rpool_block_##TYPE##_t* freeHeadBlock; \
    } rpool_##TYPE##_t; \
    rpool_##TYPE##_t* rpool_##TYPE##_create(); \
    void rpool_##TYPE##_destroy(rpool_##TYPE##_t* pool); \
    TYPE* rpool_##TYPE##_get(rpool_##TYPE##_t* poolTemp); \
    int rpool_##TYPE##_free(TYPE* data, rpool_##TYPE##_t* pool)

#define rpool_init(TYPE, size_init, size_adjust) \
    rdeclare_pool(TYPE) = NULL; \
  static void travel_##TYPE##_info(void (*action_func)(void* item)) \
  { \
    rinfo("pool "#TYPE": [%"PRId64", %"PRId64"]", rget_pool(TYPE)->capacity, rget_pool(TYPE)->totalFree); \
    if (action_func) \
        action_func(rget_pool(TYPE)); \
  } \
  static inline rpool_block_##TYPE##_t* rpool_##TYPE##_expand(rpool_##TYPE##_t* poolTemp, int num) \
  { \
    if (!poolTemp) { \
	    rinfo(#TYPE" pool is NULL"); \
        return NULL; \
    } \
    rpool_block_##TYPE##_t* poolBlock = raymalloc(sizeof(rpool_block_##TYPE##_t)); \
    if (!poolBlock) \
    { \
		rinfo(#TYPE" poolBlock is NULL"); \
        return NULL; /* malloc failed */ \
    } \
    poolBlock->items = raycmalloc_type(num, rpool_##TYPE##_item_t); \
    if (!poolBlock->items) \
    { \
        rayfree(poolBlock); \
        poolBlock = NULL; \
		rinfo(#TYPE" poolBlock->items is NULL"); \
        return NULL; /* calloc failed */ \
    } \
    poolBlock->head = &poolBlock->items[0]; \
    for (size_t i = 0; i < num - 1; i++) \
    { \
      poolBlock->items[i].next = &poolBlock->items[i + 1]; \
    } \
    poolBlock->items[num - 1].next = NULL; \
    poolBlock->totalCount = num; \
    poolBlock->freeCount = num; \
    poolBlock->itemFirst = (int64_t)poolBlock->head; \
    poolBlock->itemLast = (int64_t)(&poolBlock->items[num - 1]); \
    poolBlock->nextBlock = NULL; \
    poolBlock->prevBlock = NULL; \
    poolTemp->capacity += num; \
    poolTemp->totalFree += num; \
    rinfo("expand block "#TYPE"(%"PRId64", %d, %"PRId64", %"PRId64") success, (%p)", \
		poolTemp->capacity, num, poolBlock->itemFirst, poolBlock->itemLast, poolBlock); \
    return poolBlock; \
  } \
  rpool_##TYPE##_t* rpool_##TYPE##_create() \
  { \
    rpool_##TYPE##_t* poolTemp = raymalloc(sizeof(rpool_##TYPE##_t)); \
    if (!poolTemp) \
    { \
      return NULL; /* memory malloc failed */ \
    } \
    poolTemp->capacity = 0; \
    poolTemp->totalFree = 0; \
    poolTemp->freeHeadBlock = NULL; \
    rpool_block_##TYPE##_t* poolBlock = rpool_##TYPE##_expand(poolTemp, size_init); \
    if (!poolBlock) { \
        rayfree(poolTemp); \
        poolTemp = NULL; \
        return NULL; \
    } \
    poolTemp->freeHeadBlock = poolBlock; \
    rpool_chain_node_t* chainNode = raymalloc(sizeof(rpool_chain_node_t)); \
    if (!chainNode) { \
        rinfo("create pool "#TYPE"(%d, %d) failed add to chain, (%p)", size_init, size_adjust, poolTemp); \
        return NULL; \
    } \
    chainNode->next = rpool_chain->next; \
    chainNode->prev = rpool_chain; \
    chainNode->pool_self = poolTemp; \
    chainNode->rpool_travel_block_func = (rpool_type_travel_block_func)travel_##TYPE##_info; \
    chainNode->rpool_destroy_pool_func = (rpool_type_destroy_pool_func)rpool_##TYPE##_destroy; \
    rpool_chain->next = chainNode; \
    rinfo("create pool "#TYPE"(%d, %d) success, (%p)", size_init, size_adjust, poolTemp); \
    return poolTemp; \
  } \
  void rpool_##TYPE##_destroy(rpool_##TYPE##_t* poolTemp) \
  { \
    poolTemp = poolTemp == NULL ? rget_pool(TYPE) : poolTemp; \
    if (!poolTemp || !rget_pool(TYPE)) { \
        rinfo("destroy "#TYPE"(%d, %d), pool is NULL, (%p)", size_init, size_adjust, poolTemp); \
        return; \
    } \
    rpool_block_##TYPE##_t* poolBlock = poolTemp->freeHeadBlock; \
    while (poolBlock) { \
		rpool_block_##TYPE##_t* nextBlock = poolBlock->nextBlock; \
        if (nextBlock) { \
            nextBlock->prevBlock = poolBlock->prevBlock; \
        } \
        rayfree(poolBlock->items); \
        rayfree(poolBlock); \
        poolBlock = nextBlock; \
    } \
	if (rpool_chain) { \
		rpool_chain_node_t* tempChainNode = rpool_chain; \
		while ((tempChainNode = tempChainNode->next) != NULL) { \
			if (tempChainNode->pool_self == poolTemp) { \
				if (tempChainNode->prev) { \
					tempChainNode->prev->next = tempChainNode->next; \
				} else { \
					rpool_chain = tempChainNode->next; \
				} \
				if (tempChainNode->next) { \
					tempChainNode->next->prev = tempChainNode->prev; \
				} \
				rayfree(tempChainNode); \
				tempChainNode = NULL; \
				break; \
			} \
		} \
	} \
    rinfo("destroy pool "#TYPE"(%d, %d) success, (%p)", size_init, size_adjust, poolTemp); \
    rayfree(poolTemp); \
    rget_pool(TYPE) = NULL; \
  } \
  TYPE* rpool_##TYPE##_get(rpool_##TYPE##_t* poolTemp) \
  { \
    if (!rget_pool(TYPE)) { \
        rinfo("get from "#TYPE"(%d, %d), pool is NULL, (%p)", size_init, size_adjust, poolTemp); \
        return NULL; \
    } \
    rpool_block_##TYPE##_t* poolBlock = poolTemp->freeHeadBlock; \
    rpool_##TYPE##_item_t* item = poolBlock->head; \
    if(!item) \
    { \
        while (poolBlock->nextBlock) { \
            poolBlock = poolBlock->nextBlock; \
            item = poolBlock->head; \
            if(item) \
                break; \
        } \
        if(!item) { \
            poolBlock = rpool_##TYPE##_expand(poolTemp, size_adjust); \
            if (!poolBlock) { \
                rinfo("malloc from pool "#TYPE"(%d, %d) failed.", size_init, size_adjust); \
                return NULL; \
            } \
            rpool_block_##TYPE##_t* blockSecond = poolTemp->freeHeadBlock->nextBlock; \
            poolTemp->freeHeadBlock->nextBlock = poolBlock;/*空的插入头部后面*/ \
            if (blockSecond) { \
                blockSecond->prevBlock = poolBlock; \
            } \
            poolBlock->prevBlock = poolTemp->freeHeadBlock; \
            poolBlock->nextBlock = blockSecond; \
            item = poolBlock->head; \
        } \
    } \
    poolBlock->head = item->next; \
    poolBlock->freeCount -= 1; \
    poolTemp->totalFree -= 1; \
    /** rinfo("malloc, "#TYPE"(%p)", item); **/ \
    return &item->data; \
  } \
  int rpool_##TYPE##_free(TYPE* data, rpool_##TYPE##_t* poolTemp) \
  { \
    if (!rget_pool(TYPE)) { \
        rinfo("free to "#TYPE"(%d, %d), pool is NULL, (%p)", size_init, size_adjust, poolTemp); \
        return -1; \
    } \
    rpool_block_##TYPE##_t* poolBlock = poolTemp->freeHeadBlock; \
    rpool_##TYPE##_item_t* itemData = (rpool_##TYPE##_item_t*)data;\
	/** rinfo("free from pool "#TYPE" block[%p, [%p], %p), %s", poolBlock->items, data, (poolBlock->items + poolBlock->totalCount), __FUNCTION__); **/ \
    if((itemData < poolBlock->items) || (itemData >= (poolBlock->items + poolBlock->totalCount))) \
    { \
        while ((poolBlock = poolBlock->nextBlock) != NULL) { \
			/** rinfo("free to "#TYPE" pool block(%d,%d) [%p, {%p}, %p)", \
				poolBlock->totalCount, poolBlock->freeCount, poolBlock->items, data, (poolBlock->items + poolBlock->totalCount)); **/ \
            if((itemData >= poolBlock->items) && (itemData < (poolBlock->items + poolBlock->totalCount))) { \
                break; \
            } \
        } \
        if (!poolBlock) { \
            rinfo("free to pool "#TYPE"(%d, %d) failed, %p", size_init, size_adjust, data); \
            return -1; \
        } \
    } \
    itemData->next = poolBlock->head; \
    poolBlock->head = itemData; \
    poolBlock->freeCount++; \
    poolTemp->totalFree++; \
    /** rinfo("free success: (%d,%d) [%p, {%p}, %p)", \
			poolBlock->totalCount, poolBlock->freeCount, poolBlock->items, data, poolBlock->items + poolBlock->totalCount); **/ \
    if (poolTemp->freeHeadBlock != poolBlock) { \
        if (poolTemp->totalFree > 2 * poolBlock->freeCount) { \
            if (unlikely(poolBlock->totalCount == poolBlock->freeCount)) { \
                rinfo("free block: "#TYPE"(%"PRId64", %"PRId64") [%p, %d]", poolTemp->capacity, poolTemp->totalFree, poolBlock, poolBlock->totalCount); \
                poolTemp->capacity -= poolBlock->totalCount; \
                poolTemp->totalFree -= poolBlock->totalCount; \
                poolBlock->prevBlock->nextBlock = poolBlock->nextBlock; \
                if (poolBlock->nextBlock) { \
                    poolBlock->nextBlock->prevBlock = poolBlock->prevBlock; \
                } \
                rayfree(poolBlock->items); \
                rayfree(poolBlock); \
/**        } else { **if ((poolTemp->capacity - poolTemp->totalFree) < (poolTemp->capacity / RAY_PROFILER_POOL_SHRINK)) { **\
                if (poolBlock = poolBlock->nextBlock) { \
                    if (poolBlock->prevBlock->freeCount > poolBlock->freeCount) { \
                        ** rinfo("change block: "#TYPE"(%"PRId64", %"PRId64") [%p, %d] ", poolTemp->capacity, poolTemp->totalFree, poolBlock, poolBlock->totalCount); ** \
                        rpool_block_##TYPE##_t* b1 = poolBlock->prevBlock; \
                        b1->nextBlock = poolBlock->nextBlock; \
                        poolBlock->nextBlock = b1; \
                        poolBlock->prevBlock = b1->prevBlock; \
                        b1->prevBlock = poolBlock; \
                    } \
                } \
**/        } \
        } \
    } \
    return rcode_ok; \
  } \
  rattribute_unused(static TYPE* malloc_##TYPE##_data (size_t size) \
  { \
    return rdata_new(TYPE); \
  }) \
  rattribute_unused(static void free_##TYPE##_data (TYPE* data) \
  { \
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
  typedef struct ralloc_anon_##ELE_SIZE##_t \
  { \
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
if (rpool_chain) { \
    rpool_chain_node_t* tempChainNode = NULL; \
    while ((tempChainNode = rpool_chain->next)) { \
        rinfo("rpool_chain must be NULL."); \
        rpool_chain->next = tempChainNode->next; \
        if (tempChainNode->rpool_destroy_pool_func != NULL) { \
            tempChainNode->rpool_destroy_pool_func(tempChainNode->pool_self); \
        } \
        rayfree(tempChainNode); \
        tempChainNode = NULL; \
    } \
    rayfree(rpool_chain); \
    rpool_chain = NULL; \
    rinfo("rpool_chain finished."); \
}



#ifdef __cplusplus
}
#endif //__cplusplus

#endif//RAY_POOL_H