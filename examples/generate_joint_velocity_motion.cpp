// Copyright (c) 2017 Franka Emika GmbH
// Use of this source code is governed by the Apache-2.0 license, see LICENSE
#include <cmath>
#include <iostream>

#include <franka/exception.h>
#include <franka/robot.h>

#include "examples_common.h"

/**
 * @example generate_joint_velocity_motion.cpp
 * An example showing how to generate a joint velocity motion.
 *
 * @warning Before executing this example, make sure there is enough space in front of the robot.
 */

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <robot-hostname>" << std::endl;
    return -1;
  }
  std::cout << "WARNING: This example will move the robot! "
            << "Please make sure to have the user stop button at hand!" << std::endl
            << "Press Enter to continue..." << std::endl;
  std::cin.ignore();
  try {
    franka::Robot robot(argv[1]);
    // First move the robot to a suitable joint configuration
    std::array<double, 7> q_init = {{0, -M_PI_4, 0, -3 * M_PI_4, 0, M_PI_2, M_PI_4}};
    MotionGenerator motion_generator(0.5, q_init);
    robot.control(motion_generator);
    std::cout << "Finished moving to initial joint configuration." << std::endl;
    // Set additional parameters always before the control loop, NEVER in the control loop!
    // Set collision behavior.
    robot.setCollisionBehavior(
        {{20.0, 20.0, 18.0, 18.0, 16.0, 14.0, 12.0}}, {{20.0, 20.0, 18.0, 18.0, 16.0, 14.0, 12.0}},
        {{20.0, 20.0, 18.0, 18.0, 16.0, 14.0, 12.0}}, {{20.0, 20.0, 18.0, 18.0, 16.0, 14.0, 12.0}},
        {{20.0, 20.0, 20.0, 25.0, 25.0, 25.0}}, {{20.0, 20.0, 20.0, 25.0, 25.0, 25.0}},
        {{20.0, 20.0, 20.0, 25.0, 25.0, 25.0}}, {{20.0, 20.0, 20.0, 25.0, 25.0, 25.0}});

    const std::array<double, 7> max_joint_acc{{14.25, 7.125, 11.875, 11.875, 14.25, 19.0, 19.0}};

    double time_max = 1.0;
    double omega_max = 1.0;
    double time = 0.0;
    robot.control([=, &time](const franka::RobotState& state,
                             franka::Duration time_step) -> franka::JointVelocities {
      time += time_step.toSec();

      double cycle = std::floor(std::pow(-1.0, (time - std::fmod(time, time_max)) / time_max));
      double omega = cycle * omega_max / 2.0 * (1.0 - std::cos(2.0 * M_PI / time_max * time));

      franka::JointVelocities velocities = {{0.0, 0.0, 0.0, omega, omega, omega, omega}};

      if (time >= 2 * time_max) {
        std::cout << std::endl << "Finished motion, shutting down example" << std::endl;
        return franka::MotionFinished(velocities);
      }
      // state.q_d contains the last joint velocity command received by the robot.
      // In case of packet loss due to bad connection or due to a slow control loop
      // not reaching the 1kHz rate, even if your desired velocity trajectory
      // is smooth, discontinuities might occur.
      // Saturating the acceleration computed with respect to the last command received
      // by the robot will prevent from getting discontinuity errors.
      // Note that if the robot does not receive a command it will try to extrapolate
      // the desired behavior assuming a constant acceleration model
      return saturate(max_joint_acc, velocities.dq, state.dq_d);
    });
  } catch (const franka::Exception& e) {
    std::cout << e.what() << std::endl;
    return -1;
  }

  return 0;
}
