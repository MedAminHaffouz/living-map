"""CV module: camera -> person detector (YOLOv8n NCNN/TFLite) -> conf threshold -> k-of-n track -> /cv/detections.
Posture from box aspect ratio (P1) or keypoints (P2). Bearing from bbox centre + intrinsics.
TODO(owner): model loading, camera capture, tracker. Interface is fixed: publish CvDetection."""
import rclpy
from rclpy.node import Node
import lm_interfaces.msg as R

class CvModule(Node):
    def __init__(self):
        super().__init__("cv_module")
        self.conf_th = self.declare_parameter("conf_threshold", 0.5).value
        self.k, self.n = self.declare_parameter("k", 3).value, self.declare_parameter("n", 5).value
        self.pub = self.create_publisher(R.CvDetection, "/cv/detections", 10)
        self.create_timer(1.0 / self.declare_parameter("fps", 5.0).value, self.tick)
    def tick(self):
        pass  # TODO: grab frame -> infer -> filter -> track -> self.pub.publish(CvDetection(...))

def main():
    rclpy.init(); rclpy.spin(CvModule())
