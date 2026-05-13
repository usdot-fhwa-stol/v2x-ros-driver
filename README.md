| CI Build Status | Sonar Code Quality |  Unit Tests Workflow | DockerHub Image Build |
|------|-----|-----|-----|
[![CI](https://github.com/usdot-fhwa-stol/v2x-ros-driver/actions/workflows/ci.yml/badge.svg)](https://github.com/usdot-fhwa-stol/v2x-ros-driver/actions/workflows/ci.yml) | [![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=usdot-fhwa-stol_v2x-ros-driver&metric=alert_status)](https://sonarcloud.io/dashboard?id=usdot-fhwa-stol_v2x-ros-driver) | [![Build Workflows](https://github.com/usdot-fhwa-stol/v2x-ros-driver/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/usdot-fhwa-stol/v2x-ros-driver/actions/workflows/ci.yml) |  [![Build Workflows](https://github.com/usdot-fhwa-stol/v2x-ros-driver/actions/workflows/dockerhub.yml/badge.svg?branch=develop)](https://github.com/usdot-fhwa-stol/v2x-ros-driver/actions/workflows/dockerhub.yml)


# v2x-ros-driver
v2x-ros-driver is the driver for use with V2X radios and had been tested with the following OBUs: Cohda (MK5/MK6C/MK6), Commsignia, Kapsch. It implements the communication interface allowing UPER-encoded SAE J2735 communication to be exposed to a ROS network using [carma_driver_msgs/msg/ByteArray](https://github.com/usdot-fhwa-stol/carma-msgs/blob/develop/carma_driver_msgs/msg/ByteArray.msg). It has been tested with SAE J2735 2016 but can support later versions as well.

For more information regarding the recommended setup/configuration for an OBU, please refer to this [documentation](/docs/).

# Related Repositories
## v2x-ros-conversion
The [v2x-ros-conversion repository](https://github.com/usdot-fhwa-stol/v2x-ros-conversion) contains a Message node that decodes the carma_driver_msgs/msg/ByteArray passed by v2x-ros-driver into a binary blob and forwards it to the C-V2X driver, such that any OBU/RSU can broadcast the message directly. It also contains a J2735 Converter node that converts the data in the carma_driver_msgs/msg/ByteArray msgs based on J2735 standard units to another message using more usable SI units and vice-versa.

## v2x-ros-emulator
The [v2x-ros-emulator repository](https://github.com/usdot-fhwa-stol/v2x-emulator) enables Raspberry Pi’s to act as mock OBUs and RSUs for the CDA1tenth platform.

# CARMA Platform
The driver was initially developed for CARMA Platform. The primary CARMA Platform repository can be found on [github](https://github.com/usdot-fhwa-stol/carma-platform) and is part of the [USDOT FHWA STOL](https://github.com/usdot-fhwa-stol/)
github organization. Documentation on how the CARMA Platform functions, how it will evolve over time, and how you can contribute can be found at the above links as well

## carma-msgs
The [carma-msgs repository](https://github.com/usdot-fhwa-stol/carma-msgs) contains various message types used by CARMA Platform, including those necessary for v2x-ros-driver (Ex: carma_driver_msgs/msg/ByteArray).

# CDA 1Tenth
[CDA 1Tenth](https://github.com/usdot-fhwa-stol/cda1tenth-bringup/tree/develop) is a low cost research platform for developing and testing cooperative driving automation (CDA) on small scale or simulated vehicles and infrastructure.

## Contribution
Welcome to the CARMA contributing guide. Please read this guide to learn about our development process, how to propose pull requests and improvements, and how to build and test your changes to this project. [CARMA Contributing Guide](https://github.com/usdot-fhwa-stol/carma-platform/blob/develop/Contributing.md)

## Code of Conduct
Please read our [CARMA Code of Conduct](https://github.com/usdot-fhwa-stol/carma-platform/blob/develop/Code_of_Conduct.md) which outlines our expectations for participants within the CARMA community, as well as steps to reporting unacceptable behavior. We are committed to providing a welcoming and inspiring community for all and expect our code of conduct to be honored. Anyone who violates this code of conduct may be banned from the community.

## Attribution
The development team would like to acknowledge the people who have made direct contributions to the design and code in this repository. [CARMA Attribution](https://github.com/usdot-fhwa-stol/carma-platform/blob/develop/ATTRIBUTION.txt)

## License
By contributing to the Federal Highway Administration (FHWA) Connected Automated Research Mobility Applications (CARMA), you agree that your contributions will be licensed under its Apache License 2.0 license. [CARMA License](https://github.com/usdot-fhwa-stol/carma-platform/blob/develop/docs/License.md)

## Contact
Please click on the CARMA logo below to visit the Federal Highway Adminstration(FHWA) CARMA website. For more information, contact CAVSupportServices@dot.gov.

[![CARMA Image](https://raw.githubusercontent.com/usdot-fhwa-stol/carma-platform/develop/docs/image/CARMA_icon.png)](https://highways.dot.gov/research/research-programs/operations/CARMA)
