﻿/** 
 * Copyright (c) 2016
 *
 * This library is free software { you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 * - 仅支持lua5.3
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lua.h"
#include "lapi.h"
#include "lauxlib.h"
#include "lualib.h"
#include "lobject.h"
#include "lstate.h"
#include "ltable.h"

#define RAY_BUILD_AS_DLL
#define RAY_LIB

#include "rcommon.h"
#include "rstring.h"
#include "rlog.h"
#include "rpool.h"
#include "rdict.h"
#include "rlist.h"

#ifdef ros_windows
#pragma comment(lib, "luad.lib")
#endif

static rdict_t* all_data = NULL;
static rlist_t* travel_list = NULL;

#define key_len_max 32

static int inited = 0;
static int table_level = 1;

#define size_ptr 8
#define gnode_last(h)    gnode(h, cast(size_t, sizenode(h)))

struct rprofiler_data_child_s;
typedef struct rprofiler_data_child_s rprofiler_data_child_t;
struct rprofiler_mem_data_s;
typedef struct rprofiler_mem_data_s rprofiler_mem_data_t;

struct rprofiler_data_child_s {
    rprofiler_mem_data_t* data;
    rprofiler_data_child_t* next;
};

struct rprofiler_mem_data_s {
    int size;
    int ref_size;//引用对象size
    rprofiler_data_child_t* children;
    rprofiler_data_child_t* parent;//仅临时当前，可能有多个
    void* self;//仅临时

    char desc[0];
};

rpool_define_global();
rpool_declare(rprofiler_data_child_t);// .h
rdefine_pool(rprofiler_data_child_t, 10000, 5000); // .c


// #define dump_lua_stack(L) dump_stack(L, get_filename(__FILE__), __FUNCTION__, __LINE__)
#define dump_lua_stack(L)
static int dump_stack(lua_State *L, char* origin_file, const char* origin_func, int origin_line) {
    rdebug("stack info, top = %d, from = [%s:%s:%d]", lua_gettop(L), origin_file, origin_func, origin_line);
    for (int i = 1; i <= lua_gettop(L); i++) {
        //TNONE -1; NIL 0; BOOLEAN 1; LU 2; NUMBER 3; STR 4; TABLE 5; FUNCTION 6; USERDATA 7; THREAD    8; NUMTAGS 9
        rdebug("stack info, [%d: %d -> %p]", i, lua_type(L, i), lua_topointer(L, i));
    }

    return 1;
}

static const rprofiler_mem_data_t* read_object(lua_State *L, rprofiler_mem_data_t* parent_data, 
    int type, const void* p, int level, char* var_key, bool travel_immediately);

// static int get_type(lua_State *L, void* p) {
//     luaL_checkstack(L, LUA_MINSTACK, NULL);

//     int type = lua_type(L, -1);
//     lua_pop(L,1);

//     return type;
// }

static int get_table_size(Table *h, bool fast) {
    // if (fast) {
#if LUA_VERSION_NUM >= 504
        return ((int)sizenode(h) + (int)h->alimit) * (int)sizeof(Node);
#else
        return (int)sizeof(Table) + (int)sizenode(h) * (int)sizeof(Node) + (int)h->sizearray * (int)sizeof(TValue);
#endif
    // } else {
    //     Node *node, *last_node = gnode_last(h);
    //     int size_table = (int)luaH_getn(h) * size_ptr;
    //     for (node = gnode(h, 0); node < last_node; node++) {
    //         if (!ttisnil(gval(node))) {//有几率会崩
    //             size_table += size_ptr;
    //         }
    //     }
    //     return (int)sizeof(Table) + size_table;
    // }
}

static rprofiler_mem_data_t* init_node_data(int type, void* p, int size, char* desc) {
    int size_desc = sizeof(rprofiler_mem_data_t) + rstr_len(desc) + key_len_max;//预留一个desc的位置
    rprofiler_mem_data_t* data = rdata_new_size(size_desc);

    data->size = size;
    data->ref_size = 0;
    data->children = NULL;
    data->parent = NULL;
    data->self = NULL;
    rstr_fmt(data->desc, "%d-%p-%s", size_desc - 1, type, p, desc == NULL ? rstr_empty : desc);
    // rinfo("table = %p, fast size = %d", obj, data->size);
    return data;
}

static int add_node_child_data(rprofiler_mem_data_t* data, rprofiler_mem_data_t* child) {
    if (data == NULL) {
        rerror("data cant be null.");
        return 1;
    }

    // child->ref += 1;

    rprofiler_data_child_t* child_new = rpool_new_data(rprofiler_data_child_t);
    child_new->data = child;
    child_new->next = NULL;

    rprofiler_data_child_t* child_last = data->children;
    if (child_last != NULL) {
        while (child_last->next) {
            child_last = child_last->next;
        }

        child_last->next = child_new;
    } else {
        data->children = child_new;
    }
    return 0;
}

static char* index2string(lua_State *L, int index, char* buffer, size_t size) {
    int t = lua_type(L, index);
    switch (t) {
    case LUA_TSTRING:
        // return lua_tostring(L, index);
        snprintf(buffer, size, "[%s]", lua_tostring(L, index));
        break;
    case LUA_TNUMBER:
        snprintf(buffer, size, "[%lg]", lua_tonumber(L, index));
        break;
    case LUA_TBOOLEAN:
        snprintf(buffer, size, "[%s]", lua_toboolean(L, index) ? "true" : "false");
        break;
    case LUA_TNIL:
        snprintf(buffer, size, "[nil]");
        break;
    case LUA_TTABLE:
        snprintf(buffer, size, "[tb:0x%p]", lua_topointer(L, index));
        break;
    default:
        snprintf(buffer, size, "[%s:0x%p]", lua_typename(L, t), lua_topointer(L, index));
        break;
    }
    return buffer;
}


static int travel_table(lua_State *L, rprofiler_mem_data_t* table_data, Table *h, char* var_key) {
    // rprofiler_mem_data_t* temp_data = NULL;

    if (h->metatable != NULL) {//todo Ray 不一定是，参见const
        read_object(L, table_data, LUA_TTABLE, h->metatable, table_level, "[meta]", false);
    }

    char* var_key_cur;

    dump_lua_stack(L);
    sethvalue(L, L->top, h);
    api_incr_top(L);
    dump_lua_stack(L);

    lua_pushnil(L);//预留给key
    dump_lua_stack(L);

    rprofiler_mem_data_t* data_kv = NULL;
    int type = 0;
    while (lua_next(L, -2) != NULL) {
        dump_lua_stack(L);
        char temp[key_len_max * 2];
        char* key_desc = index2string(L, -2, temp, sizeof(temp));
        type = lua_type(L, -2);
        if (type == LUA_TTABLE) {//todo Ray 其他，table会主动压一次栈
            read_object(L, table_data, type, lua_topointer(L, -2), table_level, key_desc, false);
        } else {
            data_kv = init_node_data(type, table_data, 1, key_desc);//todo Ray 大小，短字符串共享
            // rinfo("ParentTb = %s -> key = %s", var_key, key_desc);
        }

        char* val_desc = index2string(L, -1, temp, sizeof(temp));
        type = lua_type(L, -1);
        if (type == LUA_TTABLE) {//会主动压一次栈
            read_object(L, table_data, type, lua_topointer(L, -1), table_level, val_desc, false);
        } else {
            if (data_kv != NULL) {
                data_kv->size += 1;//
                //snprintf(temp, key_len_max * 2, "[%s] -> %s", key_desc, val_desc);
                rstr_cat(data_kv->desc, val_desc, key_len_max * 2);
            } else {
                data_kv = init_node_data(type, table_data, 1, val_desc);//todo Ray 大小，短字符串共享
            }
            // rinfo("ParentTb = %s -> value = %s", var_key, val_desc);
        }
        lua_pop(L, 1);
        dump_lua_stack(L);

        if (data_kv != NULL) {
            // rdict_add(all_data, data_kv, data_kv);
            if (table_data != NULL) {//table的元素
                add_node_child_data(table_data, data_kv);
            }

            data_kv = NULL;
        }
    }

    rinfo("check end, table = %p", h);
    
    lua_pop(L, 1);//pop key & tb，5.3自动干掉了key！！
    dump_lua_stack(L);

    // Node *n, *limit = gnode_last(h);
    // unsigned int i;
    
// #if LUA_VERSION_NUM >= 504
//     for (i = 0; i < h->alimit; i++) {
// #else
//     for (i = 0; i < h->sizearray; i++) {
// #endif   
//         rnum2str(var_key_cur, i + 1, "[%d]");
//         const TValue *item = &h->array[i];
//         read_object(parent_data, item, table_level, var_key_cur);
//     }

//     for (n = gnode(h, 0); n < limit; n++) {
//         if (n == NULL || gval(n) == NULL || !isvalid_object(NULL, n)) {
//             continue;
//         }

//         if(!ttisnil(gval(n))) {
// #if LUA_VERSION_NUM >= 504
//             const TValue* key = (const TValue *)&(n->u.key_val);
// #else
//             const TValue *key = gkey(n);//union TKey { struct { TValuefields; int next; /* for chaining next node) */ } nk; TValue tvk;}
// #endif 
//             if (key != NULL && ttisstring(key)) {
//                 var_key_cur = getstr(tsvalue(key));
//             } else {
//                 rnum2str(var_key_cur, key, "tk-%p");
//             }
//             read_object(parent_data, key, table_level, var_key_cur);

