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
#include "rscript_lua.h"

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
        rdebug("method start, frame_top = %d, func_top = %d", frame_top, func_top);
        dump_lua_stack(L);
        lua_rotate(L, frame_top + 1, 2);//self,func -> 1,2
        dump_lua_stack(L);
        //lua_rotate(L, frame_top + 2, 1);//func -> 1
        dump_lua_stack(L);
        lua_settop(L, func_top + 2);
    } else {
        rdebug("global function start, frame_top = %d, func_top = %d", frame_top, func_top);
        dump_lua_stack(L);
        //lua_insert(L, frame_top + 1);
        //lua_rotate(L, -1, func_top);//func_top = 2, 346->364
        //lua_rotate(L, -1, 1);//346->346
        lua_rotate(L, frame_top + 1, 1);
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

static int call_script_lua(lua_State* L, char* func_name, int params_amount, int ret_amouont) {
    int status = -1;
    bool result = false;
    
    int frame_top = lua_gettop(L) - params_amount;

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

    dump_lua_stack(L);
    return result ? rcode_ok : rcode_invalid;
}


static int init_lua(rscript_context_t* ctx, const void* cfg_data) {
    int status = 0;
    
    rscript_context_lua_t* ctx_script = rdata_new(rscript_context_lua_t);
    rdata_init(ctx_script, sizeof(rscript_context_lua_t));
    ctx->ctx_script = ctx_script;

    lua_State* L = luaL_newstate();

    rassert(L != NULL, "new lua L failed.");

    ctx_script->L = L;

    luaL_openlibs(L);

    _load_3rd(L);

    status = luaL_dofile(L, "./script/main.lua");
    // luaL_loadfile(L, "script/main.lua");
    // int ret_code = lua_pcall(L, 0, 0, 0);

    if (check_result(L, status)) {
        rinfo("lua init finished.");
        return rcode_ok;
    }

    rerror("lua init failed.");

    return rcode_invalid;
}

static int uninit_lua(rscript_context_t* ctx, const void* cfg_data) {
    rscript_context_lua_t* ctx_script = ctx->ctx_script;

    if (ctx_script == NULL) {
        rerror("invalid context.");
        return rcode_invalid;
    }

    if (ctx_script->all_states != NULL) {
        rarray_iterator_t it = rarray_it(ctx_script->all_states);
        for (lua_State* L = NULL; rarray_has_next(&it); ) {
            L = rarray_next(&it);
            lua_close(L);
        }
        ctx_script->L = NULL;
    }

    if (ctx_script->L) {
        lua_close(ctx_script->L);
    }

    rdata_free(rscript_context_lua_t, ctx_script);

    rdata_free(rscript_context_t, ctx);

    rinfo("lua uninit finished.");

    return rcode_ok;
}

static int rscript_call_lua(rscript_context_t* ctx, char* func_name, int params, int rets) {

    int ret_code = call_script_lua(((rscript_context_lua_t*)ctx->ctx_script)->L, func_name, params, rets);

    return ret_code;
}

static rstr_t* rscript_dump_lua(rscript_context_t* ctx) {
    if (ctx == NULL || ctx->ctx_script == NULL) {
        rerror("invalid context.");
        return rstr_empty;
    }
    rscript_context_lua_t* ctx_lua = (rscript_context_lua_t*)ctx->ctx_script;

    dump_stack(ctx_lua->L);

    return rstr_empty;
}

static rscript_t rscript_lua_obj = {
    .type = rscript_type_lua,// rscript_type_t

    .init = init_lua,//rscript_init_func
    .uninit = uninit_lua,//rscript_uninit_func
    .call_script = rscript_call_lua,//rscript_call_func
    .dump = rscript_dump_lua,//rscript_dump_stack_func

    .name = "lua_script_system"// char name[0]
};

const rscript_t* rscript_lua = &rscript_lua_obj;


#ifdef __cplusplus
}
#endif
