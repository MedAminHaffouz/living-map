/* mirrors targets/writer_pi/ros2_ws/src/lm_interfaces/srv/Calibrate.srv (checked by tests/test_writer_stm.py) */
#ifndef LM_INTERFACES__SRV__CALIBRATE_H_
#define LM_INTERFACES__SRV__CALIBRATE_H_
#include <stdint.h>
#include <stdbool.h>
typedef struct { uint8_t target; uint8_t op; } lm_interfaces__srv__Calibrate_Request;
typedef struct { bool ok; } lm_interfaces__srv__Calibrate_Response;
#endif
