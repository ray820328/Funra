﻿/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <direct.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#include "uv.h"
#include "internal.h"

/* clang-format off */
#include <winsock2.h>
#include <winperf.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <windows.h>
/* clang-format on */
#include <userenv.h>
#include <math.h>

/*
 * Max title length; the only thing MSDN tells us about the maximum length
 * of the console title is that it is smaller than 64K. However in practice
 * it is much smaller, and there is no way to figure out what the exact length
 * of the title is or can be, at least not on XP. To make it even more
 * annoying, GetConsoleTitle fails when the buffer to be read into is bigger
 * than the actual maximum length. So we make a conservative guess here;
 * just don't put the novel you're writing in the title, unless the plot
 * survives truncation.
 */
#define MAX_TITLE_LENGTH 8192

/* The number of nanoseconds in one second. */
#define UV__NANOSEC 1000000000

/* Max user name length, from iphlpapi.h */
#ifndef UNLEN
# define UNLEN 256
#endif


/* A RtlGenRandom() by any other name... */
extern BOOLEAN NTAPI SystemFunction036(PVOID Buffer, ULONG BufferLength);

/* Cached copy of the process title, plus a mutex guarding it. */
static char *process_title;
static CRITICAL_SECTION process_title_lock;

/* Frequency of the high-resolution clock. */
static uint64_t hrtime_frequency_ = 0;


/*
 * One-time initialization code for functionality defined in util.c.
 */
void uv__util_init(void) {
  LARGE_INTEGER perf_frequency;

  /* Initialize process title access mutex. */
  InitializeCriticalSection(&process_title_lock);

  /* Retrieve high-resolution timer frequency
   * and precompute its reciprocal.
   */
  if (QueryPerformanceFrequency(&perf_frequency)) {
    hrtime_frequency_ = perf_frequency.QuadPart;
  } else {
    uv_fatal_error(GetLastError(), "QueryPerformanceFrequency");
  }
}


int uv_exepath(char* buffer, size_t* size_ptr) {
  int utf8_len, utf16_buffer_len, utf16_len;
  WCHAR* utf16_buffer;
  int err;

  if (buffer == NULL || size_ptr == NULL || *size_ptr == 0) {
    return UV_EINVAL;
  }

  if (*size_ptr > 32768) {
    /* Windows paths can never be longer than this. */
    utf16_buffer_len = 32768;
  } else {
    utf16_buffer_len = (int) *size_ptr;
  }

  utf16_buffer = (WCHAR*) uv__malloc(sizeof(WCHAR) * utf16_buffer_len);
  if (!utf16_buffer) {
    return UV_ENOMEM;
  }

  /* Get the path as UTF-16. */
  utf16_len = GetModuleFileNameW(NULL, utf16_buffer, utf16_buffer_len);
  if (utf16_len <= 0) {
    err = GetLastError();
    goto error;
  }

  /* utf16_len contains the length, *not* including the terminating null. */
  utf16_buffer[utf16_len] = L'\0';

  /* Convert to UTF-8 */
  utf8_len = WideCharToMultiByte(CP_UTF8,
                                 0,
                                 utf16_buffer,
                                 -1,
                                 buffer,
                                 (int) *size_ptr,
                                 NULL,
                                 NULL);
  if (utf8_len == 0) {
    err = GetLastError();
    goto error;
  }

  uv__free(utf16_buffer);

  /* utf8_len *does* include the terminating null at this point, but the
   * returned size shouldn't. */
  *size_ptr = utf8_len - 1;
  return 0;

 error:
  uv__free(utf16_buffer);
  return uv_translate_sys_error(err);
}


int uv_cwd(char* buffer, size_t* size) {
  DWORD utf16_len;
  WCHAR *utf16_buffer;
  int r;

  if (buffer == NULL || size == NULL) {
    return UV_EINVAL;
  }

  utf16_len = GetCurrentDirectoryW(0, NULL);
  if (utf16_len == 0) {
    return uv_translate_sys_error(GetLastError());
  }
  utf16_buffer = uv__malloc(utf16_len * sizeof(WCHAR));
  if (utf16_buffer == NULL) {
    return UV_ENOMEM;
  }

  utf16_len = GetCurrentDirectoryW(utf16_len, utf16_buffer);
  if (utf16_len == 0) {
    uv__free(utf16_buffer);
    return uv_translate_sys_error(GetLastError());
  }

  /* utf16_len contains the length, *not* including the terminating null. */
  utf16_buffer[utf16_len] = L'\0';

  /* The returned directory should not have a trailing slash, unless it points
   * at a drive root, like c:\. Remove it if needed. */
  if (utf16_buffer[utf16_len - 1] == L'\\' &&
      !(utf16_len == 3 && utf16_buffer[1] == L':')) {
    utf16_len--;
    utf16_buffer[utf16_len] = L'\0';
  }

  /* Check how much space we need */
  r = WideCharToMultiByte(CP_UTF8,
                          0,
                          utf16_buffer,
                          -1,
                          NULL,
                          0,
                          NULL,
                          NULL);
  if (r == 0) {
    uv__free(utf16_buffer);
    return uv_translate_sys_error(GetLastError());
  } else if (r > (int) *size) {
    uv__free(utf16_buffer);
    *size = r;
    return UV_ENOBUFS;
  }

  /* Convert to UTF-8 */
  r = WideCharToMultiByte(CP_UTF8,
                          0,
                          utf16_buffer,
                          -1,
                          buffer,
                          *size > INT_MAX ? INT_MAX : (int) *size,
                          NULL,
                          NULL);
  uv__free(utf16_buffer);

  if (r == 0) {
    return uv_translate_sys_error(GetLastError());
  }

  *size = r - 1;
  return 0;
}


