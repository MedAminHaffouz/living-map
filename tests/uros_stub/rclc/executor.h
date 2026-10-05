#pragma once
#include <rcl/rcl.h>
typedef struct { void *handles; } rclc_executor_t;
typedef enum { ON_NEW_DATA, ALWAYS } rclc_executor_handle_invocation_t;
typedef void (*rclc_subscription_callback_t)(const void *);
typedef void (*rclc_service_callback_t)(const void *, void *);
rclc_executor_t rclc_executor_get_zero_initialized_executor(void);
rcl_ret_t rclc_executor_init(rclc_executor_t *executor, rcl_context_t *context, const size_t number_of_handles, const rcl_allocator_t *allocator);
rcl_ret_t rclc_executor_add_subscription(rclc_executor_t *executor, rcl_subscription_t *subscription, void *msg, rclc_subscription_callback_t callback, rclc_executor_handle_invocation_t invocation);
rcl_ret_t rclc_executor_add_service(rclc_executor_t *executor, rcl_service_t *service, void *request_msg, void *response_msg, rclc_service_callback_t callback);
rcl_ret_t rclc_executor_spin_some(rclc_executor_t *executor, const uint64_t timeout_ns);
rcl_ret_t rclc_executor_fini(rclc_executor_t *executor);
