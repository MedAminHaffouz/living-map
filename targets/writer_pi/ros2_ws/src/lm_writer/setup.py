from setuptools import setup
NODES = ["stm_adapter", "cv_module", "event_detection", "priority_decision",
         "beacon_writer", "beacon_dropper", "map_processing"]
setup(name="lm_writer", version="0.1.0", packages=["lm_writer"],
      data_files=[("share/ament_index/resource_index/packages", ["resource/lm_writer"]), ("share/lm_writer", ["package.xml"])],
      install_requires=["setuptools"], zip_safe=True,
      entry_points={"console_scripts": [f"{n} = lm_writer.{n}:main" for n in NODES]})