int uv_chdir(const char* dir) {
  WCHAR *utf16_buffer;
  size_t utf16_len, new_utf16_len;
  WCHAR drive_letter, env_var[4];

  if (dir == NULL) {
    return UV_EINVAL;
  }

  utf16_len = MultiByteToWideChar(CP_UTF8,
                                  0,
                                  dir,
                                  -1,
                                  NULL,
                                  0);
  if (utf16_len == 0) {
    return uv_translate_sys_error(GetLastError());
  }
  utf16_buffer = uv__malloc(utf16_len * sizeof(WCHAR));
  if (utf16_buffer == NULL) {
    return UV_ENOMEM;
  }

  if (MultiByteToWideChar(CP_UTF8,
                          0,
                          dir,
                          -1,
                          utf16_buffer,
                          utf16_len) == 0) {
    uv__free(utf16_buffer);
    return uv_translate_sys_error(GetLastError());
  }

  if (!SetCurrentDirectoryW(utf16_buffer)) {
    uv__free(utf16_buffer);
    return uv_translate_sys_error(GetLastError());
  }

  /* Windows stores the drive-local path in an "hidden" environment variable,
   * which has the form "=C:=C:\Windows". SetCurrentDirectory does not update
   * this, so we'll have to do it. */
  new_utf16_len = GetCurrentDirectoryW(utf16_len, utf16_buffer);
  if (new_utf16_len > utf16_len ) {
    uv__free(utf16_buffer);
    utf16_buffer = uv__malloc(new_utf16_len * sizeof(WCHAR));
    if (utf16_buffer == NULL) {
      /* When updating the environment variable fails, return UV_OK anyway.
       * We did successfully change current working directory, only updating
       * hidden env variable failed. */
      return 0;
    }
    new_utf16_len = GetCurrentDirectoryW(new_utf16_len, utf16_buffer);
  }
  if (utf16_len == 0) {
    uv__free(utf16_buffer);
    return 0;
  }

  /* The returned directory should not have a trailing slash, unless it points
   * at a drive root, like c:\. Remove it if needed. */
  if (utf16_buffer[utf16_len - 1] == L'\\' &&
      !(utf16_len == 3 && utf16_buffer[1] == L':')) {
    utf16_len--;
    utf16_buffer[utf16_len] = L'\0';
  }

  if (utf16_len < 2 || utf16_buffer[1] != L':') {
    /* Doesn't look like a drive letter could be there - probably an UNC path.
     * TODO: Need to handle win32 namespaces like \\?\C:\ ? */
    drive_letter = 0;
  } else if (utf16_buffer[0] >= L'A' && utf16_buffer[0] <= L'Z') {
    drive_letter = utf16_buffer[0];
  } else if (utf16_buffer[0] >= L'a' && utf16_buffer[0] <= L'z') {
    /* Convert to uppercase. */
    drive_letter = utf16_buffer[0] - L'a' + L'A';
  } else {
    /* Not valid. */
    drive_letter = 0;
  }

  if (drive_letter != 0) {
    /* Construct the environment variable name and set it. */
    env_var[0] = L'=';
    env_var[1] = drive_letter;
    env_var[2] = L':';
    env_var[3] = L'\0';

    SetEnvironmentVariableW(env_var, utf16_buffer);
  }

  uv__free(utf16_buffer);
  return 0;
}


void uv_loadavg(double avg[3]) {
  /* Can't be implemented */
  avg[0] = avg[1] = avg[2] = 0;
}


uint64_t uv_get_free_memory(void) {
  MEMORYSTATUSEX memory_status;
  memory_status.dwLength = sizeof(memory_status);

  if (!GlobalMemoryStatusEx(&memory_status)) {
     return -1;
  }

  return (uint64_t)memory_status.ullAvailPhys;
}


uint64_t uv_get_total_memory(void) {
  MEMORYSTATUSEX memory_status;
  memory_status.dwLength = sizeof(memory_status);

  if (!GlobalMemoryStatusEx(&memory_status)) {
    return -1;
  }

  return (uint64_t)memory_status.ullTotalPhys;
}


uint64_t uv_get_constrained_memory(void) {
  return 0;  /* Memory constraints are unknown. */
}


uv_pid_t uv_os_getpid(void) {
  return GetCurrentProcessId();
}


uv_pid_t uv_os_getppid(void) {
  int parent_pid = -1;
  HANDLE handle;
  PROCESSENTRY32 pe;
  DWORD current_pid = GetCurrentProcessId();

  pe.dwSize = sizeof(PROCESSENTRY32);
  handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

  if (Process32First(handle, &pe)) {
    do {
      if (pe.th32ProcessID == current_pid) {
        parent_pid = pe.th32ParentProcessID;
        break;
      }
    } while( Process32Next(handle, &pe));
  }

  CloseHandle(handle);
  return parent_pid;
}


char** uv_setup_args(int argc, char** argv) {
  return argv;
}


void uv__process_title_cleanup(void) {
}


int uv_set_process_title(const char* title) {
  int err;
  int length;
  WCHAR* title_w = NULL;

  uv__once_init();

  /* Find out how big the buffer for the wide-char title must be */
  length = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
  if (!length) {
    err = GetLastError();
    goto done;
  }

  /* Convert to wide-char string */
  title_w = (WCHAR*)uv__malloc(sizeof(WCHAR) * length);
  if (!title_w) {
    uv_fatal_error(ERROR_OUTOFMEMORY, "uv__malloc");
  }

  length = MultiByteToWideChar(CP_UTF8, 0, title, -1, title_w, length);
  if (!length) {
    err = GetLastError();
    goto done;
  }

  /* If the title must be truncated insert a \0 terminator there */
  if (length > MAX_TITLE_LENGTH) {
    title_w[MAX_TITLE_LENGTH - 1] = L'\0';
  }

  if (!SetConsoleTitleW(title_w)) {
    err = GetLastError();
    goto done;
  }

  EnterCriticalSection(&process_title_lock);
  uv__free(process_title);
  process_title = uv__strdup(title);
  LeaveCriticalSection(&process_title_lock);

  err = 0;

done:
  uv__free(title_w);
  return uv_translate_sys_error(err);
}


static int uv__get_process_title(void) {
  WCHAR title_w[MAX_TITLE_LENGTH];

  if (!GetConsoleTitleW(title_w, sizeof(title_w) / sizeof(WCHAR))) {
    return -1;
  }

  if (uv__convert_utf16_to_utf8(title_w, -1, &process_title) != 0)
    return -1;

  return 0;
}


int uv_get_process_title(char* buffer, size_t size) {
  size_t len;

  if (buffer == NULL || size == 0)
    return UV_EINVAL;

  uv__once_init();

  EnterCriticalSection(&process_title_lock);
  /*
   * If the process_title was never read before nor explicitly set,
   * we must query it with getConsoleTitleW
   */
  if (!process_title && uv__get_process_title() == -1) {
    LeaveCriticalSection(&process_title_lock);
    return uv_translate_sys_error(GetLastError());
  }

  assert(process_title);
  len = strlen(process_title) + 1;

  if (size < len) {
    LeaveCriticalSection(&process_title_lock);
    return UV_ENOBUFS;
  }

  memcpy(buffer, process_title, len);
  LeaveCriticalSection(&process_title_lock);

  return 0;
}


uint64_t uv_hrtime(void) {
  uv__once_init();
  return uv__hrtime(UV__NANOSEC);
}

uint64_t uv__hrtime(unsigned int scale) {
  LARGE_INTEGER counter;
  double scaled_freq;
  double result;

  assert(hrtime_frequency_ != 0);
  assert(scale != 0);
  if (!QueryPerformanceCounter(&counter)) {
    uv_fatal_error(GetLastError(), "QueryPerformanceCounter");
  }
  assert(counter.QuadPart != 0);

  /* Because we have no guarantee about the order of magnitude of the
   * performance counter interval, integer math could cause this computation
   * to overflow. Therefore we resort to floating point math.
   */
  scaled_freq = (double) hrtime_frequency_ / scale;
  result = (double) counter.QuadPart / scaled_freq;
  return (uint64_t) result;
}


int uv_resident_set_memory(size_t* rss) {
  HANDLE current_process;
  PROCESS_MEMORY_COUNTERS pmc;

  current_process = GetCurrentProcess();

  if (!GetProcessMemoryInfo(current_process, &pmc, sizeof(pmc))) {
    return uv_translate_sys_error(GetLastError());
  }

  *rss = pmc.WorkingSetSize;

  return 0;
}