//             const TValue *value = gval(n);
//             if (value != NULL && ttisstring(value)) {
//                 var_key_cur = getstr(tsvalue(value));
//             } else {
//                 rnum2str(var_key_cur, value, "tv-%p");
//             }
            
//             read_object(parent_data, value, table_level, var_key_cur);
//         }
//     }

    return 0;
}

static const rprofiler_mem_data_t* read_object(lua_State *L, rprofiler_mem_data_t* parent_data, 
    int type, const void* p, int level, char* var_key, bool travel_immediately) {
    rprofiler_mem_data_t* data = NULL;

    rdict_entry_t *de = rdict_find(all_data, p);
    if (de != NULL) {
        data = (rprofiler_mem_data_t*)de->value.ptr;
        add_node_child_data(parent_data, data);
        return data;
    }

    switch(type) {
    case LUA_TTABLE: {
        Table *h = gco2t(p);
        data = init_node_data(type, p, get_table_size(h, false), var_key);

        //data->size = get_table_size(h, false);
        rinfo("table = %p, fast size = %d", p, data->size);

        rdict_add(all_data, p, data);
        if (parent_data != NULL) {
            add_node_child_data(parent_data, data);
        }

        if (travel_immediately) {
            travel_table(L, data, h, var_key);
        } else {
            data->parent = parent_data;
            data->self = h;
            rlist_rpush(travel_list, data);
        }

        break;
    }
    case LUA_TUSERDATA: {
        data = init_node_data(type, p, size_ptr, var_key);//懒得统计实际大小，算一个指针
        rdict_add(all_data, p, data);
        if (parent_data != NULL) {
            add_node_child_data(parent_data, data);
        }

        if (uvalue((TValue*)p)->metatable != NULL) {//light c 不一定是个function
            //read_object(parent_data, uvalue((TValue*)p)->metatable, table_level, "[usd_meta]", false);
        }
        break;
    }
    default:
        rinfo("Unknown type, obj = %p, type = %d, key = %s", p, type, var_key);
        break;
    }

    return data;
}

