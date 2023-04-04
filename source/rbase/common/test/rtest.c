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
#include <signal.h>

#include "rmemory.h"
#include "rlog.h"
#include "rbase/common/test/rtest.h"

#include "rtime.h"
#include "rlist.h"
#include "rstring.h"
#include "rfile.h"
#include "rtools.h"

#if defined(ros_windows)
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

#else

#include <sys/resource.h>
#endif

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

/** 禁止直接或间接调用写日志或使用锁（触发一般源于内核中断线程）*/
static void handle_signal(int signal) {
    fprintf(stdout, "handle signal, code = %d\n", signal);
    if (signal == SIGABRT) {
        fprintf(stderr, "code = %d, handle abort.\n", signal);
    }
}

#ifdef ros_windows

static void create_dump_file(LPCSTR lpstrDumpFilePathName, EXCEPTION_POINTERS* pException) {
    HANDLE hDumpFile = CreateFile(lpstrDumpFilePathName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
    dumpInfo.ExceptionPointers = pException;
    dumpInfo.ThreadId = GetCurrentThreadId();
    dumpInfo.ClientPointers = TRUE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
    CloseHandle(hDumpFile);
}

static LONG WINAPI handle_crash(struct _EXCEPTION_POINTERS* ExceptionInfo) {
    create_dump_file("core.dmp", ExceptionInfo);
    return EXCEPTION_EXECUTE_HANDLER;
}

#endif //ros_windows

int main(int argc, char **argv) {
    setlocale(LC_ALL, "zh_CN.UTF-8");
    setlocale(LC_NUMERIC, "zh_CN");
    setlocale(LC_TIME, "zh_CN");
    setlocale(LC_COLLATE, "C");//"POSIX"或"C"，则strcoll()与strcmp()作用完全相同

    rmem_init();
    rtools_init();

#if defined(ros_windows)
    _set_error_mode(_OUT_TO_MSGBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    signal(SIGABRT, handle_signal);

    SetUnhandledExceptionFilter(handle_crash);

    wchar_t local_time[100];
    time_t t = time(NULL);
    wcsftime(local_time, 100, L"%A %c", localtime(&t));
    wprintf(L"PI: %.2f\n当前时间: %Ls\n", 3.14, local_time);
#elif defined(ros_linux)
    signal(SIGPIPE, SIG_IGN);
    signal(SIGSYS, SIG_IGN);
    signal(SIGTERM, handle_signal);  // when terminal by kill, call OnQuitSignal
    signal(SIGINT, handle_signal);   // when Ctrl+C, call OnQuitSignal

    struct rlimit rlimit_data;
    rlimit_data.rlim_cur = -1;
    rlimit_data.rlim_max = -1;
    setrlimit(RLIMIT_CORE, &rlimit_data);
#endif //_WIN64

    rlog_init("${date}/rtest_common_${index}.log", rlog_level_all, false, 100);

    rinfo("STDC_VERSION = %ld, STDC_IEC_599 = %d", STDC_VERSION, STDC_IEC_599);

    char* exe_root = rdir_get_exe_root();
    rinfo("当前路径: %s", exe_root);
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

    rtest_add_test_entry(run_rtime_tests);
    rtest_add_test_entry(run_rbuffer_tests);
    rtest_add_test_entry(run_rpool_tests);
    rtest_add_test_entry(run_rstring_tests);
    rtest_add_test_entry(run_rthread_tests);
    rtest_add_test_entry(run_rarray_tests);
	rtest_add_test_entry(run_rcommon_tests);
	rtest_add_test_entry(run_rdict_tests);
    rtest_add_test_entry(run_rlog_tests);
    rtest_add_test_entry(run_rfile_tests);
    rtest_add_test_entry(run_rtools_tests);

    ret_code = 0;

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

