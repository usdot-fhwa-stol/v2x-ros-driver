# Copyright (C) 2022 LEIDOS.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

from ament_index_python import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch.actions import Shutdown
import launch.actions
import launch.events
import launch_ros.events.lifecycle
import lifecycle_msgs.msg
from carma_ros2_utils.launch.get_current_namespace import GetCurrentNamespace

import os


'''
This file is can be used to launch the CARMA v2x_ros_driver_node.
  Though in carma-platform it may be launched directly from the base launch file.
'''

def generate_launch_description():

    # Declare the log_level launch argument
    log_level = LaunchConfiguration('log_level')
    declare_log_level_arg = DeclareLaunchArgument(
        name ='log_level', default_value='WARN')

    configuration_delay = LaunchConfiguration('configuration_delay')
    declare_configuration_delay_arg = DeclareLaunchArgument(
        name ='configuration_delay', default_value='4.0')

    # Get parameter file path
    param_file_path = os.path.join(
        get_package_share_directory('v2x_ros_driver'), 'config/params.yaml')


    # Launch node(s) in a carma container to allow logging to be configured
    container = ComposableNodeContainer(
        package='carma_ros2_utils',
        name='v2x_ros_driver_container',
        namespace=GetCurrentNamespace(),
        executable='carma_component_container_mt',
        composable_node_descriptions=[

            # Launch the core node(s)
            ComposableNode(
                    package='v2x_ros_driver',
                    plugin='V2XDriverApplication::Node',
                    name='v2x_ros_driver_node',
                    extra_arguments=[
                        {'use_intra_process_comms': True},
                        {'--log-level' : log_level }
                    ],
                    remappings=[
                        ("inbound_binary_msg", "comms/inbound_binary_msg"),
                        ("outbound_binary_msg", "comms/outbound_binary_msg"),
                    ],
                    parameters=[ param_file_path ]
            ),
        ],
        on_exit= Shutdown()
    )

    ros2_cmd = launch.substitutions.FindExecutable(name='ros2')

    process_configure = launch.actions.ExecuteProcess(
        cmd=[ros2_cmd, "lifecycle", "set", "/v2x_ros_driver", "configure"],
    )

    configuration_trigger = launch.actions.TimerAction(
        period=configuration_delay,
        actions=[
            process_configure,
        ]
    )

    configured_event_handler = launch.actions.RegisterEventHandler(launch.event_handlers.OnExecutionComplete(
        target_action=process_configure, on_completion=[
            launch.actions.ExecuteProcess(
                cmd=[ros2_cmd, "lifecycle", "set", "/v2x_ros_driver", "activate"],
            )
        ])
    )


    return LaunchDescription([
        declare_log_level_arg,
        declare_configuration_delay_arg,
        container,
        configuration_trigger,
        configured_event_handler
    ])