int uv_uptime(double* uptime) {
  BYTE stack_buffer[4096];
  BYTE* malloced_buffer = NULL;
  BYTE* buffer = (BYTE*) stack_buffer;
  size_t buffer_size = sizeof(stack_buffer);
  DWORD data_size;

  PERF_DATA_BLOCK* data_block;
  PERF_OBJECT_TYPE* object_type;
  PERF_COUNTER_DEFINITION* counter_definition;

  DWORD i;

  for (;;) {
    LONG result;

    data_size = (DWORD) buffer_size;
    result = RegQueryValueExW(HKEY_PERFORMANCE_DATA,
                              L"2",
                              NULL,
                              NULL,
                              buffer,
                              &data_size);
    if (result == ERROR_SUCCESS) {
      break;
    } else if (result != ERROR_MORE_DATA) {
      *uptime = 0;
      return uv_translate_sys_error(result);
    }

    buffer_size *= 2;
    /* Don't let the buffer grow infinitely. */
    if (buffer_size > 1 << 20) {
      goto internalError;
    }

    uv__free(malloced_buffer);

    buffer = malloced_buffer = (BYTE*) uv__malloc(buffer_size);
    if (malloced_buffer == NULL) {
      *uptime = 0;
      return UV_ENOMEM;
    }
  }

  if (data_size < sizeof(*data_block))
    goto internalError;

  data_block = (PERF_DATA_BLOCK*) buffer;

  if (wmemcmp(data_block->Signature, L"PERF", 4) != 0)
    goto internalError;

  if (data_size < data_block->HeaderLength + sizeof(*object_type))
    goto internalError;

  object_type = (PERF_OBJECT_TYPE*) (buffer + data_block->HeaderLength);

  if (object_type->NumInstances != PERF_NO_INSTANCES)
    goto internalError;

  counter_definition = (PERF_COUNTER_DEFINITION*) (buffer +
      data_block->HeaderLength + object_type->HeaderLength);
  for (i = 0; i < object_type->NumCounters; i++) {
    if ((BYTE*) counter_definition + sizeof(*counter_definition) >
        buffer + data_size) {
      break;
    }

    if (counter_definition->CounterNameTitleIndex == 674 &&
        counter_definition->CounterSize == sizeof(uint64_t)) {
      if (counter_definition->CounterOffset + sizeof(uint64_t) > data_size ||
          !(counter_definition->CounterType & PERF_OBJECT_TIMER)) {
        goto internalError;
      } else {
        BYTE* address = (BYTE*) object_type + object_type->DefinitionLength +
                        counter_definition->CounterOffset;
        uint64_t value = *((uint64_t*) address);
        *uptime = floor((double) (object_type->PerfTime.QuadPart - value) /
                        (double) object_type->PerfFreq.QuadPart);
        uv__free(malloced_buffer);
        return 0;
      }
    }

    counter_definition = (PERF_COUNTER_DEFINITION*)
        ((BYTE*) counter_definition + counter_definition->ByteLength);
  }

  /* If we get here, the uptime value was not found. */
  uv__free(malloced_buffer);
  *uptime = 0;
  return UV_ENOSYS;

 internalError:
  uv__free(malloced_buffer);
  *uptime = 0;
  return UV_EIO;
}


int uv_cpu_info(uv_cpu_info_t** cpu_infos_ptr, int* cpu_count_ptr) {
  uv_cpu_info_t* cpu_infos;
  SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* sppi;
  DWORD sppi_size;
  SYSTEM_INFO system_info;
  DWORD cpu_count, i;
  NTSTATUS status;
  ULONG result_size;
  int err;
  uv_cpu_info_t* cpu_info;

  cpu_infos = NULL;
  cpu_count = 0;
  sppi = NULL;

  uv__once_init();

  GetSystemInfo(&system_info);
  cpu_count = system_info.dwNumberOfProcessors;

  cpu_infos = uv__calloc(cpu_count, sizeof *cpu_infos);
  if (cpu_infos == NULL) {
    err = ERROR_OUTOFMEMORY;
    goto error;
  }

  sppi_size = cpu_count * sizeof(*sppi);
  sppi = uv__malloc(sppi_size);
  if (sppi == NULL) {
    err = ERROR_OUTOFMEMORY;
    goto error;
  }

  status = pNtQuerySystemInformation(SystemProcessorPerformanceInformation,
                                     sppi,
                                     sppi_size,
                                     &result_size);
  if (!NT_SUCCESS(status)) {
    err = pRtlNtStatusToDosError(status);
    goto error;
  }

  assert(result_size == sppi_size);

  for (i = 0; i < cpu_count; i++) {
    WCHAR key_name[128];
    HKEY processor_key;
    DWORD cpu_speed;
    DWORD cpu_speed_size = sizeof(cpu_speed);
    WCHAR cpu_brand[256];
    DWORD cpu_brand_size = sizeof(cpu_brand);
    size_t len;

    len = _snwprintf(key_name,
                     ARRAY_SIZE(key_name),
                     L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%d",
                     i);

    assert(len > 0 && len < ARRAY_SIZE(key_name));

    err = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        key_name,
                        0,
                        KEY_QUERY_VALUE,
                        &processor_key);
    if (err != ERROR_SUCCESS) {
      goto error;
    }

    err = RegQueryValueExW(processor_key,
                           L"~MHz",
                           NULL,
                           NULL,
                           (BYTE*)&cpu_speed,
                           &cpu_speed_size);
    if (err != ERROR_SUCCESS) {
      RegCloseKey(processor_key);
      goto error;
    }

    err = RegQueryValueExW(processor_key,
                           L"ProcessorNameString",
                           NULL,
                           NULL,
                           (BYTE*)&cpu_brand,
                           &cpu_brand_size);
    RegCloseKey(processor_key);
    if (err != ERROR_SUCCESS)
      goto error;

    cpu_info = &cpu_infos[i];
    cpu_info->speed = cpu_speed;
    cpu_info->cpu_times.user = sppi[i].UserTime.QuadPart / 10000;
    cpu_info->cpu_times.sys = (sppi[i].KernelTime.QuadPart -
        sppi[i].IdleTime.QuadPart) / 10000;
    cpu_info->cpu_times.idle = sppi[i].IdleTime.QuadPart / 10000;
    cpu_info->cpu_times.irq = sppi[i].InterruptTime.QuadPart / 10000;
    cpu_info->cpu_times.nice = 0;

    uv__convert_utf16_to_utf8(cpu_brand,
                              cpu_brand_size / sizeof(WCHAR),
                              &(cpu_info->model));
  }

  uv__free(sppi);

  *cpu_count_ptr = cpu_count;
  *cpu_infos_ptr = cpu_infos;

  return 0;

 error:
  if (cpu_infos != NULL) {
    /* This is safe because the cpu_infos array is zeroed on allocation. */
    for (i = 0; i < cpu_count; i++)
      uv__free(cpu_infos[i].model);
  }

  uv__free(cpu_infos);
  uv__free(sppi);

  return uv_translate_sys_error(err);
}


