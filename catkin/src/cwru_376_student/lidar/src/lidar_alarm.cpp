#include <ros/ros.h>
#include <std_msgs/Float32.h>
#include <std_msgs/Bool.h>

class LidarAlarm {
private:
	static LidarAlarm *instance;
    // subscribe to the lidar_nearest topic
    ros::Subscriber lidar_nearest_subscriber;
	// publish a Boolean on lidar_alarm
    ros::Publisher lidar_alarm;
	// track the current state of the alarm
	bool isAlarmed;
	// the closest something can be before the alarm is raised (meters)
	double minimum_safe_distance;
public:
	LidarAlarm(ros::NodeHandle& nh);
	// callback for when the lidar_nearest has data
	static void lidarNearestCallback(const std_msgs::Float32& nearest);
};

LidarAlarm::LidarAlarm(ros::NodeHandle& nh) {
	// set the singleton instance variable
	instance = this;
	// subscribe to lidar_nearest
	lidar_nearest_subscriber = nh.subscribe("lidar_nearest", 1, lidarNearestCallback);
	// broadcast on the lidar_alarm topic
	lidar_alarm = nh.advertise<std_msgs::Bool>("lidar_alarm", 1);
	// by default, alarm is off
	isAlarmed = false;
	// get the minimum_safe_distance from the Param Server. If not, set to 0.5 m.
	if (!nh.getParam("/lidar_alarm/lidar_safe_distance", minimum_safe_distance)){
		ROS_WARN("/lidar_alarm/lidar_safe_distance param should be specified");
		minimum_safe_distance = 0.5; // meters
	}
};

LidarAlarm *LidarAlarm::instance;

void LidarAlarm::lidarNearestCallback(const std_msgs::Float32& nearest) {
	instance->isAlarmed = (nearest.data < instance->minimum_safe_distance);
	// publish the alarm
	std_msgs::Bool lidar_alarm_msg;
	lidar_alarm_msg.data = instance->isAlarmed;
	instance->lidar_alarm.publish(lidar_alarm_msg);
}

int main(int argc, char **argv) {
    ros::init(argc, argv, "lidar_alarm"); // name this node
    ros::NodeHandle nh;
	
	LidarAlarm lidar_alarm(nh);
	
	// do nothing; delegate further processing to callbacks
    ros::spin();
	
    return 0;
}
