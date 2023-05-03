/** 
 * Copyright (c) 2014 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

static const long long NANOSECOND_PER_SECOND = 1000000000LL;
//static const long long NANOSECOND_PER_MILLISECOND = 1000000LL;

#define UTC_BASE_YEAR   1970
#define MONTH_PER_YEAR  12
#define DAY_PER_YEAR    365
#define SEC_PER_DAY     86400
#define SEC_PER_HOUR    3600
#define SEC_PER_MIN     60

 /* 每个月的天数 */
static unsigned char g_day_per_mon[MONTH_PER_YEAR] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

// #include <lua5.3/lua.h>
// #include <lua5.3/lauxlib.h>
// #include <lua5.3/lualib.h>
#include <stdint.h>
#include <time.h>

#include "rtime.h"

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif
#ifdef WIN32
//int gettimeofdayfix(struct timeval *tp, void *tzp) //todo Ray 时区，和其他第三方库命名冲突
//{
//  time_t clock;
//  struct tm tm;
//  SYSTEMTIME wtm;
//  GetLocalTime(&wtm);
//  tm.tm_year = wtm.wYear - 1900;
//  tm.tm_mon = wtm.wMonth - 1;
//  tm.tm_mday = wtm.wDay;
//  tm.tm_hour = wtm.wHour;
//  tm.tm_min = wtm.wMinute;
//  tm.tm_sec = wtm.wSecond;
//  tm.tm_isdst = -1;
//  clock = mktime(&tm);
//  tp->tv_sec = (long)clock;
//  tp->tv_usec = wtm.wMilliseconds * 1000;
//  return (0);
//}

int rtime_gettimeofdayfix(struct timeval * tp, struct timezone * tzp) {
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970 
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    //uint64_t    time;
    ULARGE_INTEGER ularge;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);

    //time = ((uint64_t)file_time.dwLowDateTime);
    //time += ((uint64_t)file_time.dwHighDateTime) << 32;
    //tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    //tp->tv_usec = (long)(system_time.wMilliseconds * 1000);

    ularge.LowPart = file_time.dwLowDateTime;
    ularge.HighPart = file_time.dwHighDateTime;
    tp->tv_sec = (long)((ularge.QuadPart - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);

    return 0;
}
#endif

// 获取纳秒值，windows只能取到相对值，只能做比较不能转毫秒等
R_API int64_t rtime_nanosec() {
#ifdef WIN32
    static LARGE_INTEGER frequency = {0};
    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
        //rassert(frequency.QuadPart != 0);
    }

    LARGE_INTEGER time_now;
    QueryPerformanceCounter(&time_now);
    return time_now.QuadPart * NANOSECOND_PER_SECOND / frequency.QuadPart;
#else
    //本身消耗较大，和一次系统调用差不多，i5 300nm左右
    struct timespec time_now = { 0, 0 };
    clock_gettime(CLOCK_REALTIME, &time_now); //CLOCK_REALTIME-实际系统时间；CLOCK_MONOTONIC-调时间无效；PROCESS/THREAD
    return time_now.tv_sec * NANOSECOND_PER_SECOND + time_now.tv_nsec; //.tv_sec
#endif
}

static struct timezone* rtime_zone = NULL;
// 获取微秒值，注意不是时间戳
R_API int64_t rtime_microsec() {
  struct timeval tv;
#ifdef WIN32
  rtime_gettimeofdayfix(&tv, NULL);
#else
  gettimeofday(&tv, rtime_zone);
#endif
  int64_t n = tv.tv_sec * 1000000 + tv.tv_usec;

  return n;
}

// 获取毫秒时间戳（没带时区）
R_API int64_t rtime_millisec() {
    struct timeval tv;
#ifdef WIN32
    rtime_gettimeofdayfix(&tv, NULL);
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    int64_t timeMills = clock * 1000 + wtm.wMilliseconds;
    return timeMills;
#else
    gettimeofday(&tv, rtime_zone);
    int64_t val = tv.tv_sec * 1000000 + tv.tv_usec;
    val /= 1000;

    return val;
#endif
}

//todo Ray 多线程
static int64_t _inc_millisec;
static int64_t _inc_microsec;
static int64_t _inc_nanosec;
R_API void rtime_set_inc(int64_t millisec, int64_t microsec, int64_t nanosec) {
    _inc_millisec = millisec > 0 ? millisec : _inc_millisec;
    _inc_microsec = microsec > 0 ? microsec : _inc_microsec;
    _inc_nanosec = nanosec > 0 ? nanosec : _inc_nanosec;
}
R_API int64_t rtime_nanosec_inc() {
    return _inc_millisec++;
}
R_API int64_t rtime_microsec_inc() {
    return _inc_microsec++;
}
R_API int64_t rtime_millisec_inc() {
    return _inc_nanosec++;
}


