﻿/** 
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

#include "iconv.h"

#include "lauxlib.h"
#include "lualib.h"
#include "lua.h"
#include "lobject.h"
#include "lstate.h"

#include "pb.h"

#include "rlog.h"
#include "rfile.h"

#include "rscript_context.h"
#include "rscript.h"
#include "rscript_lua.h"

//lua-protobuf
int luaopen_pb(lua_State *L);
int luaopen_pb_io(lua_State *L);
int luaopen_pb_conv(lua_State *L);
int luaopen_pb_buffer(lua_State *L);
int luaopen_pb_slice(lua_State *L);

#define dump_lua_stack(L) dump_stack(L, get_filename(__FILE__), __FUNCTION__, __LINE__)

static int dump_stack(lua_State *L, char* origin_file, const char* origin_func, int origin_line) {
    rdebug("stack info, top = %d, from = [%s:%s:%d]", lua_gettop(L), origin_file, origin_func, origin_line);
    int type = 0;
    for (int i = 1; i <= lua_gettop(L); i++) {
        //TNONE -1; NIL 0; BOOLEAN 1; LU 2; NUMBER 3; STR 4; TABLE 5; FUNCTION 6; USERDATA 7; THREAD	8; NUMTAGS 9
        type = lua_type(L, i);
        rdebug("stack info, [%d: %d -> %p]", i, type, lua_topointer(L, i));
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

    lua_pushcfunction(L, luaopen_pb);
    lua_setfield(L, -2, "pb");
    lua_pushcfunction(L, luaopen_pb_io);
    lua_setfield(L, -2, "pb.io");
    lua_pushcfunction(L, luaopen_pb_conv);
    lua_setfield(L, -2, "pb.conv");
    lua_pushcfunction(L, luaopen_pb_buffer);
    lua_setfield(L, -2, "pb.buffer");
    lua_pushcfunction(L, luaopen_pb_slice);
    lua_setfield(L, -2, "pb.slice");

    lua_settop(L, 0);

    return true;
}

static int lua_log(lua_State* L) {
    int ret_code = 0;

    int log_level = 0;
    int param_amount = 0;
    //char content[1024];//1k以上继续拼参数
    char* content;
    char* filename = NULL;
    int line = 0;

    //rstr_init(content);

    int frame_top = lua_gettop(L);
    //dump_lua_stack(L);
    if (frame_top > 2) {
        log_level = (int)luaL_checkinteger(L, 1);
        if unlikely(log_level >= rlog_level_all) {
            rerror("invalid level, log_level = %d", log_level);
            return 0;
        }

        //filename = luaL_checkstring(L, 2);
        //line = (int)luaL_checkinteger(L, 3);
        param_amount = (int)luaL_checkinteger(L, 2);
    } else {
        rerror("invalid parameters, count = %d", frame_top);
        return 0;
    }

    lua_Debug ar;
    if (lua_getstack(L, 2, &ar)) {  /* check function at level */
        lua_getinfo(L, "Sl", &ar);  /* get info about it */
        if (ar.currentline > 0) {  /* is there info? */
            filename = ar.short_src;
            line = ar.currentline;
        } else {
            filename = ar.what; // 'Lua', 'C', 'main', 'tail' （"main": 代码块的主体部分; 'tail': 尾调用）
            //namewhat = ar.namewhat; // 'global', 'local', 'field', 'method', "upvalue", 或是 ""（Lua 用空串表示其它选项都不符合。）
            line = -1;
        }
    } else {
        filename = "NULL";
        line = -1;
    }
    
    if (param_amount > 0) {
        content = luaL_checkstring(L, 3);

        //iconv_t code_opt = iconv_open("CP65001", "UCS4BE");//"UTF-8//IGNORE", "GB18030" = GB2312 -> utf8
        //int code_result = iconv(code_opt, &content, &content_len, &content_dest, &content_dest_len);
        //if (code_result < 0) {
        //    rerror("error on code changing. result = %d", code_result);
        //}
        //iconv_close(code_opt);

    } else {
        content = rstr_empty;
    }
    //for (int index = 3; index < param_amount; index++) {

    //}

    rlog_printf(NULL, log_level, "[%ld] %s:%d %s\n", rthread_cur_id(), filename, line, content);

    return ret_code;
}
static int lua_get_exe_root(lua_State* L) {
    int ret_code = 1;
    
    char* root = rdir_get_exe_root();
    lua_pushstring(L, root);
    rstr_free(root);

    return ret_code;
}

