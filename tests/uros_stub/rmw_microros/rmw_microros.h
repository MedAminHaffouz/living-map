#pragma once
#include <rcl/rcl.h>
rmw_ret_t rmw_uros_ping_agent(const int timeout_ms, const uint8_t attempts);
rmw_ret_t rmw_uros_set_context_entity_destroy_session_timeout(rmw_context_t *context, const int64_t session_timeout);
