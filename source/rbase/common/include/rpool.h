#ifndef RAY_POOL_H
#define RAY_POOL_H

#include "rlist.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#pragma region "自定义内存池"

#define rpool_t(TYPE) rpool_##TYPE##_t
#define rpool_create(TYPE) rpool_##TYPE##_create()
#define rpool_destroy(TYPE, POOL) rpool_##TYPE##_destroy(POOL)
#define rpool_get(TYPE, POOL) rpool_##TYPE##_get(POOL)
#define rpool_free(TYPE, OBJ, POOL) rpool_##TYPE##_free(OBJ, POOL)

#define rdefine_pool_name(T) data_##T##_pool
#define rdefine_pool(T, SIZE_INIT, SIZE_ADJUST) RPOOL_INIT(T, SIZE_INIT, SIZE_ADJUST)
#define rdeclare_pool(T) rpool_t(T)* rdefine_pool_name(T)
#define rget_pool(T) rdefine_pool_name(T)
#define rcreate_pool(T) rpool_create(T)
#define rdestroy_pool(T) \
            do { \
                rpool_destroy(T, rdefine_pool_name(T)); \
                rdefine_pool_name(T) = NULL; \
            } while(0)
            
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
    void (*travelRPool)(void (*doFn)(void* item));
    struct rpool_chain_node_t* prev;
    struct rpool_chain_node_t* next;
} rpool_chain_node_t;

extern rpool_chain_node_t* rpool_chain;