static int is_windows_version_or_greater(DWORD os_major,
                                         DWORD os_minor,
                                         WORD service_pack_major,
                                         WORD service_pack_minor) {
  OSVERSIONINFOEX osvi;
  DWORDLONG condition_mask = 0;
  int op = VER_GREATER_EQUAL;

  /* Initialize the OSVERSIONINFOEX structure. */
  ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  osvi.dwMajorVersion = os_major;
  osvi.dwMinorVersion = os_minor;
  osvi.wServicePackMajor = service_pack_major;
  osvi.wServicePackMinor = service_pack_minor;

  /* Initialize the condition mask. */
  VER_SET_CONDITION(condition_mask, VER_MAJORVERSION, op);
  VER_SET_CONDITION(condition_mask, VER_MINORVERSION, op);
  VER_SET_CONDITION(condition_mask, VER_SERVICEPACKMAJOR, op);
  VER_SET_CONDITION(condition_mask, VER_SERVICEPACKMINOR, op);

  /* Perform the test. */
  return (int) VerifyVersionInfo(
    &osvi,
    VER_MAJORVERSION | VER_MINORVERSION |
    VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
    condition_mask);
}


static int address_prefix_match(int family,
                                struct sockaddr* address,
                                struct sockaddr* prefix_address,
                                int prefix_len) {
  uint8_t* address_data;
  uint8_t* prefix_address_data;
  int i;

  assert(address->sa_family == family);
  assert(prefix_address->sa_family == family);

  if (family == AF_INET6) {
    address_data = (uint8_t*) &(((struct sockaddr_in6 *) address)->sin6_addr);
    prefix_address_data =
      (uint8_t*) &(((struct sockaddr_in6 *) prefix_address)->sin6_addr);
  } else {
    address_data = (uint8_t*) &(((struct sockaddr_in *) address)->sin_addr);
    prefix_address_data =
      (uint8_t*) &(((struct sockaddr_in *) prefix_address)->sin_addr);
  }

  for (i = 0; i < prefix_len >> 3; i++) {
    if (address_data[i] != prefix_address_data[i])
      return 0;
  }

  if (prefix_len % 8)
    return prefix_address_data[i] ==
      (address_data[i] & (0xff << (8 - prefix_len % 8)));

  return 1;
}


int uv_interface_addresses(uv_interface_address_t** addresses_ptr,
    int* count_ptr) {
  IP_ADAPTER_ADDRESSES* win_address_buf;
  ULONG win_address_buf_size;
  IP_ADAPTER_ADDRESSES* adapter;

  uv_interface_address_t* uv_address_buf;
  char* name_buf;
  size_t uv_address_buf_size;
  uv_interface_address_t* uv_address;

  int count;

  int is_vista_or_greater;
  ULONG flags;

  *addresses_ptr = NULL;
  *count_ptr = 0;

  is_vista_or_greater = is_windows_version_or_greater(6, 0, 0, 0);
  if (is_vista_or_greater) {
    flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
      GAA_FLAG_SKIP_DNS_SERVER;
  } else {
    /* We need at least XP SP1. */
    if (!is_windows_version_or_greater(5, 1, 1, 0))
      return UV_ENOTSUP;

    flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
      GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_PREFIX;
  }


  /* Fetch the size of the adapters reported by windows, and then get the list
   * itself. */
  win_address_buf_size = 0;
  win_address_buf = NULL;

  for (;;) {
    ULONG r;

    /* If win_address_buf is 0, then GetAdaptersAddresses will fail with.
     * ERROR_BUFFER_OVERFLOW, and the required buffer size will be stored in
     * win_address_buf_size. */
    r = GetAdaptersAddresses(AF_UNSPEC,
                             flags,
                             NULL,
                             win_address_buf,
                             &win_address_buf_size);

    if (r == ERROR_SUCCESS)
      break;

    uv__free(win_address_buf);

    switch (r) {
      case ERROR_BUFFER_OVERFLOW:
        /* This happens when win_address_buf is NULL or too small to hold all
         * adapters. */
        win_address_buf = uv__malloc(win_address_buf_size);
        if (win_address_buf == NULL)
          return UV_ENOMEM;

        continue;

      case ERROR_NO_DATA: {
        /* No adapters were found. */
        uv_address_buf = uv__malloc(1);
        if (uv_address_buf == NULL)
          return UV_ENOMEM;

        *count_ptr = 0;
        *addresses_ptr = uv_address_buf;

        return 0;
      }

      case ERROR_ADDRESS_NOT_ASSOCIATED:
        return UV_EAGAIN;

      case ERROR_INVALID_PARAMETER:
        /* MSDN says:
         *   "This error is returned for any of the following conditions: the
         *   SizePointer parameter is NULL, the Address parameter is not
         *   AF_INET, AF_INET6, or AF_UNSPEC, or the address information for
         *   the parameters requested is greater than ULONG_MAX."
         * Since the first two conditions are not met, it must be that the
         * adapter data is too big.
         */
        return UV_ENOBUFS;

      default:
        /* Other (unspecified) errors can happen, but we don't have any special
         * meaning for them. */
        assert(r != ERROR_SUCCESS);
        return uv_translate_sys_error(r);
    }
  }

  /* Count the number of enabled interfaces and compute how much space is
   * needed to store their info. */
  count = 0;
  uv_address_buf_size = 0;

  for (adapter = win_address_buf;
       adapter != NULL;
       adapter = adapter->Next) {
    IP_ADAPTER_UNICAST_ADDRESS* unicast_address;
    int name_size;

    /* Interfaces that are not 'up' should not be reported. Also skip
     * interfaces that have no associated unicast address, as to avoid
     * allocating space for the name for this interface. */
    if (adapter->OperStatus != IfOperStatusUp ||
        adapter->FirstUnicastAddress == NULL)
      continue;

    /* Compute the size of the interface name. */
    name_size = WideCharToMultiByte(CP_UTF8,
                                    0,
                                    adapter->FriendlyName,
                                    -1,
                                    NULL,
                                    0,
                                    NULL,
                                    FALSE);
    if (name_size <= 0) {
      uv__free(win_address_buf);
      return uv_translate_sys_error(GetLastError());
    }
    uv_address_buf_size += name_size;

    /* Count the number of addresses associated with this interface, and
     * compute the size. */
    for (unicast_address = (IP_ADAPTER_UNICAST_ADDRESS*)
                           adapter->FirstUnicastAddress;
         unicast_address != NULL;
         unicast_address = unicast_address->Next) {
      count++;
      uv_address_buf_size += sizeof(uv_interface_address_t);
    }
  }

  /* Allocate space to store interface data plus adapter names. */
  uv_address_buf = uv__malloc(uv_address_buf_size);
  if (uv_address_buf == NULL) {
    uv__free(win_address_buf);
    return UV_ENOMEM;
  }

  /* Compute the start of the uv_interface_address_t array, and the place in
   * the buffer where the interface names will be stored. */
  uv_address = uv_address_buf;
  name_buf = (char*) (uv_address_buf + count);

  /* Fill out the output buffer. */
  for (adapter = win_address_buf;
       adapter != NULL;
       adapter = adapter->Next) {
    IP_ADAPTER_UNICAST_ADDRESS* unicast_address;
    int name_size;
    size_t max_name_size;

    if (adapter->OperStatus != IfOperStatusUp ||
        adapter->FirstUnicastAddress == NULL)
      continue;

    /* Convert the interface name to UTF8. */
    max_name_size = (char*) uv_address_buf + uv_address_buf_size - name_buf;
    if (max_name_size > (size_t) INT_MAX)
      max_name_size = INT_MAX;
    name_size = WideCharToMultiByte(CP_UTF8,
                                    0,
                                    adapter->FriendlyName,
                                    -1,
                                    name_buf,
                                    (int) max_name_size,
                                    NULL,
                                    FALSE);
    if (name_size <= 0) {
      uv__free(win_address_buf);
      uv__free(uv_address_buf);
      return uv_translate_sys_error(GetLastError());
    }

    /* Add an uv_interface_address_t element for every unicast address. */
    for (unicast_address = (IP_ADAPTER_UNICAST_ADDRESS*)
                           adapter->FirstUnicastAddress;
         unicast_address != NULL;
         unicast_address = unicast_address->Next) {
      struct sockaddr* sa;
      ULONG prefix_len;

      sa = unicast_address->Address.lpSockaddr;

      /* XP has no OnLinkPrefixLength field. */
      if (is_vista_or_greater) {
        prefix_len =
          ((IP_ADAPTER_UNICAST_ADDRESS_LH*) unicast_address)->OnLinkPrefixLength;
      } else {
        /* Prior to Windows Vista the FirstPrefix pointed to the list with
         * single prefix for each IP address assigned to the adapter.
         * Order of FirstPrefix does not match order of FirstUnicastAddress,
         * so we need to find corresponding prefix.
         */
        IP_ADAPTER_PREFIX* prefix;
        prefix_len = 0;

        for (prefix = adapter->FirstPrefix; prefix; prefix = prefix->Next) {
          /* We want the longest matching prefix. */
          if (prefix->Address.lpSockaddr->sa_family != sa->sa_family ||
              prefix->PrefixLength <= prefix_len)
            continue;

          if (address_prefix_match(sa->sa_family, sa,
              prefix->Address.lpSockaddr, prefix->PrefixLength)) {
            prefix_len = prefix->PrefixLength;
          }
        }

        /* If there is no matching prefix information, return a single-host
         * subnet mask (e.g. 255.255.255.255 for IPv4).
         */
        if (!prefix_len)
          prefix_len = (sa->sa_family == AF_INET6) ? 128 : 32;
      }

      memset(uv_address, 0, sizeof *uv_address);

      uv_address->name = name_buf;

      if (adapter->PhysicalAddressLength == sizeof(uv_address->phys_addr)) {
        memcpy(uv_address->phys_addr,
               adapter->PhysicalAddress,
               sizeof(uv_address->phys_addr));
      }

      uv_address->is_internal =
          (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK);

      if (sa->sa_family == AF_INET6) {
        uv_address->address.address6 = *((struct sockaddr_in6 *) sa);

        uv_address->netmask.netmask6.sin6_family = AF_INET6;
        memset(uv_address->netmask.netmask6.sin6_addr.s6_addr, 0xff, prefix_len >> 3);
        /* This check ensures that we don't write past the size of the data. */
        if (prefix_len % 8) {
          uv_address->netmask.netmask6.sin6_addr.s6_addr[prefix_len >> 3] =
              0xff << (8 - prefix_len % 8);
        }

      } else {
        uv_address->address.address4 = *((struct sockaddr_in *) sa);

        uv_address->netmask.netmask4.sin_family = AF_INET;
        uv_address->netmask.netmask4.sin_addr.s_addr = (prefix_len > 0) ?
            htonl(0xffffffff << (32 - prefix_len)) : 0;
      }

      uv_address++;
    }

    name_buf += name_size;
  }

  uv__free(win_address_buf);

  *addresses_ptr = uv_address_buf;
  *count_ptr = count;

  return 0;
}


