/** 
 * Copyright (c) 2016
 *
 * This library is free software { you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "lauxlib.h"
#include "lualib.h"
#include "lua.h"
    
#include "rlog.h"

#include "rscript_context.h"
#include "rscript.h"

#define dump_lua_stack(L) dump_stack(L)

static lua_State* L = NULL;

static int dump_stack(lua_State *L) {
    rdebug("stack info, top = %d", lua_gettop(L));
    for (int i = 1; i <= lua_gettop(L); i++) {
        //TNONE -1; NIL 0; BOOLEAN 1; LU 2; NUMBER 3; STR 4; TABLE 5; FUNCTION 6; USERDATA 7; THREAD	8; NUMTAGS 9
        rdebug("stack info, [%d: %d -> %p]", i, lua_type(L, i), lua_topointer(L, i));
    }

    return 1;
}

static bool check_result(lua_State* L, int status) {
    if (status == LUA_OK) {
        return true;
    }

    const char* msg = lua_tostring(L, -1);
    if (msg == NULL) {
        msg = "catch error, but msg is not as string.";
    }

    rerror("[script]: error on calling lua, msg = %s", msg);
    
    lua_pop(L, 1);  /* remove msg */

    return false;
}

static bool _load_3rd(lua_State* L) {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");

    // lua_pushcfunction(L, luaopen_pb);
    // lua_setfield(L, -2, "pb");

    // lua_pushcfunction(L, luaopen_pb_io);
    // lua_setfield(L, -2, "pb.io");

    lua_settop(L, 0);

    return true;
}

//获取脚本function地址，如：xxx.yyy:zzz / xxx.yyy.zzz
static int set_script_func(lua_State* L, const char* func_name, int frame_top) {
    int status = -1;
    bool result = false;

    int len = 0;
    bool method = false;
    //char char_tmp = rstr_end;
    char* p = NULL;
    char* q = NULL;
    char temp_name[64];

    //int full_len = rstr_len(func_name);
    int func_top = lua_gettop(L);

    //if (unlikely(full_len == 0 || full_len > 63)) {
    //    rerror("[script]: invalid function name, len = %d", full_len);
    //    rgoto(0);
    //}

    for (p = q = func_name; len < 64; ++p) {
        switch (*p) {
            case '.':
                //char_tmp = *p;
                //*p = rstr_end;
                break;
            case ':':
                //char_tmp = *p;
                //*p = rstr_end;
                method = true;
                break;
            case '\0':
                break;
            case ' ':
                //rerror("invalid of blanks. func_name = %s", func_name);
                //rgoto(0);
                continue;
            default:
                temp_name[len++] = *p;

                continue;
        }

        temp_name[len] = rstr_end;
        len = 0;

        if (q == func_name) {
            lua_getglobal(L, temp_name);
        } else {
            lua_getfield(L, -1, temp_name);
        }

        if (lua_isnil(L, -1)) {
            //rerror("invalid part of [%s]. full = %s", q, func_name);
            rerror("invalid part of [%s]. full = %s", temp_name, func_name);
            rgoto(0);
        }

        if (*p == rstr_end) {
            break;
        }

        //*p = char_tmp;
        q = p + 1;
    }
    
    if (method) {
        //lua_insert(L, func_top + 1);//-1); // //lua_rotate(L, (idx), 1)
        //lua_insert(L, func_top + 2);//-2); //
        lua_rotate(L, -1, (lua_gettop(L) - frame_top - 2));//self,func -> 1,2
        lua_rotate(L, frame_top + 2, 1);//func -> 1
        lua_settop(L, func_top + 2);
    } else {
        dump_lua_stack(L);
        //lua_insert(L, func_top + 1);
        lua_rotate(L, -1, func_top + 1, 1);
        dump_lua_stack(L);
        lua_settop(L, func_top + 1);
    }
    
    //if (!lua_isfunction(L, 1)) {
    //    rerror("[%s] is not a function. full = %s", temp_name, func_name);
    //    rgoto(0);
    //}

    result = true;

exit0:
    if (!result) {
        lua_settop(L, func_top);
    }

    return result ? rcode_ok : rcode_invalid;
}

