# v2x_ros_driver

This package defines a carma-plaform compatible driver for Cohda OBUs though other models of OBU may work as well. The driver is compatible with C-V2X radios. The design documents for this driver can be found [here](https://usdot-carma.atlassian.net/wiki/spaces/CRMMSG/pages/1319272562/Detailed+Design+-+V2X+ROS+Driver).

This driver is known to work successfully with the Cohda Wireless MK5 and MK6 OBU.  It implements the communication interface allowing UPER-encoded SAE J2735 communication to be exposed to a ROS network using [carma_driver_msgs/msg/ByteArray](https://github.com/usdot-fhwa-stol/carma-msgs/blob/develop/carma_driver_msgs/msg/ByteArray.msg). It has been tested with SAE J2735 2016 but can support later versions as well.

## Deployment Instructions

The v2x-ros-driver is a ros2 package currently implemented in [ros2-humble](https://docs.ros.org/en/humble/Installation.html). It creates a UDP client to listen to input from the v2x radio and broadcast the UPER encoded message to ROS.
In order to deploy the driver in a ros2-humble configured environment, the following steps can be used

### Deploy using docker (recommended)

1. Pull the latest docker image for driver from dockerhub

```sh
docker pull usdotfhwastol/v2x-ros-driver:<latest-release-tag>
```

*Latest release tag can be obtained from github tags. Docker images are tagged with the same tag.*

#### Note: The repository was recently renamed and until a new release is available, users may use usdotfhwastol/carma-cohda-dsrc-driver:carma-system-4.5.0 which builds an older versions of this code base but is manufacturer agnostic

1. Run the Docker image

```sh
docker run -it --network host usdotfhwastol/v2x-ros-driver:<latest_release_tag_from_github>
```

### Build from source

#### Note: Assumption here is that user is building on a ros2 humble development environment

1. Clone the repository into workspace

```sh
git clone https://github.com/usdot-fhwa-stol/v2x-ros-driver.git
```

1. Clone the dependencies into workspace

```sh
chmod +x <path_to_workspace>/docker/checkout.bash
./<path_to_workspace>/docker/checkout.bash -r <path_to_workspace> -b <latest_release_tag_from_github>
```

1. Build the package

```sh
source /opt/ros/humble/setup.bash
colcon build --packages-up-to v2x_ros_driver
```

1. Launch the node

```sh
source <path_to_package_install_directory>/install/setup.bash
ros2 launch v2x_ros_driver v2x_ros_driver.launch.py
```
