// reachability_from_above.cpp
// wsn, March 2015
// compute reachability, w/ z_tool_des = [1;0;0] = x_base
// w/ robot mounted as is, x_base points down
// Fix the value of x, --> const height; scan over y and z
//const double x_des = 0.374;

#include <ros/ros.h>
#include <irb120_kinematics.h>
#include <geometry_msgs/Point.h>
#include <std_msgs/Int16.h>
#include <fstream>
#include <tf/transform_listener.h>
//#include <std_msgs/Int16MultiArray>

// define some global scope variable
Eigen::Vector3d g_p;
Eigen::Quaterniond g_quat;
// Eigen::Matrix3d g_R;
// Eigen::Affine3d g_A_flange_desired;

// define some variable which is used to take coordinates transformations
tf::TransformListener* g_tfListener;
tf::StampedTransform g_armlink1_wrt_baseLink;
geometry_msgs::PoseStamped g_desToolflange_pose_in;
geometry_msgs::PoseStamped g_desToolflange_pose_wrt_base_link;

Eigen::Vector3d tfLink1toBaselink (geometry_msgs::Point P_des, Eigen::Matrix3d R_des) {
    Eigen::Quaterniond q(R_des); // transform rotation matrix to quaternion format
    g_desToolflange_pose_in.header.seq = 1;
    g_desToolflange_pose_in.header.stamp = ros::Time::now();
    g_desToolflange_pose_in.header.frame_id = "link1";
    g_desToolflange_pose_in.pose.position.x = P_des.x;
    g_desToolflange_pose_in.pose.position.y = P_des.y;
    g_desToolflange_pose_in.pose.position.z = P_des.z;
    g_desToolflange_pose_in.pose.orientation.x = q.x();
    g_desToolflange_pose_in.pose.orientation.y = q.y();
    g_desToolflange_pose_in.pose.orientation.z = q.z();
    g_desToolflange_pose_in.pose.orientation.w = q.w();
    // taking transformation: convert g_desToolflange_pose_in expressed in "link1" frame into 
    // g_desToolflange_pose_wrt_base_link expressed in "base_link" frame.
    g_tfListener->transformPose("base_link", g_desToolflange_pose_in, g_desToolflange_pose_wrt_base_link);

    g_p[0] = g_desToolflange_pose_wrt_base_link.pose.position.x;
    g_p[1] = g_desToolflange_pose_wrt_base_link.pose.position.y;
    g_p[2] = g_desToolflange_pose_wrt_base_link.pose.position.z;
    g_quat.x() = g_desToolflange_pose_wrt_base_link.pose.orientation.x;
    g_quat.y() = g_desToolflange_pose_wrt_base_link.pose.orientation.y;
    g_quat.z() = g_desToolflange_pose_wrt_base_link.pose.orientation.z;
    g_quat.w() = g_desToolflange_pose_wrt_base_link.pose.orientation.w;   
    // g_R = g_quat.matrix();  
    // g_A_flange_desired.translation() = g_p;
    // g_A_flange_desired.linear() = g_R;

    return g_p;
}

