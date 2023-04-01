/** 
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
#include "rfile.h"

#ifdef ros_windows
#pragma comment(lib, "luad.lib")
#endif

static rdict_t* all_data = NULL;
static rlist_t* travel_list = NULL;

#define key_len_max 64

static int inited = 0;

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
    int level;
    int size;
    int table_nodes;
    int table_arrays;
    int ref_size;//引用对象size
    rprofiler_data_child_t* children;
    rprofiler_data_child_t* parent;//仅临时当前，可能有多个
    void* self;//仅临时

    char desc[0];
};

rpool_define_global();
rpool_declare(rprofiler_data_child_t);// .h
static rdefine_pool(rprofiler_data_child_t, 10000, 5000); // .c


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

static int get_table_size(Table* h, int* nodes, int* arrays) {
    // if (fast) {
#if LUA_VERSION_NUM >= 504
        return ((int)sizenode(h) + (int)h->alimit) * (int)sizeof(Node);
#else
        *nodes = (int)sizenode(h);
        *arrays = (int)h->sizearray;
        return (int)sizeof(Table) + (*nodes) * (int)sizeof(Node) + (int)h->sizearray * (int)sizeof(TValue);
#endif
    // } else {//不好统计，short string是共享的
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

static rprofiler_mem_data_t* init_node_data(int level, void* p, int size, char* desc) {
    int size_desc = sizeof(rprofiler_mem_data_t) + 2 * key_len_max;//预留一个desc的位置rstr_len(desc) + 
    rprofiler_mem_data_t* data = rdata_new_size(size_desc);

    data->level = level;
    data->size = size;
    data->table_nodes = 0;
    data->table_arrays = 0;
    data->ref_size = 0;
    data->children = NULL;
    data->parent = NULL;
    data->self = NULL;
    // rstr_fmt(data->desc, "%d-%p-%s", size_desc - 1, level, p, desc == NULL ? rstr_empty : desc);
    // rstr_fmt(data->desc, "%s", size_desc - 1, desc == NULL ? rstr_empty : desc);
    data->desc[0] = rstr_end;
    rstr_cat(data->desc, desc, 2 * key_len_max - 1);
    return data;
}

static int add_node_child_data(rprofiler_mem_data_t* data, rprofiler_mem_data_t* child) {
    if (data == NULL) {
        rerror("data cant be null.");
        return 1;
    }
    // rdebug("add child, %p <- %p", data, child);

    // child->ref += 1;

    rprofiler_data_child_t* child_new = rpool_new_data(rprofiler_data_child_t);
    child_new->data = child;
    child_new->next = NULL;

    rprofiler_data_child_t* child_last = data->children;
    if (child_last != NULL) {
        while (child_last->next != NULL) {
            child_last = child_last->next;
        }

        child_last->next = child_new;
    } else {
        data->children = child_new;
    }
    return 0;
}

static char* index2string(lua_State *L, int index, int level, char* prefix, char* buffer, size_t size) {
    prefix = prefix == NULL ? rstr_empty : prefix;

    int t = lua_type(L, index);
    switch (t) {
    case LUA_TSTRING:
        // return lua_tostring(L, index);
        snprintf(buffer, size, "%d|%s[%s]", level, prefix, lua_tostring(L, index));
        break;
    case LUA_TNUMBER:
        snprintf(buffer, size, "%d|%s[%lg]", level, prefix, lua_tonumber(L, index));
        break;
    case LUA_TBOOLEAN:
        snprintf(buffer, size, "%d|%s[%s]", level, prefix, lua_toboolean(L, index) ? "true" : "false");
        break;
    case LUA_TNIL:
        snprintf(buffer, size, "%d|%s[nil]", level, prefix);
        break;
    case LUA_TTABLE:
        snprintf(buffer, size, "%d|%s[tb_%p]", level, prefix, lua_topointer(L, index));
        break;
    default:
        snprintf(buffer, size, "%d|%s[%s_%p]", level, prefix, lua_typename(L, t), lua_topointer(L, index));
        break;
    }
    return buffer;
}

//table_data为h的统计对象
static int travel_table(lua_State *L, rprofiler_mem_data_t* table_data, Table *h, char* var_key, int table_level) {
    // rprofiler_mem_data_t* temp_data = NULL;

    if (h->metatable != NULL) {//不一定是，参见const，内部验证
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
        char temp[key_len_max];
        char temp2[key_len_max];
        char* key_desc = index2string(L, -2, table_level, NULL, temp, sizeof(temp) - 1);
        type = lua_type(L, -2);
        if (type == LUA_TTABLE) {//todo Ray 其他，table会主动压一次栈
            read_object(L, table_data, type, lua_topointer(L, -2), table_level, key_desc, false);
        } else {
            // data_kv = init_node_data(table_level, table_data, 1, key_desc);//todo Ray 大小，短字符串共享
            // rinfo("ParentTb = %s -> key = %s", var_key, key_desc);
        }

        rstr_cat(key_desc, " => ", 0);

        char* val_desc = index2string(L, -1, table_level, key_desc + 2, temp2, sizeof(temp2) - 1);
        type = lua_type(L, -1);
        if (type == LUA_TTABLE) {//会主动压一次栈
            read_object(L, table_data, type, lua_topointer(L, -1), table_level, val_desc, false);
        } else {
            if (data_kv != NULL) {
                data_kv->size += 1;//
                //snprintf(temp2, key_len_max * 2, "[%s] -> %s", key_desc, val_desc);
                // rstr_cat(data_kv->desc, val_desc, rstr_len(data_kv->desc) + key_len_max);
            } else {
                data_kv = init_node_data(table_level, table_data, 1, val_desc);//todo Ray 大小，短字符串共享
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

    // rinfo("check end, level = %d, table = %p", table_level, h);
    
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

    // rinfo("read_object, level = %d, obj = %p, type = %d, key = %s, go = %d", level, p, type, var_key, travel_immediately);

    int mem_size = 0;
    int nodes = 0;
    int arrays = 0;

    switch(type) {
    case LUA_TTABLE: {
        Table *h = gco2t(p);

        mem_size = get_table_size(h, &nodes, &arrays);

        data = init_node_data(level, p, mem_size, var_key);
        data->table_nodes = nodes;
        data->table_arrays = arrays;

        rdict_add(all_data, p, data);
        if (parent_data != NULL) {
            add_node_child_data(parent_data, data);
        }

        if (travel_immediately) {
            travel_table(L, data, h, var_key, level);
        } else {
            data->parent = parent_data;
            data->self = h;
            rlist_rpush(travel_list, data);
        }

        break;
    }
    case LUA_TUSERDATA: {
        data = init_node_data(level, p, size_ptr, var_key);//懒得统计实际大小，算一个指针
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
        rinfo("Unknown type, level = %d, obj = %p, type = %d, key = %s", level, p, type, var_key);
        break;
    }

    return data;
}

static int write_json_item(rprofiler_mem_data_t* data, rfile_item_t* file_item) {
    int total_size = 0;
    char temp[4096];
    int real_size = 0;
    int last_level = 0;

    rprofiler_mem_data_t* data_cur = data;
    if (data_cur != NULL) {
        total_size += data_cur->size;
        last_level = data_cur->level;

        // rinfo(" {\"ptr\":\"%p\", \"size\":\"%d#%d#%d\", \"desc\": \"%s\"}", data_cur, data_cur->size, data_cur->table_nodes, data_cur->table_arrays, data_cur->desc);
        rstr_fmt(temp, "\"%p\":{\"size\":\"%d#%d#%d\", \"desc\": \"%s\"", sizeof(temp), data_cur, data_cur->size, data_cur->table_nodes, data_cur->table_arrays, data_cur->desc);
        rfile_write(file_item, temp, rstr_len(temp), &real_size);

        if (data_cur->children != NULL) {
            rfile_write(file_item, ", nodes = { ", 0, &real_size);
        }

        rprofiler_data_child_t* child = data_cur->children;
        while (child != NULL && child->data != NULL) {
            // rinfo("%d_%p -> [child=%p] %d_%s", last_level, data_cur, child->data, child->data->level, child->data->desc);
            if (last_level <= child->data->level) {
                total_size += write_json_item(child->data, file_item);
            }

            child = child->next;
        }

        if (data_cur->children != NULL) {
            rfile_write(file_item, " } ", 0, &real_size);
        }

        rfile_write(file_item, "}\n", 0, &real_size);

        rfile_flush(file_item);
    }

    return total_size;
}

static int output_json(rprofiler_mem_data_t* root_data) {
    if (root_data == NULL) {
        return 0;
    }

    const char* filename = "./rmem_info.json";
    if (rfile_exists(filename)) {
        rfile_remove(filename);
    }

    rfile_item_t* file_item = NULL;
    rfile_init_item(&file_item, filename);

    rfile_open(file_item, rfile_open_mode_overwrite, false);//覆盖模式，文件不要占用

    int total_size = 0;
    int real_size = 0;

    rfile_write(file_item, "{\n", 0, &real_size);
    total_size = write_json_item(root_data, file_item);
    rfile_write(file_item, "}", 0, &real_size);

    rfile_uninit_item(file_item);

    return total_size;
}

static int rprofiler(lua_State *L) {
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

    rpool_init_global();
    if (rget_pool(rprofiler_data_child_t) == NULL) {
        rget_pool(rprofiler_data_child_t) = rcreate_pool(rprofiler_data_child_t);
        rassert(rget_pool(rprofiler_data_child_t) != NULL, "rprofiler_data_child_t pool");
    }

    const void* root_obj = NULL;
    int type = 0;

    int frame_top = lua_gettop(L);

    if (frame_top > 0) {
        root_obj = lua_topointer(L, 1);
        type = lua_type(L, 1);
    }

    int table_level_max = 1;//0为遍历所有
    if (frame_top > 1) {
        table_level_max = (int)luaL_checkinteger(L, 2);
    }

    rinfo("Start travelling , root = %p, level = %d", root_obj, table_level_max);
    
    rprofiler_mem_data_t* root_data = read_object(L, NULL, type, root_obj, 0, "[root]", true);

    rlist_node_t* table_node = NULL;
    rprofiler_mem_data_t* table_data = NULL;
    int level_size = rlist_size(travel_list);
    int level_cur = 1;
    int total_count = 1;
    while ((table_node = rlist_lpop(travel_list)) != NULL) {
        table_data = table_node->val;
        travel_table(L, table_data, (Table*)table_data->self, table_data->desc, level_cur);

        if (total_count++ % 1000 == 0) {
            rinfo("read objects, count = %d", total_count);
        }

        // rdata_free(rprofiler_mem_data_t, table_data);
        rdata_free(rlist_node_t, table_node);

        if (--level_size == 0) {
            level_size = rlist_size(travel_list);

            if (table_level_max > 0 && ++level_cur > table_level_max) {
                break;
            }
        }
    }

    
    int total_size = 0;
    // rprofiler_mem_data_t* data_cur = NULL;
    // rdict_iterator_t it = rdict_it(all_data);
    // for (rdict_entry_t *de = NULL; (de = rdict_next(&it)) != NULL; ) {
    //     data_cur = (rprofiler_mem_data_t*)de->value.ptr;//parent可能有多个
    //     total_size += data_cur->size;
    //     rinfo("[%p, %d#%d#%d: %s]\n", data_cur, data_cur->size, data_cur->table_nodes, data_cur->table_arrays, data_cur->desc);
    //     rprofiler_data_child_t* child = data_cur->children;
    //     while (child != NULL && child->data != NULL) {
    //         rinfo("-[%d: %s]", child->data->size, child->data->desc);
    //         child = child->next;
    //     }
    // }

    total_size = output_json(root_data);

    rdict_clear(all_data);

    rinfo("Stop travelling , root = %p, data = %p, level = %d, table_amount = %d, total_size = %d", 
        root_obj, root_data, table_level_max, total_count, total_size);

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