//void rtime_2metis(time_t t, char *pcTime) {
//    struct tm *tm_t;
//    tm_t = localtime(&t);
//    strftime(pcTime, 128, "%F %T", tm_t);
//}
//
//long rtimr_4metis(char *str_time) {
//    struct tm stm;
//
//    strptime(str_time, "%Y-%m-%d %H:%M:%S", &stm);
//
//    long t = mktime(&stm);
//    return t;
//}

/*************************************************************************************
************************ unix timestamp to time string 跨平台， 安卓用[begin] ***********************
***************************************************************************************/

/*
 * 判断是否是闰年
 * 参数：year：需要判断的年份数
 * 返回：闰年返回1，否则返回0
 */
static unsigned char rlib_is_leap_year(unsigned short year) {
    if ((year % 400) == 0) {
        return 1;
    } else if ((year % 100) == 0) {
        return 0;
    } else if ((year % 4) == 0) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * 得到每个月有多少天
 * 参数：month：需要得到天数的月份数
 *       year：该月所对应的年份数
 * 返回：该月有多少天
 */
static unsigned char rlib_last_day_of_mon(unsigned char month, unsigned short year) {
    if ((month == 0) || (month > 12)) {
        return g_day_per_mon[1] + rlib_is_leap_year(year);
    }

    if (month != 2) {
        return g_day_per_mon[month - 1];
    } else {
        return g_day_per_mon[1] + rlib_is_leap_year(year);
    }
}


static int rtime_zone_int = 8;
R_API void rtime_set_time_zone(int time_zone) {
    rtime_zone_int = time_zone;
}
R_API int rtime_get_time_zone() {
    return rtime_zone_int;
}

R_API int* rtime_from_time_millis_security(int64_t time_millis, int* datas) {
	static int rtime_datas[7] = { 0 ,0, 0, 0, 0, 0, 0 };//todo Ray多线程 ThreadLocale
    if (datas == NULL) {
        datas = rtime_datas;
    }
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int mills = time_millis % 1000;

    time_millis /= 1000;

    //将时间戳值转化成天数。通过天数可以比较方便地算出年、月、日。
    int days = (int)(time_millis / SEC_PER_DAY);
    //这个时间戳值的年。
    int year_temp = 0;
    int day_temp = 0;
    //使用夹逼法计算 days 天中包含的年数。
    for (year_temp = UTC_BASE_YEAR; days > 0; year_temp++) {
        day_temp = (DAY_PER_YEAR + rlib_is_leap_year(year_temp)); //这一年有多少天？
        if (days >= day_temp) { //条件成立，则 year_temp 即是这个时间戳值所代表的年数。
            days -= day_temp;
        } else {
            break;
        }
    }
    year = year_temp;

    //这个时间戳值的月
    int month_temp = 0;
    for (month_temp = 1; month_temp < MONTH_PER_YEAR; month_temp++) {
        day_temp = rlib_last_day_of_mon(month_temp, year);
        if (days >= day_temp) {
            days -= day_temp;
        } else {
            break;
        }
    }
    month = month_temp;

    day = days + 1;

    //转化成秒。
    int secs = time_millis % SEC_PER_DAY;
    //这个时间戳值的小时数。
    hour = (secs / SEC_PER_HOUR + rtime_zone_int) % 24;
    //这个时间戳值的分钟数。
    secs %= SEC_PER_HOUR;
    minute = secs / SEC_PER_MIN;
    //这个时间戳的秒钟数。
    second = secs % SEC_PER_MIN;

    datas[0] = year;
    datas[1] = month;
    datas[2] = day;
    datas[3] = hour;
    datas[4] = minute;
    datas[5] = second;
    datas[6] = mills;
    return datas;
}

//不支持多线程
R_API int* rtime_from_time_millis(int64_t time_millis) {
    return rtime_from_time_millis_security(time_millis, NULL);
}


R_API int64_t rtimeout_get_block(rtimeout_t* tm) {
    if (tm->block < 0 && tm->total < 0) {
        return -1;
    } else if (tm->block < 0) {//total > 0，计算total剩余时间
        int64_t t = tm->total + tm->start - rtime_microsec();
        return rmacro_max(t, 0);
    } else if (tm->total < 0) {//直接返回block时间
        return tm->block;
    } else {// block & total，返回block，total剩余 的极小值
        int64_t t = tm->total + tm->start - rtime_microsec();
        return rmacro_min(tm->block, rmacro_max(t, 0));
    }
}

R_API int64_t rtimeout_get_total(rtimeout_t* tm) {
    if (tm->block < 0 && tm->total < 0) {
        return -1;
    } else if (tm->block < 0) {
        int64_t t = tm->total + tm->start - rtime_microsec();
        return rmacro_max(t, 0);
    } else if (tm->total < 0) {
        int64_t t = tm->block + tm->start - rtime_microsec();
        return rmacro_max(t, 0);
    } else {
        int64_t t = tm->total + tm->start - rtime_microsec();
        return rmacro_min(tm->block, rmacro_max(t, 0));
    }
}

