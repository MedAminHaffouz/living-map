# Pi-internal (hand-written). Event Detection output, pose-tagged in frame W (= slam_toolbox map frame, origin B0).
builtin_interfaces/Time stamp
uint16 id
uint8 type        # EventType
float32 severity  # [0,1]
float32 conf      # [0,1]
float32 x
float32 y
uint8[] co_events # other EventTypes seen at the same spot
