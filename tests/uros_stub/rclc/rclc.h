#pragma once
#include <rcl/rcl.h>
typedef struct { rcl_context_t context; rcl_allocator_t *allocator; } rclc_support_t;
rcl_ret_t rclc_support_init(rclc_support_t *support, int argc, char const *const *argv, rcl_allocator_t *allocator);
rcl_ret_t rclc_support_fini(rclc_support_t *support);
rcl_ret_t rclc_node_init_default(rcl_node_t *node, const char *name, const char *namespace_, rclc_support_t *support);
rcl_ret_t rclc_publisher_init_default(rcl_publisher_t *publisher, const rcl_node_t *node, const rosidl_message_type_support_t *type_support, const char *topic_name);
rcl_ret_t rclc_publisher_init_best_effort(rcl_publisher_t *publisher, const rcl_node_t *node, const rosidl_message_type_support_t *type_support, const char *topic_name);
rcl_ret_t rclc_subscription_init_default(rcl_subscription_t *subscription, const rcl_node_t *node, const rosidl_message_type_support_t *type_support, const char *topic_name);
rcl_ret_t rclc_service_init_default(rcl_service_t *service, const rcl_node_t *node, const rosidl_service_type_support_t *type_support, const char *service_name);
