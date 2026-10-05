#ifndef GEOMETRY_MSGS__MSG__TWIST_H_
#define GEOMETRY_MSGS__MSG__TWIST_H_
typedef struct { double x, y, z; } geometry_msgs__msg__Vector3;
typedef struct { geometry_msgs__msg__Vector3 linear, angular; } geometry_msgs__msg__Twist;
#endif