void uv_free_interface_addresses(uv_interface_address_t* addresses,
    int count) {
  uv__free(addresses);
}


int uv_getrusage(uv_rusage_t *uv_rusage) {
  FILETIME createTime, exitTime, kernelTime, userTime;
  SYSTEMTIME kernelSystemTime, userSystemTime;
  PROCESS_MEMORY_COUNTERS memCounters;
  IO_COUNTERS ioCounters;
  int ret;

  ret = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime);
  if (ret == 0) {
    return uv_translate_sys_error(GetLastError());
  }

  ret = FileTimeToSystemTime(&kernelTime, &kernelSystemTime);
  if (ret == 0) {
    return uv_translate_sys_error(GetLastError());
  }

  ret = FileTimeToSystemTime(&userTime, &userSystemTime);
  if (ret == 0) {
    return uv_translate_sys_error(GetLastError());
  }

  ret = GetProcessMemoryInfo(GetCurrentProcess(),
                             &memCounters,
                             sizeof(memCounters));
  if (ret == 0) {
    return uv_translate_sys_error(GetLastError());
  }

  ret = GetProcessIoCounters(GetCurrentProcess(), &ioCounters);
  if (ret == 0) {
    return uv_translate_sys_error(GetLastError());
  }

  memset(uv_rusage, 0, sizeof(*uv_rusage));

  uv_rusage->ru_utime.tv_sec = userSystemTime.wHour * 3600 +
                               userSystemTime.wMinute * 60 +
                               userSystemTime.wSecond;
  uv_rusage->ru_utime.tv_usec = userSystemTime.wMilliseconds * 1000;

  uv_rusage->ru_stime.tv_sec = kernelSystemTime.wHour * 3600 +
                               kernelSystemTime.wMinute * 60 +
                               kernelSystemTime.wSecond;
  uv_rusage->ru_stime.tv_usec = kernelSystemTime.wMilliseconds * 1000;

  uv_rusage->ru_majflt = (uint64_t) memCounters.PageFaultCount;
  uv_rusage->ru_maxrss = (uint64_t) memCounters.PeakWorkingSetSize / 1024;

  uv_rusage->ru_oublock = (uint64_t) ioCounters.WriteOperationCount;
  uv_rusage->ru_inblock = (uint64_t) ioCounters.ReadOperationCount;

  return 0;
}


int uv_os_homedir(char* buffer, size_t* size) {
  uv_passwd_t pwd;
  size_t len;
  int r;

  /* Check if the USERPROFILE environment variable is set first. The task of
     performing input validation on buffer and size is taken care of by
     uv_os_getenv(). */
  r = uv_os_getenv("USERPROFILE", buffer, size);

  /* Don't return an error if USERPROFILE was not found. */
  if (r != UV_ENOENT)
    return r;

  /* USERPROFILE is not set, so call uv__getpwuid_r() */
  r = uv__getpwuid_r(&pwd);

  if (r != 0) {
    return r;
  }

  len = strlen(pwd.homedir);

  if (len >= *size) {
    *size = len + 1;
    uv_os_free_passwd(&pwd);
    return UV_ENOBUFS;
  }

  memcpy(buffer, pwd.homedir, len + 1);
  *size = len;
  uv_os_free_passwd(&pwd);

  return 0;
}


