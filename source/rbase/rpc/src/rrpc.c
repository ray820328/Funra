/** 
 * Copyright (c) 2016
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 * 
 * rrpc设计遵循以下规则:
 * 1, 幂等性: RequestID标记一个逻辑请求，重复发多次仅执行一次，小于当前处理id的返回错误
 * 2, 兼容性（前后）: 如向后兼容考虑远程实参默认值等
 * 3, 错误返回: 分接口响应和逻辑响应，接口rpc阶段发生错误时直接返回rpc错误，不要把code放在逻辑响应里面（方便重试/熔断/降级）
 * 4, 超时控制: 设置默认超时，调用接口的客户端设置真实超时
 * 5, 限流: 服务端需要实现本地限流，获取用户信息，为该用户创建一个限流器
 * 6, 重试: 基于wrapper函数/拦截器拦截code，实现重试
 * 其他: 
 * 负载均衡分流，限制连接数，限制最大请求/响应+分页长度，监控（针对每个接口获取度量指标模板，在调用完成后统计数据上报metrics）
 * 每个rpc方法的定义中都必须包含一个返回值，并且返回值不能为空 【不约束】
 */

#include "rrpc.h"

static int rrpc_register_node(rrpc_service_type_t service_type, ripc_data_source_t* ds) {
    int ret_code = rcode_ok;

    return ret_code;
}

static int rrpc_on_node_register(rrpc_service_type_t service_type, ripc_data_source_t* ds) {
    int ret_code = rcode_ok;

    return ret_code;
}

static int rrpc_on_node_unregister(rrpc_service_type_t service_type, ripc_data_source_t* ds) {
    int ret_code = rcode_ok;

    return ret_code;
}

static int rrpc_call_service(rrpc_service_type_t service_type, void* params) {
    int ret_code = rcode_ok;

    return ret_code;
}
