/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RRPC_SERVER_H
#define RRPC_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rcommon.h"

/* ------------------------------- Macros ------------------------------------*/

/* ------------------------------- APIs ------------------------------------*/
#define RECYCLE_REQUESTS                1

/* === Server === */
typedef struct _ServerRequest ServerRequest;
typedef struct _ServerConnection ServerConnection;
struct _ServerRequest
{
  uint32_t request_id;                  /* in little-endian */
  uint32_t method_index;                /* in native-endian */
  ServerConnection *conn;
  ProtobufC_RPC_Server *server;
  union {
    /* if conn != NULL, then the request is alive: */
    struct { ServerRequest *prev, *next; } alive;

    /* if conn == NULL, then the request is defunct: */
    struct { ProtobufCAllocator *allocator; } defunct;

    /* well, if it is in the recycled list, then it's recycled :/ */
    struct { ServerRequest *next; } recycled;
  } info;
};
struct _ServerConnection
{
  int fd;
  ProtobufCRPCDataBuffer incoming, outgoing;

  ProtobufC_RPC_Server *server;
  ServerConnection *prev, *next;

  unsigned n_pending_requests;
  ServerRequest *first_pending_request, *last_pending_request;
};


/* When we get a response in the wrong thread,
   we proxy it over a system pipe.  Actually, we allocate one
   of these structures and pass the pointer over the pipe.  */
typedef struct _ProxyResponse ProxyResponse;
struct _ProxyResponse
{
  ServerRequest *request;
  unsigned len;
  /* data follows the structure */
};

struct _ProtobufC_RPC_Server
{
  ProtobufCRPCDispatch *dispatch;
  ProtobufCAllocator *allocator;
  ProtobufCService *underlying;
  ProtobufC_RPC_AddressType address_type;
  char *bind_name;
  ServerConnection *first_connection, *last_connection;
  ProtobufC_RPC_FD listening_fd;

  ServerRequest *recycled_requests;

  /* multithreading support */
  ProtobufC_RPC_IsRpcThreadFunc is_rpc_thread_func;
  void * is_rpc_thread_data;
  int proxy_pipe[2];
  unsigned proxy_extra_data_len;
  uint8_t proxy_extra_data[sizeof (void*)];

  ProtobufC_RPC_Error_Func error_handler;
  void *error_handler_data;

  ProtobufC_RPC_Protocol rpc_protocol;

  /* configuration */
  unsigned max_pending_requests_per_connection;
};


#ifdef __cplusplus
}
#endif

#endif //RRPC_SERVER_H
