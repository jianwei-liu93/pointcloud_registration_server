
#include "pointcloud_registration_server/reg_creation.h"

namespace RegCreation
{
	bool registrationFromYAML(pointcloud_registration_server::registration_service *srv, std::string yaml_file_name)
	{
//		if( ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug) )
//    ros::console::notifyLoggerLevelsChanged();  

		ros::NodeHandle nh;
		ROS_DEBUG_STREAM("[RegCreation] Beginning to attempt to initialize parameters from server for registration " + yaml_file_name + ".");

		// ------------ Basic Stuff Required for All Registration Processes ------------
		std::string name;
		bool temp_bool;
		if( !nh.getParam(yaml_file_name + "/registration_name", name) )
		{
			ROS_ERROR_STREAM("[RegCreation] Failed to get registration process name from yaml file! Exiting.");
			return false;
		}
		if( !nh.getParam(yaml_file_name + "/" + name + "/registration_type", srv->request.registration_type) )
		{
			ROS_ERROR_STREAM("[RegCreation] Failed to get registration type." );
			return false;
		} 
		if( !nh.param<bool>(yaml_file_name + "/should_publish", temp_bool, true) )
			ROS_WARN_STREAM("[RegCreation] Failed to get should_publish - defaulting to true.");
		srv->request.should_publish = temp_bool;
		if( !nh.param<float>(yaml_file_name + "/" + name + "/epsilon", srv->request.epsilon, .001) )
			ROS_WARN_STREAM("[RegCreation] Failed to get epsilon - defaulting to 0.001");
		if( !nh.param<int>(yaml_file_name + "/" + name +  "/max_iterations", srv->request.max_iterations, 30) )
			ROS_WARN_STREAM("[RegCreation] Failed to get max_iterations - defaulting to 30.");

		// ------------------------ Stuff for Preprocessing ------------------------
		if( !nh.param<bool>(yaml_file_name + "/preprocessing/voxelize_target", temp_bool, true) )
			ROS_WARN_STREAM("[RegCreation] Failed to get voxelize_target - defaulting to true.");
		srv->request.voxelize_target = temp_bool;
		if( !nh.param<bool>(yaml_file_name + "/preprocessing/voxelize_source", temp_bool, true) )
			ROS_WARN_STREAM("[RegCreation] Failed to get voxelize_source - defaulting to true.");
		srv->request.voxelize_source = temp_bool;
		if( !nh.param<float>(yaml_file_name + "/preprocessing/voxel_leaf_size", srv->request.voxel_leaf_size, 0.01) )
			ROS_WARN_STREAM("[RegCreation] Failed to get voxel_leaf_size - defaulting to 0.01.");
		if( !nh.getParam(yaml_file_name + "/preprocessing/clipping_dimensions", srv->request.clipping_dimensions) )
		{
			ROS_WARN_STREAM("[RegCreation] Failed to get clipping_dimensions - defaulting to -1 to 1 cube.");
			for(int i=0; i<6; i++)
				srv->request.clipping_dimensions.push_back(pow((-1),i%2+1));
		}
		if( !nh.param<bool>(yaml_file_name + "/preprocessing/center_clipping_base", temp_bool, true) )
			ROS_WARN_STREAM("[RegCreation] Failed to get center_clipping_base - defaulting to true.");
		srv->request.center_clipping_base = temp_bool;
		if( !nh.param<bool>(yaml_file_name + "/preprocessing/stitch_unvoxelized", temp_bool, true) )
			ROS_WARN_STREAM("[RegCreation] Failed to get stitch_unvoxelized - defaulting to true.");
		srv->request.stitch_unvoxelized = temp_bool;
		if( !nh.param<bool>(yaml_file_name + "/preprocessing/stitch_unclipped", temp_bool, true) )
			ROS_WARN_STREAM("[RegCreation] Failed to get stitch_unclipped - defaulting to true.");
		srv->request.stitch_unclipped = temp_bool;


		float temp_float;
		std::vector<float> temp_float_vector;
		switch(srv->request.registration_type)
		{
			case 1: 		// ICP
				if( !nh.param<float>(yaml_file_name + "/" + name + "/ksearch", temp_float, 30) )
					ROS_WARN_STREAM("[RegCreation] Failed to get ksearch - defaulting to true.");
				srv->request.parameters.push_back(temp_float);
				if( !nh.param<float>(yaml_file_name + "/" + name + "/max_dist", temp_float, 0.1) )
					ROS_WARN_STREAM("[RegCreation] Failed to get max_dist - defaulting to true.");
				srv->request.parameters.push_back(temp_float);
				if( !nh.getParam(yaml_file_name + "/" + name + "/alpha", temp_float_vector) )
					ROS_WARN_STREAM("[RegCreation] Failed to get alpha - defaulting to true.");
				for(int i=0; i<4; i++)
					srv->request.parameters.push_back(temp_float_vector[i]);
				break;
			case 2: 		// NDT
				if( !nh.param<float>(yaml_file_name + "/" + name + "/step_size", temp_float, 0.1) )
					ROS_WARN_STREAM("[RegCreation] Failed to get step_size - defaulting to true.");
				srv->request.parameters.push_back(temp_float);
				if( !nh.param<float>(yaml_file_name + "/" + name + "/resolution", temp_float, 1.0) )
					ROS_WARN_STREAM("[RegCreation] Failed to get resolution - defaulting to true.");
				srv->request.parameters.push_back(temp_float);
				break;
		}

		if(srv->request.center_clipping_base)
		{
			pointcloud_processing_server::pointcloud_task transform_task = PointcloudTaskCreation::transformTask("transform", "base_link", false, "foo");
			srv->request.source_tasks.push_back(transform_task);
			srv->request.target_tasks.push_back(transform_task);
		}
		pointcloud_processing_server::pointcloud_task clipping_task = PointcloudTaskCreation::clippingConditionalTask("clipping", srv->request.clipping_dimensions, false, false, "foo");
		srv->request.source_tasks.push_back(clipping_task);
		srv->request.target_tasks.push_back(clipping_task);
		if(srv->request.voxelize_source)
		{
			pointcloud_processing_server::pointcloud_task voxelization_task = PointcloudTaskCreation::voxelizationTask("voxelization", srv->request.voxel_leaf_size, srv->request.voxel_leaf_size, srv->request.voxel_leaf_size, false, false, "foo");
			srv->request.source_tasks.push_back(voxelization_task);
		}
		if(srv->request.voxelize_target)
		{
			pointcloud_processing_server::pointcloud_task voxelization_task = PointcloudTaskCreation::voxelizationTask("voxelization", srv->request.voxel_leaf_size, srv->request.voxel_leaf_size, srv->request.voxel_leaf_size, false, false, "foo");
			srv->request.target_tasks.push_back(voxelization_task);
		}
		if(srv->request.center_clipping_base)
		{
			pointcloud_processing_server::pointcloud_task transform_task = PointcloudTaskCreation::transformTask("transform", "map", false, "foo");
			srv->request.source_tasks.push_back(transform_task);
			srv->request.target_tasks.push_back(transform_task);
		}

		return true;
	}
};