int uv_os_tmpdir(char* buffer, size_t* size) {
  wchar_t *path;
  DWORD bufsize;
  size_t len;

  if (buffer == NULL || size == NULL || *size == 0)
    return UV_EINVAL;

  len = 0;
  len = GetTempPathW(0, NULL);
  if (len == 0) {
    return uv_translate_sys_error(GetLastError());
  }
  /* Include space for terminating null char. */
  len += 1;
  path = uv__malloc(len * sizeof(wchar_t));
  if (path == NULL) {
    return UV_ENOMEM;
  }
  len  = GetTempPathW(len, path);

  if (len == 0) {
    uv__free(path);
    return uv_translate_sys_error(GetLastError());
  }

  /* The returned directory should not have a trailing slash, unless it points
   * at a drive root, like c:\. Remove it if needed. */
  if (path[len - 1] == L'\\' &&
      !(len == 3 && path[1] == L':')) {
    len--;
    path[len] = L'\0';
  }

  /* Check how much space we need */
  bufsize = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);

  if (bufsize == 0) {
    uv__free(path);
    return uv_translate_sys_error(GetLastError());
  } else if (bufsize > *size) {
    uv__free(path);
    *size = bufsize;
    return UV_ENOBUFS;
  }

  /* Convert to UTF-8 */
  bufsize = WideCharToMultiByte(CP_UTF8,
                                0,
                                path,
                                -1,
                                buffer,
                                *size,
                                NULL,
                                NULL);
  uv__free(path);

  if (bufsize == 0)
    return uv_translate_sys_error(GetLastError());

  *size = bufsize - 1;
  return 0;
}


void uv_os_free_passwd(uv_passwd_t* pwd) {
  if (pwd == NULL)
    return;

  uv__free(pwd->username);
  uv__free(pwd->homedir);
  pwd->username = NULL;
  pwd->homedir = NULL;
}


/*
 * Converts a UTF-16 string into a UTF-8 one. The resulting string is
 * null-terminated.
 *
 * If utf16 is null terminated, utf16len can be set to -1, otherwise it must
 * be specified.
 */
int uv__convert_utf16_to_utf8(const WCHAR* utf16, int utf16len, char** utf8) {
  DWORD bufsize;

  if (utf16 == NULL)
    return UV_EINVAL;

  /* Check how much space we need */
  bufsize = WideCharToMultiByte(CP_UTF8,
                                0,
                                utf16,
                                utf16len,
                                NULL,
                                0,
                                NULL,
                                NULL);

  if (bufsize == 0)
    return uv_translate_sys_error(GetLastError());

  /* Allocate the destination buffer adding an extra byte for the terminating
   * NULL. If utf16len is not -1 WideCharToMultiByte will not add it, so
   * we do it ourselves always, just in case. */
  *utf8 = uv__malloc(bufsize + 1);

  if (*utf8 == NULL)
    return UV_ENOMEM;

  /* Convert to UTF-8 */
  bufsize = WideCharToMultiByte(CP_UTF8,
                                0,
                                utf16,
                                utf16len,
                                *utf8,
                                bufsize,
                                NULL,
                                NULL);

  if (bufsize == 0) {
    uv__free(*utf8);
    *utf8 = NULL;
    return uv_translate_sys_error(GetLastError());
  }

  (*utf8)[bufsize] = '\0';
  return 0;
}


/*
 * Converts a UTF-8 string into a UTF-16 one. The resulting string is
 * null-terminated.
 *
 * If utf8 is null terminated, utf8len can be set to -1, otherwise it must
 * be specified.
 */
int uv__convert_utf8_to_utf16(const char* utf8, int utf8len, WCHAR** utf16) {
  int bufsize;

  if (utf8 == NULL)
    return UV_EINVAL;

  /* Check how much space we need */
  bufsize = MultiByteToWideChar(CP_UTF8, 0, utf8, utf8len, NULL, 0);

  if (bufsize == 0)
    return uv_translate_sys_error(GetLastError());

  /* Allocate the destination buffer adding an extra byte for the terminating
   * NULL. If utf8len is not -1 MultiByteToWideChar will not add it, so
   * we do it ourselves always, just in case. */
  *utf16 = uv__malloc(sizeof(WCHAR) * (bufsize + 1));

  if (*utf16 == NULL)
    return UV_ENOMEM;

  /* Convert to UTF-16 */
  bufsize = MultiByteToWideChar(CP_UTF8, 0, utf8, utf8len, *utf16, bufsize);

  if (bufsize == 0) {
    uv__free(*utf16);
    *utf16 = NULL;
    return uv_translate_sys_error(GetLastError());
  }

  (*utf16)[bufsize] = L'\0';
  return 0;
}


int uv__getpwuid_r(uv_passwd_t* pwd) {
  HANDLE token;
  wchar_t username[UNLEN + 1];
  wchar_t *path;
  DWORD bufsize;
  int r;

  if (pwd == NULL)
    return UV_EINVAL;

  /* Get the home directory using GetUserProfileDirectoryW() */
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token) == 0)
    return uv_translate_sys_error(GetLastError());

  bufsize = 0;
  GetUserProfileDirectoryW(token, NULL, &bufsize);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    r = GetLastError();
    CloseHandle(token);
    return uv_translate_sys_error(r);
  }

  path = uv__malloc(bufsize * sizeof(wchar_t));
  if (path == NULL) {
    CloseHandle(token);
    return UV_ENOMEM;
  }

  if (!GetUserProfileDirectoryW(token, path, &bufsize)) {
    r = GetLastError();
    CloseHandle(token);
    uv__free(path);
    return uv_translate_sys_error(r);
  }

  CloseHandle(token);

  /* Get the username using GetUserNameW() */
  bufsize = ARRAY_SIZE(username);
  if (!GetUserNameW(username, &bufsize)) {
    r = GetLastError();
    uv__free(path);

    /* This should not be possible */
    if (r == ERROR_INSUFFICIENT_BUFFER)
      return UV_ENOMEM;

    return uv_translate_sys_error(r);
  }

  pwd->homedir = NULL;
  r = uv__convert_utf16_to_utf8(path, -1, &pwd->homedir);
  uv__free(path);

  if (r != 0)
    return r;

  pwd->username = NULL;
  r = uv__convert_utf16_to_utf8(username, -1, &pwd->username);

  if (r != 0) {
    uv__free(pwd->homedir);
    return r;
  }

  pwd->shell = NULL;
  pwd->uid = -1;
  pwd->gid = -1;

  return 0;
}


int uv_os_get_passwd(uv_passwd_t* pwd) {
  return uv__getpwuid_r(pwd);
}


int uv_os_environ(uv_env_item_t** envitems, int* count) {
  wchar_t* env;
  wchar_t* penv;
  int i, cnt;
  uv_env_item_t* envitem;

  *envitems = NULL;
  *count = 0;

  env = GetEnvironmentStringsW();
  if (env == NULL)
    return 0;

  for (penv = env, i = 0; *penv != L'\0'; penv += wcslen(penv) + 1, i++);

  *envitems = uv__calloc(i, sizeof(**envitems));
  if (*envitems == NULL) {
    FreeEnvironmentStringsW(env);
    return UV_ENOMEM;
  }

  penv = env;
  cnt = 0;

  while (*penv != L'\0' && cnt < i) {
    char* buf;
    char* ptr;

    if (uv__convert_utf16_to_utf8(penv, -1, &buf) != 0)
      goto fail;

    /* Using buf + 1 here because we know that `buf` has length at least 1,
     * and some special environment variables on Windows start with a = sign. */
    ptr = strchr(buf + 1, '=');
    if (ptr == NULL) {
      uv__free(buf);
      goto do_continue;
    }

    *ptr = '\0';

    envitem = &(*envitems)[cnt];
    envitem->name = buf;
    envitem->value = ptr + 1;

    cnt++;

  do_continue:
    penv += wcslen(penv) + 1;
  }

  FreeEnvironmentStringsW(env);

  *count = cnt;
  return 0;

fail:
  FreeEnvironmentStringsW(env);

  for (i = 0; i < cnt; i++) {
    envitem = &(*envitems)[cnt];
    uv__free(envitem->name);
  }
  uv__free(*envitems);

  *envitems = NULL;
  *count = 0;
  return UV_ENOMEM;
}


