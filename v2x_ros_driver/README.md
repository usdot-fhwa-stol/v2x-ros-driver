# v2x_ros_driver

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

2. Run the Docker image

```sh
docker run -it --network host usdotfhwastol/v2x-ros-driver:<latest_release_tag_from_github>
```

### Build from source

#### Note: Assumption here is that user is building on a ros2 humble development environment

3. Clone the repository into workspace

```sh
git clone https://github.com/usdot-fhwa-stol/v2x-ros-driver.git
```

4. Clone the dependencies into workspace

```sh
chmod +x <path_to_workspace>/docker/checkout.bash
./<path_to_workspace>/docker/checkout.bash -r <path_to_workspace> -b <latest_release_tag_from_github>
```

5. Build the package

```sh
source /opt/ros/humble/setup.bash
colcon build --packages-up-to v2x_ros_driver
```

6. Launch the node

```sh
source <path_to_package_install_directory>/install/setup.bash
ros2 launch v2x_ros_driver v2x_ros_driver.launch.py
```
