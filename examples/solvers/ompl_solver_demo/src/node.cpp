/*
 *  Created on: 30 Apr 2014
 *      Author: Vladimir Ivan
 * 
 * Copyright (c) 2016, University Of Edinburgh 
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 * 
 *  * Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer. 
 *  * Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *  * Neither the name of  nor the names of its contributors may be used to 
 *    endorse or promote products derived from this software without specific 
 *    prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#include "ompl_solver_demo/node.h"

using namespace exotica;

OMPLSolverDemoNode::OMPLSolverDemoNode()
    : nh_("~"), nhg_()
{

  {
    // Declarations
    MotionSolver_ptr sol;
    Server_ptr ser;
    PlanningProblem_ptr prob;

    // Get config file path, problem name and solver name from launch file
    std::string config_name;
    std::string problem_name, solver_name;
    nh_.getParam("config", config_name);
    nh_.getParam("problem", problem_name);
    nh_.getParam("solver", solver_name);
    ROS_INFO_STREAM("Config: "<<config_name<<"\nSolver: "<<solver_name[0]<<"\nProblem: "<<problem_name[0]);

      // Initialise and solve
      Initialiser::Instance()->initialise(config_name, ser, sol, prob, problem_name, solver_name);
      // Assign the problem to the solver
      sol->specifyProblem(prob);
      // Create the initial configuration
      Eigen::VectorXd q = Eigen::VectorXd::Zero(
          prob->scene_->getNumJoints());
      Eigen::MatrixXd solution;

      Eigen::VectorXd goal = *boost::static_pointer_cast<TaskTerminationCriterion>(prob->getTaskDefinitions().begin()->second)->getGoal();
      sol->setGoalState(goal);

      HIGHLIGHT("Problem description:\n"<<sol->print(""));

      ROS_INFO_STREAM("Calling solve()");

      ros::WallTime start_time = ros::WallTime::now();
      // Solve the problem using the IK solver
      try
      {
        sol->Solve(q, solution);
        double time =
            ros::Duration((ros::WallTime::now() - start_time).toSec()).toSec();
        ROS_INFO_STREAM_THROTTLE(0.5, "Finished solving ("<<time<<"s)");
        ROS_INFO_STREAM_THROTTLE(0.5,
            "Solution "<<solution.row(solution.rows()-1));

        // Publish the states to rviz
        jointStatePublisher_ = nhg_.advertise<sensor_msgs::JointState>(
            "/joint_states", 1);
        sensor_msgs::JointState jnt;
        jnt.position.resize(solution.cols());
        jnt.name =
            prob->scene_->getSolver().getJointNames();
        ros::Rate loop_rate(50.0); // Magic number for now
        int t = 0;
        ROS_INFO_STREAM_THROTTLE(0.5, "Publishing states to rviz ...");
        while (ros::ok())
        {
          jnt.header.stamp = ros::Time::now();
          jnt.header.seq++;
          for (int j = 0; j < solution.cols(); j++)
            jnt.position[j] = solution(t, j);
          jointStatePublisher_.publish(jnt);

          t = t + 1 >= solution.rows() ? 0 : t + 1;
          ros::spinOnce();
          loop_rate.sleep();
        }
      }
      catch (SolveException e)
      {
        double time = ros::Duration(
            (ros::WallTime::now() - start_time).toSec()).toSec();
        ROS_INFO_STREAM_THROTTLE(0.5,
            e.what()<<" ("<<time<<"s)");
      }


  }
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "OMPLSolverDemoNode");
  ROS_INFO_STREAM("Started");
  OMPLSolverDemoNode ex;
  ros::spin();
}
