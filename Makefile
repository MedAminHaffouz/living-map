ROS_MSG := targets/writer_pi/ros2_ws/src/lm_interfaces/msg
.PHONY: gen test sim ros clean
gen:            ## schema.yaml -> C / Python / ROS msgs (+ copy into lm_interfaces)
	python3 contracts/codegen.py
	cp contracts/generated/ros/msg/*.msg $(ROS_MSG)/
test: gen       ## contracts cross-check, firmware host builds, ONA/CP e2e, sim graph
	python3 -m pytest -q tests
	cd sim && python3 -m pytest -q tests
sim:            ## Phase 1 simulation (whole mission, all agents, one process)
	cd sim && python3 run.py
ros: gen        ## build the Writer Pi workspace (on the Pi / ROS 2 machine)
	cd targets/writer_pi/ros2_ws && colcon build --symlink-install
clean:
	find . -name __pycache__ -prune -exec rm -rf {} +; rm -rf targets/writer_pi/ros2_ws/{build,install,log}