int uv_os_getenv(const char* name, char* buffer, size_t* size) {
  wchar_t fastvar[512];
  wchar_t* var;
  DWORD varlen;
  wchar_t* name_w;
  DWORD bufsize;
  size_t len;
  int r;

  if (name == NULL || buffer == NULL || size == NULL || *size == 0)
    return UV_EINVAL;

  r = uv__convert_utf8_to_utf16(name, -1, &name_w);

  if (r != 0)
    return r;

  var = fastvar;
  varlen = ARRAY_SIZE(fastvar);

  for (;;) {
    SetLastError(ERROR_SUCCESS);
    len = GetEnvironmentVariableW(name_w, var, varlen);

    if (len < varlen)
      break;

    /* Try repeatedly because we might have been preempted by another thread
     * modifying the environment variable just as we're trying to read it.
     */
    if (var != fastvar)
      uv__free(var);

    varlen = 1 + len;
    var = uv__malloc(varlen * sizeof(*var));

    if (var == NULL) {
      r = UV_ENOMEM;
      goto fail;
    }
  }

  uv__free(name_w);
  name_w = NULL;

  if (len == 0) {
    r = GetLastError();
    if (r != ERROR_SUCCESS) {
      r = uv_translate_sys_error(r);
      goto fail;
    }
  }

  /* Check how much space we need */
  bufsize = WideCharToMultiByte(CP_UTF8, 0, var, -1, NULL, 0, NULL, NULL);

  if (bufsize == 0) {
    r = uv_translate_sys_error(GetLastError());
    goto fail;
  } else if (bufsize > *size) {
    *size = bufsize;
    r = UV_ENOBUFS;
    goto fail;
  }

  /* Convert to UTF-8 */
  bufsize = WideCharToMultiByte(CP_UTF8,
                                0,
                                var,
                                -1,
                                buffer,
                                *size,
                                NULL,
                                NULL);

  if (bufsize == 0) {
    r = uv_translate_sys_error(GetLastError());
    goto fail;
  }

  *size = bufsize - 1;
  r = 0;

fail:

  if (name_w != NULL)
    uv__free(name_w);

  if (var != fastvar)
    uv__free(var);

  return r;
}


int uv_os_setenv(const char* name, const char* value) {
  wchar_t* name_w;
  wchar_t* value_w;
  int r;

  if (name == NULL || value == NULL)
    return UV_EINVAL;

  r = uv__convert_utf8_to_utf16(name, -1, &name_w);

  if (r != 0)
    return r;

  r = uv__convert_utf8_to_utf16(value, -1, &value_w);

  if (r != 0) {
    uv__free(name_w);
    return r;
  }

  r = SetEnvironmentVariableW(name_w, value_w);
  uv__free(name_w);
  uv__free(value_w);

  if (r == 0)
    return uv_translate_sys_error(GetLastError());

  return 0;
}


int uv_os_unsetenv(const char* name) {
  wchar_t* name_w;
  int r;

  if (name == NULL)
    return UV_EINVAL;

  r = uv__convert_utf8_to_utf16(name, -1, &name_w);

  if (r != 0)
    return r;

  r = SetEnvironmentVariableW(name_w, NULL);
  uv__free(name_w);

  if (r == 0)
    return uv_translate_sys_error(GetLastError());

  return 0;
}


int uv_os_gethostname(char* buffer, size_t* size) {
  WCHAR buf[UV_MAXHOSTNAMESIZE];
  size_t len;
  char* utf8_str;
  int convert_result;

  if (buffer == NULL || size == NULL || *size == 0)
    return UV_EINVAL;

  uv__once_init(); /* Initialize winsock */

  if (GetHostNameW(buf, UV_MAXHOSTNAMESIZE) != 0)
    return uv_translate_sys_error(WSAGetLastError());

  convert_result = uv__convert_utf16_to_utf8(buf, -1, &utf8_str);

  if (convert_result != 0)
    return convert_result;

  len = strlen(utf8_str);
  if (len >= *size) {
    *size = len + 1;
    uv__free(utf8_str);
    return UV_ENOBUFS;
  }

  memcpy(buffer, utf8_str, len + 1);
  uv__free(utf8_str);
  *size = len;
  return 0;
}


static int uv__get_handle(uv_pid_t pid, int access, HANDLE* handle) {
  int r;

  if (pid == 0)
    *handle = GetCurrentProcess();
  else
    *handle = OpenProcess(access, FALSE, pid);

  if (*handle == NULL) {
    r = GetLastError();

    if (r == ERROR_INVALID_PARAMETER)
      return UV_ESRCH;
    else
      return uv_translate_sys_error(r);
  }

  return 0;
}


int uv_os_getpriority(uv_pid_t pid, int* priority) {
  HANDLE handle;
  int r;

  if (priority == NULL)
    return UV_EINVAL;

  r = uv__get_handle(pid, PROCESS_QUERY_LIMITED_INFORMATION, &handle);

  if (r != 0)
    return r;

  r = GetPriorityClass(handle);

  if (r == 0) {
    r = uv_translate_sys_error(GetLastError());
  } else {
    /* Map Windows priority classes to Unix nice values. */
    if (r == REALTIME_PRIORITY_CLASS)
      *priority = UV_PRIORITY_HIGHEST;
    else if (r == HIGH_PRIORITY_CLASS)
      *priority = UV_PRIORITY_HIGH;
    else if (r == ABOVE_NORMAL_PRIORITY_CLASS)
      *priority = UV_PRIORITY_ABOVE_NORMAL;
    else if (r == NORMAL_PRIORITY_CLASS)
      *priority = UV_PRIORITY_NORMAL;
    else if (r == BELOW_NORMAL_PRIORITY_CLASS)
      *priority = UV_PRIORITY_BELOW_NORMAL;
    else  /* IDLE_PRIORITY_CLASS */
      *priority = UV_PRIORITY_LOW;

    r = 0;
  }

  CloseHandle(handle);
  return r;
}


