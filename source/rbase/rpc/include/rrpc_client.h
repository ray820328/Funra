/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RRPC_CLIENT_H
#define RRPC_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rcommon.h"

/* ------------------------------- Macros ------------------------------------*/

/* ------------------------------- APIs ------------------------------------*/


#define UINT_TO_POINTER(ui)      ((void*)(uintptr_t)(ui))
#define POINTER_TO_UINT(ptr)     ((unsigned)(uintptr_t)(ptr))

#define MAX_FAILED_MSG_LENGTH   512

typedef enum
{
  PROTOBUF_C_RPC_CLIENT_STATE_INIT,
  PROTOBUF_C_RPC_CLIENT_STATE_NAME_LOOKUP,
  PROTOBUF_C_RPC_CLIENT_STATE_CONNECTING,
  PROTOBUF_C_RPC_CLIENT_STATE_CONNECTED,
  PROTOBUF_C_RPC_CLIENT_STATE_FAILED_WAITING,
  PROTOBUF_C_RPC_CLIENT_STATE_FAILED,               /* if no autoreconnect */
  PROTOBUF_C_RPC_CLIENT_STATE_DESTROYED
} ProtobufC_RPC_ClientState;

typedef struct _Closure Closure;
struct _Closure
{
  /* these will be NULL for unallocated request ids */
  const ProtobufCMessageDescriptor *response_type;
  ProtobufCClosure closure;

  /* this is the next request id, or 0 for none */
  void *closure_data;
};


struct _ProtobufC_RPC_Client
{
  ProtobufCService base_service;
  ProtobufCRPCDataBuffer incoming;
  ProtobufCRPCDataBuffer outgoing;
  ProtobufCAllocator *allocator;
  ProtobufCRPCDispatch *dispatch;
  ProtobufC_RPC_AddressType address_type;
  char *name;
  ProtobufC_RPC_FD fd;
  protobuf_c_boolean autoreconnect;
  unsigned autoreconnect_millis;
  ProtobufC_RPC_NameLookup_Func resolver;
  ProtobufC_RPC_Error_Func error_handler;
  void *error_handler_data;
  ProtobufC_RPC_Protocol rpc_protocol;
  ProtobufC_RPC_ClientState state;
  union {
    struct {
      ProtobufCRPCDispatchIdle *idle;
    } init;
    struct {
      protobuf_c_boolean pending;
      protobuf_c_boolean destroyed_while_pending;
      uint16_t port;
    } name_lookup;
    struct {
      unsigned closures_alloced;
      unsigned first_free_request_id;
      /* indexed by (request_id-1) */
      Closure *closures;
    } connected;
    struct {
      ProtobufCRPCDispatchTimer *timer;
      char *error_message;
    } failed_waiting;
    struct {
      char *error_message;
    } failed;
  } info;
};


#ifdef __cplusplus
}
#endif

#endif //RRPC_CLIENT_H
