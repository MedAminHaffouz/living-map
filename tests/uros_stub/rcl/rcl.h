#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef int32_t rcl_ret_t;
typedef int32_t rmw_ret_t;
#define RCL_RET_OK 0
#define RMW_RET_OK 0
#define RCL_MS_TO_NS(ms) ((ms) * 1000000LL)
typedef struct { void *state; } rcl_allocator_t;
typedef struct { void *impl; } rcl_context_t;
typedef struct { void *impl; } rcl_node_t;
typedef struct { void *impl; } rcl_publisher_t;
typedef struct { void *impl; } rcl_subscription_t;
typedef struct { void *impl; } rcl_service_t;
typedef struct rmw_context_s rmw_context_t;
typedef struct rmw_publisher_allocation_s rmw_publisher_allocation_t;
typedef struct rosidl_message_type_support_t rosidl_message_type_support_t;
typedef struct rosidl_service_type_support_t rosidl_service_type_support_t;
#define ROSIDL_GET_MSG_TYPE_SUPPORT(pkg, sub, name) ((const rosidl_message_type_support_t *)0)
#define ROSIDL_GET_SRV_TYPE_SUPPORT(pkg, sub, name) ((const rosidl_service_type_support_t *)0)
rcl_allocator_t rcl_get_default_allocator(void);
rcl_node_t rcl_get_zero_initialized_node(void);
rcl_publisher_t rcl_get_zero_initialized_publisher(void);
rcl_subscription_t rcl_get_zero_initialized_subscription(void);
rcl_service_t rcl_get_zero_initialized_service(void);
rcl_ret_t rcl_publish(const rcl_publisher_t *publisher, const void *ros_message, rmw_publisher_allocation_t *allocation);
rcl_ret_t rcl_publisher_fini(rcl_publisher_t *publisher, rcl_node_t *node);
rcl_ret_t rcl_subscription_fini(rcl_subscription_t *subscription, rcl_node_t *node);
rcl_ret_t rcl_service_fini(rcl_service_t *service, rcl_node_t *node);
rcl_ret_t rcl_node_fini(rcl_node_t *node);
rmw_context_t *rcl_context_get_rmw_context(rcl_context_t *context);