int main(int argc, char **argv) {
    ros::init(argc, argv, "irb120_reachability");
    
    g_tfListener = new tf::TransformListener;  //create a transform listener
    
    // wait to start receiving valid tf transforms between map and odom:
    bool tferr=true;
    ROS_INFO("waiting for tf between base_link and link1 of arm...");
    while (tferr) {
        tferr=false;
        try {
            //try to lookup transform from target frame "odom" to source frame "map"
            //The direction of the transform returned will be from the target_frame to the source_frame. 
            //Which if applied to data, will transform data in the source_frame into the target_frame. See tf/CoordinateFrameConventions#Transform_Direction
            //void tf::TransformListener::lookupTransform (const std::string &target_frame, const std::string &source_frame, const ros::Time &time, StampedTransform &transform) const 
                g_tfListener->lookupTransform("base_link", "link1", ros::Time(0), g_armlink1_wrt_baseLink);
            } catch(tf::TransformException &exception) {
                ROS_ERROR("%s", exception.what());
                tferr=true;
                ros::Duration(0.5).sleep(); // sleep for half a second
                ros::spinOnce();                
            }   
    }
    ROS_INFO("tf is good");
    // from now on, tfListener will keep track of transforms

    // create a variable of type "Point" to publish the desired point for manipulator's tool flange to move
    geometry_msgs::Point desPt;
    desPt.x = 0.0;
    desPt.y = 0.0;
    desPt.z = 0.0;

    Eigen::Vector3d p;
    Eigen::Vector3d n_des,t_des,b_des;
    Vectorq6x1 q_in;
    q_in<<0,0,0,0,0,0;

    Irb120_fwd_solver irb120_fwd_solver;
    Irb120_IK_solver ik_solver;

    // we should keep the tool flange as this orientation
    b_des<<1,0,0;
    t_des<<0,1,0;
    n_des = t_des.cross(b_des);
    
    Eigen::Matrix3d R_des;
    R_des.col(0) = n_des;
    R_des.col(1) = t_des;
    R_des.col(2) = b_des;
    
    std::vector<Vectorq6x1> q6dof_solns;
    Eigen::Vector3d reachablePtWrtBaseLink;
    Eigen::Affine3d a_tool_des; // expressed in DH frame
    
    a_tool_des.linear() = R_des; // never change
    std::cout << "====  irb120 kinematics solver ====" << std::endl;

    // output a File 
    std::ofstream outputFile;
    // std::ios::trunc If the file is opened for output operations and it already existed, its previous content is deleted and replaced by the new one.
    outputFile.open("mktPtPos.txt");
    for (double x_des = -0.4; x_des < 0.7; x_des += 0.1) {
        std::cout << std::endl;
        std::cout << "x=" << x_des <<"  ";
        for (double z_des = 0.9; z_des >- 0.4; z_des -= 0.1) {
            std::cout << std::endl;
            std::cout << "z=" << round(z_des*10) << "  ";
           for (double y_des =- 1.0; y_des < 1.0; y_des += 0.05) {
                p[0] = x_des;
                p[1]= y_des;
                p[2] = z_des;
                /*desPt.y = y_des;
                desPt.z = z_des;*/
                a_tool_des.translation() = p;
                int nsolns = ik_solver.ik_solve(a_tool_des);
                //std_msgs::Int16 iknsolns; // create a variable of type "Int16" to publish the number of IK solution
                //iknsolns.data = nsolns;
                std::cout<<nsolns;

                // Only when nsolns > 0, it is meaningful to publish the corresponding desired tool point as a topic: reachablePt
                if (nsolns>0) {
                    desPt.x = x_des; // remember the desired value of x coordiate and and assign to desPt.x
                    desPt.y = y_des; // remember the desired value of y coordiate and and assign to desPt.y
                    desPt.z = z_des; // remember the desired value of z coordiate and and assign to desPt.z
                    //ROS_INFO("desired point: x = %f, y = %f, z = %f", desPt.x, desPt.y, desPt.z);
                    ik_solver.get_solns(q6dof_solns);
                    // defining a joint limits vector for joint 0 and joint 1, such that each joint is specified within a range of motion
                    std::vector<double> jointLimits {0,-M_PI,-M_PI/2,M_PI/6}; 
                    std::vector<int> weight{1,2,3,3,2,1}; //defining a weight vector
                    double sum;
                    double minimum = 1e6;
                    std_msgs::Int16 bestIkSoluNo;
                    Vectorq6x1 oneIkSolu;
                    // Once nsolns > 0, choose a IK solution both meet the specified joint limit the weight requirement
                    for (int i = 0; i < q6dof_solns.size(); ++i){
                        oneIkSolu = q6dof_solns[i];
                        if (oneIkSolu[0] < jointLimits[0] && oneIkSolu[0] > jointLimits[1] 
                            && oneIkSolu[1] < jointLimits[3] && oneIkSolu[1] > jointLimits[2]) {
                            sum = 0;
                            for (int ijnt = 0; ijnt < 6; ++ijnt){
                                sum = sum + oneIkSolu[ijnt] * weight[ijnt];
                            }
                            if (sum < minimum){
                                minimum = sum;
                                bestIkSoluNo.data = i; // remember the IK solution which has the minimum last joint angle solution
                            }
                        }   
                    }
                    reachablePtWrtBaseLink = tfLink1toBaselink(desPt, R_des);
                    outputFile << reachablePtWrtBaseLink << std::endl;
                }
            }
        }
    }
    // for (int i = 0; i < reachPtVec.size(); ++i) {
    //     ROS_INFO("Reachable Point Coordinates: X = %f, Y= %f, Z = %f", reachPtVec[&i].x ,reachPtVec[&i].y, reachPtVec[&i].z);
    // }
    outputFile.close();
    // timer.sleep();
    // }
    return 0;
}
