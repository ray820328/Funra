/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include <locale.h>
#include <string.h>

#include "rlog.h"
#include "rbase/ecs/test/rtest.h"

#include "rlist.h"
#include "rstring.h"
#include "rfile.h"
#include "rtools.h"

static rlist_t *test_entries = NULL;

int rtest_add_test_entry(rtest_entry_type entry_func) {
    rlist_rpush(test_entries, entry_func);

	return rcode_ok;
}

static int init_platform();
static int run_tests(int output);

static int init_platform() {

    rlist_init(test_entries, rdata_type_ptr);

    return rcode_ok;
}

int main(int argc, char **argv) {

    setlocale(LC_ALL, "zh_CN.UTF-8");
    setlocale(LC_NUMERIC, "zh_CN");
    setlocale(LC_TIME, "zh_CN");
    setlocale(LC_COLLATE, "C");//"POSIX"或"C"，则strcoll()与strcmp()作用完全相同

    rmem_init();
    rtools_init();

    rlog_init("${date}/rtest_ecs_${index}.log", rlog_level_all, false, 100);

    char* exe_root = rdir_get_exe_root();
    rinfo("\n框架版本号: %s\n当前路径: %s", rversion_get(), exe_root);
    rstr_free(exe_root);

    init_platform();

    run_tests(0);
    // switch (argc) {
    // case 1: return run_tests(0);
    // default:
    //   fprintf(stderr, "Too many arguments.\n");
    //   fflush(stderr);
    // }

    rlog_uninit();
    rtools_uninit();
    rmem_uninit();

#ifndef __SUNPRO_C
    return rcode_ok;
#endif
}

static int run_tests(int output) {
    int ret_code;

    rtest_add_test_entry(run_rentity_tests);
    rtest_add_test_entry(run_recs_tests);

    rlist_iterator_t it = rlist_it(test_entries, rlist_dir_tail);
    rlist_node_t *node = NULL;
    while ((node = rlist_next(&it))) {
       ret_code = ((rtest_entry_type)(node->val))(output);
       if (ret_code != rcode_ok) {
           break;
       }
    }

    rdata_destroy(test_entries, rlist_destroy);

    return ret_code;
}


recs_cmp_t* rtest_recs_cmp_new(recs_context_t* ctx, recs_cmp_type_t data_type) {
    int ret_code = rcode_ok;
    recs_cmp_t* data = NULL;

    if (ctx == NULL) {
        rwarn("invalid params.");
        rgoto(1);
    }

    // rinfo("recs_cmp_type = %d", data_type);
    switch (data_type) {
    case recs_ctype_rtest01:
        data = (recs_cmp_t*)rdata_new(rtest_cmp_t);
        break;
    default:
        rerror("not defined, data_type = %d", data_type);
        break;
    }

exit1:
    return data;
}