const struct luaL_Reg funra_funcs[] = {
    {"Log", lua_log},
    {"GetWorkRoot", lua_get_exe_root},
    {NULL, NULL},
};

static int _load_funra(lua_State* L) {
    lua_createtable(L, 0, sizeof(funra_funcs) / sizeof((funra_funcs)[0]) - 1);
    luaL_setfuncs(L, funra_funcs, 0);
    lua_setglobal(L, "funra");

#if defined(ros_windows)
    lua_pushstring(L, "Windows");
#elif defined(ros_linux)
    lua_pushstring(L, "Linux");
#elif defined(ros_darwin)
    lua_pushstring(L, "Darwin");
#else
    lua_pushstring(L, "UnknownOS");
#endif
    lua_setglobal(L, "ROS_TYPE");

    return 1;
}

//获取脚本function地址，如：xxx.yyy:zzz / xxx.yyy.zzz，不支持Coroutine
static int set_script_func(lua_State* L, char* func_name, int frame_top, int* top_add) {
    // int status = -1;
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
        lua_pushnil(L);
        lua_copy(L, -3, -1);
        //rdebug("method start, frame_top = %d, func_top = %d", frame_top, func_top);
        //dump_lua_stack(L);
        //lua_rotate(L, frame_top + 1, 2);//self,func -> 1,2
        lua_rotate(L, frame_top + 1, 3);
        lua_settop(L, func_top + 3);

        *top_add = 1;

        if (!lua_isfunction(L, frame_top + 2)) {
            rerror("[%s] is not a function. full = %s", temp_name, func_name);
            rgoto(0);
        }
    } else {
        //rdebug("global function start, frame_top = %d, func_top = %d", frame_top, func_top);
        //dump_lua_stack(L);
        lua_rotate(L, frame_top + 1, 1);
        lua_settop(L, func_top + 1);

        if (!lua_isfunction(L, frame_top + 1)) {
            rerror("[%s] is not a function. full = %s", temp_name, func_name);
            rgoto(0);
        }
    }
    
    result = true;

exit0:
    if (!result) {
        lua_settop(L, func_top);
    }

    return result ? rcode_ok : rcode_invalid;
}

//快捷入口只支持最多一层table，如：func_name / table_name.func_name / table_name:func_name
//static int fast_set_script_func(lua_State* L, const char* table_name, const char* func_name, int frame_top, int* top_add) { }

static int call_script_lua(lua_State* L, char* func_name, int params_amount, int ret_amouont) {
    int status = -1;
    bool result = false;
    
    int frame_top = lua_gettop(L) - params_amount;
    int top_add  = 0;

    status = set_script_func(L, func_name, frame_top, &top_add);
    if (status != rcode_ok) {
        rgoto(0);
    }

    //rdebug("invoke [%s], param_cnt=%d, ret_cnt=%d, stack frame_top = %d, top_add = %d, cur_top = %d", 
    //    func_name, params_amount, ret_amouont, frame_top, top_add, lua_gettop(L));
    //dump_lua_stack(L);

    status = lua_pcall(L, params_amount + top_add, ret_amouont, 0); //int nargs, int nresults, int errfunc,

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

    //dump_lua_stack(L);
    return result ? rcode_ok : rcode_invalid;
}


static int init_lua(rscript_context_t* ctx, const void* cfg_data) {
    int status = 0;
    
    rscript_lua_cfg_t* cfg = (rscript_lua_cfg_t*) cfg_data;
    rassert(cfg != NULL, "get cfg failed.");

    rscript_context_lua_t* ctx_script = rdata_new(rscript_context_lua_t);
    rdata_init(ctx_script, sizeof(rscript_context_lua_t));
    ctx->ctx_script = ctx_script;

    lua_State* L = luaL_newstate();

    rassert(L != NULL, "new lua L failed.");

    ctx_script->L = L;

    luaL_openlibs(L);

    _load_3rd(L);

    _load_funra(L);

    rinfo("lua env init finished.");

    status = luaL_dofile(L, cfg->entry ? cfg->entry : "./script/main.lua");
    // luaL_loadfile(L, cfg->entry);
    // int ret_code = lua_pcall(L, 0, 0, 0);

    if (check_result(L, status)) {
        rinfo("load lua file finished.");
        return rcode_ok;
    }

    rerror("load lua file failed.");

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

    //rdata_free(rscript_context_t, ctx);

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

    dump_lua_stack(ctx_lua->L);//dump_stack

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


#undef dump_lua_stack

#ifdef __cplusplus
}
#endif