//快捷入口只支持最多一层table，如：func_name / table_name.func_name / table_name:func_name
static int get_script_table_func(lua_State* L, const char* table_name, const char* func_name) {
    int status = -1;
    bool result = false;

    int top_last = lua_gettop(L);

    if (unlikely(func_name == NULL)) {
        rerror("[script]: function can not be NULL.");
        rgoto(0);
    }

    if (likely(table_name != NULL)) {
        lua_getglobal(L, table_name);

        if (unlikely(!lua_istable(L, -1))) {
            rerror("[script]: table [%s] not found.", table_name);
            rgoto(0);
        }

        lua_getfield(L, -1, func_name);
    } else {
        // lua_pushcfunction(L, &before_update_lua);
        lua_getglobal(L, func_name);

        if (unlikely(!lua_isfunction(L, -1))) {
            rerror("[script]: function [%s] not found.", func_name);
            rgoto(0);
        }
    }

    lua_pushstring(L, "from c calling...");

    lua_settop(L, top_last + 2);
    // lua_pushinteger(L, 1);
    status = lua_pcall(L, 1, 1, 0); //int nargs, int nresults, int errfunc,

    if (!check_result(L, status)) {
        rerror("[script]: error on invoking [%s].", func_name);

        rgoto(0);
    }

    result = lua_toboolean(L, -1);

exit0:
    if (!result) {
        lua_settop(L, top_last);
    }

    return (result && status == LUA_OK) ? rcode_ok : rcode_invalid;
}

static int call_script_func(lua_State* L, char* func_name, int params_amount, int ret_amouont) {
    int status = -1;
    bool result = false;

    int frame_top = lua_gettop(L);

    lua_pushstring(L, "from c calling...");
    lua_pushstring(L, "from c 222");

    status = set_script_func(L, func_name, frame_top);
    if (status != rcode_ok) {
        rgoto(0);
    }

    rdebug("invoke [%s], param_cnt=%d, ret_cnt=%d, stack frame_top = %d, cur_top = %d", 
        func_name, params_amount, ret_amouont, frame_top, lua_gettop(L));
    dump_lua_stack(L);

    //lua_settop(L, lua_gettop(L) + 2);
    // lua_pushinteger(L, 1);
    status = lua_pcall(L, params_amount, ret_amouont, 0); //int nargs, int nresults, int errfunc,

    if (!check_result(L, status)) {
        rerror("[script]: error on invoking [%s]", func_name);

        rgoto(0);
    }

    result = true;// lua_toboolean(L, -1);
    //lua_pop(L, ret_amouont);//清掉返回

exit0:
    if (!result) {
        lua_settop(L, frame_top);
    }

    return result ? rcode_ok : rcode_invalid;
}


static int init_lua(rscript_context_t* ctx, const void* cfg_data) {
    int status = 0;
    
    L = luaL_newstate();

    rassert(L != NULL, "new lua L failed.");

    luaL_openlibs(L);

    _load_3rd(L);

    status = luaL_dofile(L, "./script/main.lua");
    // luaL_loadfile(L, "script/main.lua");
    // int ret_code = lua_pcall(L, 0, 0, 0);

    if (check_result(L, status)) {
        rinfo("system init finished.");
        return rcode_ok;
    }

    rerror("system init failed.");

    return rcode_invalid;
}

static int uninit_lua(rscript_context_t* ctx, const void* cfg_data) {

    lua_close(L);

    rinfo("system uninit finished.");

    return rcode_ok;
}

static int before_update_lua(rscript_context_t* ctx, const void* cfg_data) {

    return rcode_ok;
}

static int update_lua(rscript_context_t* ctx, const void* cfg_data) {
    int ret_code = 0;

    char* func_name = "LogErr";
    ret_code = call_script_func(L, func_name, 2, 1);
    if (ret_code != rcode_ok) {
        rgoto(0);
    }
    lua_pop(L, 1);

    func_name = "Util.LogErr";
    ret_code = call_script_func(L, func_name, 2, 2);
    if (ret_code != rcode_ok) {
        rgoto(0);
    }
    lua_pop(L, 2);

    func_name = "Util.Log:LogErr";
    ret_code = call_script_func(L, func_name, 3, 2);//self
    if (ret_code != rcode_ok) {
        rgoto(0);
    }
    lua_pop(L, 2);

exit0:

    return ret_code;
}

static int late_update_lua(rscript_context_t* ctx, const void* cfg_data) {

    return rcode_ok;
}

static int update_fixed_lua(rscript_context_t* ctx, const void* cfg_data) {

    return rcode_ok;
}

static rscript_t rscript_lua_obj = {
    .type = rscript_type_lua,// rscript_type_t
    .init = init_lua,//rscript_init
    .uninit = uninit_lua,//rscript_uninit
    .name = "lua_script_system"// char name[0]
};

const rscript_t* rscript_lua = &rscript_lua_obj;


#ifdef __cplusplus
}
#endif