#define RPOOL_INIT(TYPE, SIZE_INIT, SIZE_ADJUST)	\
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
  static void travel_##TYPE##_info(void (*doFn)(void* item)) \
  { \
    printdbgr("pool "#TYPE": [%%"PRId64", %%"PRId64"]\n", rget_pool(TYPE)->capacity, rget_pool(TYPE)->totalFree); \
    if (doFn) \
        doFn(rget_pool(TYPE)); \
  } \
  static RPROFILER_INLINE rpool_block_##TYPE##_t* rpool_##TYPE##_expand(rpool_##TYPE##_t* poolTemp, int num) \
  { \
    if (!poolTemp) { \
	    printdbgr(#TYPE" pool is NULL\n"); \
        return NULL; \
    } \
    rpool_block_##TYPE##_t* poolBlock = raymalloc(sizeof(rpool_block_##TYPE##_t)); \
    if (!poolBlock) \
    { \
		printdbgr(#TYPE" poolBlock is NULL\n"); \
        return NULL; /* malloc failed */ \
    } \
    poolBlock->items = calloc(num, sizeof(rpool_##TYPE##_item_t)); \
    if (!poolBlock->items) \
    { \
        rayfree(poolBlock); \
        poolBlock = NULL; \
		printdbgr(#TYPE" poolBlock->items is NULL\n"); \
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
    printdbgr("expand block "#TYPE"(%%"PRId64", %d, %%"PRId64", %%"PRId64") success, (%p)\n", \
		poolTemp->capacity, num, poolBlock->itemFirst, poolBlock->itemLast, poolBlock); \
    return poolBlock; \
  } \
  static RPROFILER_INLINE rpool_##TYPE##_t* rpool_##TYPE##_create() \
  { \
    rpool_##TYPE##_t* poolTemp = raymalloc(sizeof(rpool_##TYPE##_t)); \
    if (!poolTemp) \
    { \
      return NULL; /* memory malloc failed */ \
    } \
    poolTemp->capacity = 0; \
    poolTemp->totalFree = 0; \
    poolTemp->freeHeadBlock = NULL; \
    rpool_block_##TYPE##_t* poolBlock = rpool_##TYPE##_expand(poolTemp, SIZE_INIT); \
    if (!poolBlock) { \
        rayfree(poolTemp); \
        poolTemp = NULL; \
        return NULL; \
    } \
    poolTemp->freeHeadBlock = poolBlock; \
    rpool_chain_node_t* chainNode = raymalloc(sizeof(rpool_chain_node_t)); \
    if (!chainNode) { \
        printdbgr("create pool "#TYPE"(%d, %d) failed add to chain, (%p)\n", SIZE_INIT, SIZE_ADJUST, poolTemp); \
        return NULL; \
    } \
    chainNode->next = rpool_chain->next; \
    chainNode->prev = rpool_chain; \
    chainNode->travelRPool = travel_##TYPE##_info; \
    rpool_chain->next = chainNode; \
    printdbgr("create pool "#TYPE"(%d, %d) success, (%p)\n", SIZE_INIT, SIZE_ADJUST, poolTemp); \
    return poolTemp; \
  } \
  static RPROFILER_INLINE void rpool_##TYPE##_destroy(rpool_##TYPE##_t* poolTemp) \
  { \
    if (!poolTemp || !rget_pool(TYPE)) { \
        printdbgr("destroy "#TYPE"(%d, %d), pool is NULL, (%p)\n", SIZE_INIT, SIZE_ADJUST, poolTemp); \
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
			if (tempChainNode->travelRPool == travel_##TYPE##_info) { \
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
    printdbgr("destroy pool "#TYPE"(%d, %d) success, (%p)\n", SIZE_INIT, SIZE_ADJUST, poolTemp); \
    rayfree(poolTemp); \
    rget_pool(TYPE) = NULL; \
  } \
  static RPROFILER_INLINE TYPE* rpool_##TYPE##_get(rpool_##TYPE##_t* poolTemp) \
  { \
    if (!rget_pool(TYPE)) { \
        printdbgr("get from "#TYPE"(%d, %d), pool is NULL, (%p)\n", SIZE_INIT, SIZE_ADJUST, poolTemp); \
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
            poolBlock = rpool_##TYPE##_expand(poolTemp, SIZE_ADJUST); \
            if (!poolBlock) { \
                printdbgr("malloc from pool "#TYPE"(%d, %d) failed.\n", SIZE_INIT, SIZE_ADJUST); \
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
    /** printdbgr("malloc, "#TYPE"(%p)\n", item); **/ \
    return &item->data; \
  } \
  static RPROFILER_INLINE bool rpool_##TYPE##_free(TYPE* data, rpool_##TYPE##_t* poolTemp) \
  { \
    if (!rget_pool(TYPE)) { \
        printdbgr("free to "#TYPE"(%d, %d), pool is NULL, (%p)\n", SIZE_INIT, SIZE_ADJUST, poolTemp); \
        return false; \
    } \
    rpool_block_##TYPE##_t* poolBlock = poolTemp->freeHeadBlock; \
    rpool_##TYPE##_item_t* itemData = (rpool_##TYPE##_item_t*)data;\
	/** printdbgr("free from pool "#TYPE" block[%p, [%p], %p), %s\n", poolBlock->items, data, (poolBlock->items + poolBlock->totalCount), __FUNCTION__); **/ \
    if((itemData < poolBlock->items) || (itemData >= (poolBlock->items + poolBlock->totalCount))) \
    { \
        while ((poolBlock = poolBlock->nextBlock) != NULL) { \
			/** printdbgr("free to "#TYPE" pool block(%d,%d) [%p, {%p}, %p)\n", \
				poolBlock->totalCount, poolBlock->freeCount, poolBlock->items, data, (poolBlock->items + poolBlock->totalCount)); **/ \
            if((itemData >= poolBlock->items) && (itemData < (poolBlock->items + poolBlock->totalCount))) { \
                break; \
            } \
        } \
        if (!poolBlock) { \
            printdbgr("free to pool "#TYPE"(%d, %d) failed, %p\n", SIZE_INIT, SIZE_ADJUST, data); \
            return false; \
        } \
    } \
    itemData->next = poolBlock->head; \
    poolBlock->head = itemData; \
    poolBlock->freeCount++; \
    poolTemp->totalFree++; \
    /** printdbgr("free success: (%d,%d) [%p, {%p}, %p)\n", \
			poolBlock->totalCount, poolBlock->freeCount, poolBlock->items, data, poolBlock->items + poolBlock->totalCount); **/ \
    if (poolTemp->freeHeadBlock != poolBlock) { \
        if (poolTemp->totalFree > 2 * poolBlock->freeCount) { \
            if (unlikely(poolBlock->totalCount == poolBlock->freeCount)) { \
                printdbgr("free block: "#TYPE"(%%"PRId64", %%"PRId64") [%p, %d]\n", poolTemp->capacity, poolTemp->totalFree, poolBlock, poolBlock->totalCount); \
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
                        ** printdbgr("change block: "#TYPE"(%%"PRId64", %%"PRId64") [%p, %d] \n", poolTemp->capacity, poolTemp->totalFree, poolBlock, poolBlock->totalCount); ** \
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
    return true; \
  } \
  __attribute__((unused)) static TYPE* malloc_##TYPE##_data (size_t size) \
  { \
    return rnew_data(TYPE); \
  } \
  __attribute__((unused)) static void free_##TYPE##_data (TYPE* data) \
  { \
    rfree_data(TYPE, data); \
  }


#define POOL_BLOCK_ITEM_COUNT 64
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
                if (index > POOL_BLOCK_ITEM_COUNT - 1) { \
                        index = rtools_endindex1(used_bits); \
                        index -= 1; \
                } \
            } while(0)

#define rconcat(PRE, NEXT0) PRE##NEXT0
//ELE_SIZE不小于8字节，需要至少放下一个指针空间用于初始化
#define def_ranonymous_pool(ELE_SIZE)	\
  typedef struct ralloc_anon_##ELE_SIZE##_t \
  { \
    char data[ELE_SIZE]; \
  } ralloc_anon_##ELE_SIZE##_t; \
  static struct rdeclare_pool(ralloc_anon_##ELE_SIZE##_t)
  /* rdefine_pool(ralloc_anon_##ELE_SIZE##_t, SIZE_INIT, SIZE_ADJUST) 外面有对齐*/


#pragma endregion "自定义内存池"

#pragma region "其他"

static inline int rtools_endindex1(uint64_t val) {//1 end index (0x1-0x8000000000000000返回0-63, val==0返回-1)
    if (val == 0) {
        return -1;
    }
#ifdef WIN32
    int index = 0;
    _BitScanForward64(&index, val);
    return index;
#else
    return __builtin_ctzll(val);//__builtin_ffsll - 前导1 //__builtin_ctz - 后导0个数，__builtin_clz - 前导0个数
#endif
    }
static inline int rtools_startindex1(uint64_t val) {//1 start index (0x1-0x8000000000000000返回0-63, val==0返回-1)
    if (val == 0) {
        return -1;
    }
#ifdef WIN32
    int index = 0;
    _BitScanReverse64(&index, val);
    return index;
#else
    return 63 - __builtin_clzll(val);// __builtin_clzll(val);//前导的0的个数
#endif
}
static inline int rtools_popcount1(uint64_t val) {//1bits
#ifdef WIN32
    return -1;
#else
    return __builtin_popcountll(val);
#endif
}

#pragma endregion "其他"


#ifdef __cplusplus
}
#endif //__cplusplus

#endif//RAY_POOL_H