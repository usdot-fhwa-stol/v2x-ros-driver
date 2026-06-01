# v2x-ros-driver

| CI Build Status | Docker Build Status | DockerHub Build Status | Sonar Code Quality |
| --- | --- | --- | --- |
| [![CI](https://github.com/usdot-fhwa-stol/v2x-ros-driver/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/usdot-fhwa-stol/v2x-ros-driver/actions/workflows/ci.yml) | [![Docker Build](https://github.com/usdot-fhwa-stol/v2x-ros-driver/actions/workflows/docker.yml/badge.svg)](https://github.com/usdot-fhwa-stol/v2x-ros-driver/actions/workflows/docker.yml) | [![Dockerhub Build](https://github.com/usdot-fhwa-stol/v2x-ros-driver/actions/workflows/dockerhub.yml/badge.svg)](https://github.com/usdot-fhwa-stol/v2x-ros-driver/actions/workflows/dockerhub.yml) | [![SonarCloud Quality Gate](https://sonarcloud.io/api/project_badges/measure?project=usdot-fhwa-stol_v2x-ros-driver&metric=alert_status)](https://sonarcloud.io/dashboard?id=usdot-fhwa-stol_v2x-ros-driver) |

v2x-ros-driver is the driver for use with V2X radios and has been tested with the following OBUs: Cohda (MK5/MK6C/MK6), Commsignia, Kapsch, Ettifos. It provides the bidirectional communication between ROS and a connected V2X radio (both OBUs and RSUs) using [carma_driver_msgs/msg/ByteArray](https://github.com/usdot-fhwa-stol/carma-msgs/blob/develop/carma_driver_msgs/msg/ByteArray.msg) ROS messages carrying UPER-encoded V2X data.

For more information regarding the recommended setup/configuration for a Cohda MK6 OBU, please refer to this [documentation](/docs/).

---

## Related Repositories

### v2x-ros-conversion

The [v2x-ros-conversion repository](https://github.com/usdot-fhwa-stol/v2x-ros-conversion) contains two ROS nodes (J2735 Convertor and Message) for handling the conversion between decoded (i.e., structured, human readable) V2X ROS messages and carma_driver_msgs/msg/ByteArray messages containing UPER-encoded V2X data.

### carma-msgs

The [carma-msgs repository](https://github.com/usdot-fhwa-stol/carma-msgs) contains various message types used by CARMA Platform, including those necessary for v2x-ros-driver (Ex: carma_driver_msgs/msg/ByteArray).

### CARMA Platform

[CARMA Platform](https://github.com/usdot-fhwa-stol/carma-platform) is an FWHA run open source software that enables researches and engineers to develop and test their Cooperative Driving Automation (CDA) features on suffciently equipped vehicles. It is built on top of ROS and allows users to install custom plugins for tactical planning of vehicle behaviors and low-level motion planning. v2x-ros-driver was initially developed for CARMA Platform.

### CDA 1Tenth

[CDA 1Tenth](https://github.com/usdot-fhwa-stol/cda1tenth-bringup/tree/develop) is a low cost research platform for developing and testing Cooperative Driving Automation (CDA) on small scale vehicles and infrastructure.

### v2x-ros-emulator

The [v2x-ros-emulator repository](https://github.com/usdot-fhwa-stol/v2x-emulator) enables Raspberry Pi’s to act as mock OBUs and RSUs for the CDA1tenth platform.

## Contribution

Welcome to the CARMA contributing guide. Please read this guide to learn about our development process, how to propose pull requests and improvements, and how to build and test your changes to this project. [CARMA Contributing Guide](https://github.com/usdot-fhwa-stol/carma-platform/blob/develop/Contributing.md)

## Code of Conduct

Please read our [CARMA Code of Conduct](https://github.com/usdot-fhwa-stol/carma-platform/blob/develop/Code_of_Conduct.md) which outlines our expectations for participants within the CARMA community, as well as steps to reporting unacceptable behavior. We are committed to providing a welcoming and inspiring community for all and expect our code of conduct to be honored. Anyone who violates this code of conduct may be banned from the community.

## Attribution

The development team would like to acknowledge the people who have made direct contributions to the design and code in this repository. [CARMA Attribution](https://github.com/usdot-fhwa-stol/carma-platform/blob/develop/ATTRIBUTION.txt)

## License

By contributing to the Federal Highway Administration (FHWA) Connected Automated Research Mobility Applications (CARMA), you agree that your contributions will be licensed under its Apache License 2.0 license. [CARMA License](https://github.com/usdot-fhwa-stol/carma-platform/blob/develop/docs/License.md)

## Contact

Please click on the CARMA logo below to visit the Federal Highway Adminstration(FHWA) CARMA website. For more information, contact <CAVSupportServices@dot.gov>.

[![CARMA Image](https://raw.githubusercontent.com/usdot-fhwa-stol/carma-platform/develop/docs/image/CARMA_icon.png)](https://highways.dot.gov/research/research-programs/operations/CARMA)