int uv_os_setpriority(uv_pid_t pid, int priority) {
  HANDLE handle;
  int priority_class;
  int r;

  /* Map Unix nice values to Windows priority classes. */
  if (priority < UV_PRIORITY_HIGHEST || priority > UV_PRIORITY_LOW)
    return UV_EINVAL;
  else if (priority < UV_PRIORITY_HIGH)
    priority_class = REALTIME_PRIORITY_CLASS;
  else if (priority < UV_PRIORITY_ABOVE_NORMAL)
    priority_class = HIGH_PRIORITY_CLASS;
  else if (priority < UV_PRIORITY_NORMAL)
    priority_class = ABOVE_NORMAL_PRIORITY_CLASS;
  else if (priority < UV_PRIORITY_BELOW_NORMAL)
    priority_class = NORMAL_PRIORITY_CLASS;
  else if (priority < UV_PRIORITY_LOW)
    priority_class = BELOW_NORMAL_PRIORITY_CLASS;
  else
    priority_class = IDLE_PRIORITY_CLASS;

  r = uv__get_handle(pid, PROCESS_SET_INFORMATION, &handle);

  if (r != 0)
    return r;

  if (SetPriorityClass(handle, priority_class) == 0)
    r = uv_translate_sys_error(GetLastError());

  CloseHandle(handle);
  return r;
}


int uv_os_uname(uv_utsname_t* buffer) {
  /* Implementation loosely based on
     https://github.com/gagern/gnulib/blob/master/lib/uname.c */
  OSVERSIONINFOW os_info;
  SYSTEM_INFO system_info;
  HKEY registry_key;
  WCHAR product_name_w[256];
  DWORD product_name_w_size;
  int version_size;
  int processor_level;
  int r;

  if (buffer == NULL)
    return UV_EINVAL;

  uv__once_init();
  os_info.dwOSVersionInfoSize = sizeof(os_info);
  os_info.szCSDVersion[0] = L'\0';

  /* Try calling RtlGetVersion(), and fall back to the deprecated GetVersionEx()
     if RtlGetVersion() is not available. */
  if (pRtlGetVersion) {
    pRtlGetVersion(&os_info);
  } else {
    /* Silence GetVersionEx() deprecation warning. */
    #ifdef _MSC_VER
    #pragma warning(suppress : 4996)
    #endif
    if (GetVersionExW(&os_info) == 0) {
      r = uv_translate_sys_error(GetLastError());
      goto error;
    }
  }

  /* Populate the version field. */
  version_size = 0;
  r = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                    0,
                    KEY_QUERY_VALUE,
                    &registry_key);

  if (r == ERROR_SUCCESS) {
    product_name_w_size = sizeof(product_name_w);
    r = RegGetValueW(registry_key,
                     NULL,
                     L"ProductName",
                     RRF_RT_REG_SZ,
                     NULL,
                     (PVOID) product_name_w,
                     &product_name_w_size);
    RegCloseKey(registry_key);

    if (r == ERROR_SUCCESS) {
      version_size = WideCharToMultiByte(CP_UTF8,
                                         0,
                                         product_name_w,
                                         -1,
                                         buffer->version,
                                         sizeof(buffer->version),
                                         NULL,
                                         NULL);
      if (version_size == 0) {
        r = uv_translate_sys_error(GetLastError());
        goto error;
      }
    }
  }

  /* Append service pack information to the version if present. */
  if (os_info.szCSDVersion[0] != L'\0') {
    if (version_size > 0)
      buffer->version[version_size - 1] = ' ';

    if (WideCharToMultiByte(CP_UTF8,
                            0,
                            os_info.szCSDVersion,
                            -1,
                            buffer->version + version_size,
                            sizeof(buffer->version) - version_size,
                            NULL,
                            NULL) == 0) {
      r = uv_translate_sys_error(GetLastError());
      goto error;
    }
  }

  /* Populate the sysname field. */
#ifdef __MINGW32__
  r = snprintf(buffer->sysname,
               sizeof(buffer->sysname),
               "MINGW32_NT-%u.%u",
               (unsigned int) os_info.dwMajorVersion,
               (unsigned int) os_info.dwMinorVersion);
  assert((size_t)r < sizeof(buffer->sysname));
#else
  uv__strscpy(buffer->sysname, "Windows_NT", sizeof(buffer->sysname));
#endif

  /* Populate the release field. */
  r = snprintf(buffer->release,
               sizeof(buffer->release),
               "%d.%d.%d",
               (unsigned int) os_info.dwMajorVersion,
               (unsigned int) os_info.dwMinorVersion,
               (unsigned int) os_info.dwBuildNumber);
  assert((size_t)r < sizeof(buffer->release));

  /* Populate the machine field. */
  GetSystemInfo(&system_info);

  switch (system_info.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
      uv__strscpy(buffer->machine, "x86_64", sizeof(buffer->machine));
      break;
    case PROCESSOR_ARCHITECTURE_IA64:
      uv__strscpy(buffer->machine, "ia64", sizeof(buffer->machine));
      break;
    case PROCESSOR_ARCHITECTURE_INTEL:
      uv__strscpy(buffer->machine, "i386", sizeof(buffer->machine));

      if (system_info.wProcessorLevel > 3) {
        processor_level = system_info.wProcessorLevel < 6 ?
                          system_info.wProcessorLevel : 6;
        buffer->machine[1] = '0' + processor_level;
      }

      break;
    case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
      uv__strscpy(buffer->machine, "i686", sizeof(buffer->machine));
      break;
    case PROCESSOR_ARCHITECTURE_MIPS:
      uv__strscpy(buffer->machine, "mips", sizeof(buffer->machine));
      break;
    case PROCESSOR_ARCHITECTURE_ALPHA:
    case PROCESSOR_ARCHITECTURE_ALPHA64:
      uv__strscpy(buffer->machine, "alpha", sizeof(buffer->machine));
      break;
    case PROCESSOR_ARCHITECTURE_PPC:
      uv__strscpy(buffer->machine, "powerpc", sizeof(buffer->machine));
      break;
    case PROCESSOR_ARCHITECTURE_SHX:
      uv__strscpy(buffer->machine, "sh", sizeof(buffer->machine));
      break;
    case PROCESSOR_ARCHITECTURE_ARM:
      uv__strscpy(buffer->machine, "arm", sizeof(buffer->machine));
      break;
    default:
      uv__strscpy(buffer->machine, "unknown", sizeof(buffer->machine));
      break;
  }

  return 0;

error:
  buffer->sysname[0] = '\0';
  buffer->release[0] = '\0';
  buffer->version[0] = '\0';
  buffer->machine[0] = '\0';
  return r;
}

int uv_gettimeofday(uv_timeval64_t* tv) {
  /* Based on https://doxygen.postgresql.org/gettimeofday_8c_source.html */
  const uint64_t epoch = (uint64_t) 116444736000000000ULL;
  FILETIME file_time;
  ULARGE_INTEGER ularge;

  if (tv == NULL)
    return UV_EINVAL;

  GetSystemTimeAsFileTime(&file_time);
  ularge.LowPart = file_time.dwLowDateTime;
  ularge.HighPart = file_time.dwHighDateTime;
  tv->tv_sec = (int64_t) ((ularge.QuadPart - epoch) / 10000000L);
  tv->tv_usec = (int32_t) (((ularge.QuadPart - epoch) % 10000000L) / 10);
  return 0;
}

int uv__random_rtlgenrandom(void* buf, size_t buflen) {
  if (buflen == 0)
    return 0;

  if (SystemFunction036(buf, buflen) == FALSE)
    return UV_EIO;

  return 0;
}

void uv_sleep(unsigned int msec) {
  Sleep(msec);
}
