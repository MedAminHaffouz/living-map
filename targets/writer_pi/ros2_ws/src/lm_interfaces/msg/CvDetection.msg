# Pi-internal. CV module output (person detector), before pose-tagging.
builtin_interfaces/Time stamp
float32 conf
float32 bearing_rad   # from bbox centre + camera intrinsics, robot frame
uint8 posture         # 0 unknown, 1 standing, 2 sitting, 3 lying
float32[4] bbox       # x, y, w, h (px)