static int rprofiler(lua_State *L) {
    rpool_init_global();
    if (rget_pool(rprofiler_data_child_t) == NULL) {
        rget_pool(rprofiler_data_child_t) = rcreate_pool(rprofiler_data_child_t);
        rassert(rget_pool(rprofiler_data_child_t) != NULL, "rprofiler_data_child_t pool");
    }

    if (inited == 0) {
        setlocale(LC_ALL, "zh_CN.UTF-8");
        setlocale(LC_NUMERIC, "zh_CN");
        setlocale(LC_TIME, "zh_CN");
        setlocale(LC_COLLATE, "C");//"POSIX"或"C"，则strcoll()与strcmp()作用完全相同

        rmem_init();
        rtools_init();

        rlog_init("${date}/rprofiler_${index}.log", rlog_level_all, false, 100);

        if (all_data == NULL) {
            rdict_init(all_data, rdata_type_uint64, rdata_type_ptr, 20000, 256);
        }
        if (travel_list == NULL) {
            rlist_init(travel_list, rdata_type_ptr);
        }

        inited = 1;

        rinfo("Load rprofiler finished.");
    }
    rdict_clear(all_data);

    const void* root_obj = NULL;
    int type = 0;

    int frame_top = lua_gettop(L);

    if (frame_top > 0) {
        root_obj = lua_topointer(L, 1);
        type = lua_type(L, 1);
    }

    table_level = 1;//0为遍历所有
    if (frame_top > 1) {
        table_level = (int)luaL_checkinteger(L, 2);
    }

    rinfo("Start travelling , root = %p, level = %d", root_obj, table_level);
    
    rprofiler_mem_data_t* root_data = read_object(L, NULL, type, root_obj, table_level, "[root]", true);

    rlist_node_t* table_node = NULL;
    rprofiler_mem_data_t* table_data = NULL;
    while ((table_node = rlist_lpop(travel_list)) != NULL) {
        table_data = table_node->val;
        travel_table(L, table_data->parent, (Table*)table_data->self, table_data->desc);

        rdata_free(rprofiler_mem_data_t, table_data);
        rdata_free(rlist_node_t, table_node);
    }

    rprofiler_mem_data_t* data_cur = NULL;
    rdict_iterator_t it = rdict_it(all_data);
    for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
        data_cur = (rprofiler_mem_data_t*)de->value.ptr;

        rinfo("[%p, %d: %s]", data_cur->parent, data_cur->size, data_cur->desc);
        rprofiler_data_child_t* child = data_cur->children;
        while (child != NULL) {
            rinfo("-[%p, %d: %s]", child->data->parent, child->data->size, child->data->desc);

            child = child->next;
        }
    }
    rdict_clear(all_data);

    rinfo("Stop travelling , root = %p, level = %d", root_obj, table_level);

    rdestroy_pool(rprofiler_data_child_t);
    rpool_uninit_global();

    return 1;
}

R_API int luaopen_rprofiler(lua_State *L) {
    luaL_checkversion(L);
    lua_pushcfunction(L, rprofiler);
    return 1;
}


#undef dump_lua_stack
#undef key_len_max

#ifdef __cplusplus
}
#endif
