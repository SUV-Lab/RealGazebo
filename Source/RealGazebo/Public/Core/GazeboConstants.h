// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * Central location for all RealGazebo constants and configuration values
 */
class REALGAZEBO_API GazeboConstants
{
public:
    // Network Configuration
    static constexpr int32 DEFAULT_UDP_PORT = 8888;
    static constexpr int32 UDP_BUFFER_SIZE = 1024;
    static constexpr float UDP_RECEIVE_TIMEOUT = 0.1f;
    
    // Message IDs
    static constexpr uint8 MESSAGE_ID_POSE = 1;
    static constexpr uint8 MESSAGE_ID_MOTOR_SPEED = 2;
    static constexpr uint8 MESSAGE_ID_SERVO = 3;
    
    // Vehicle Configuration
    static constexpr int32 MAX_VEHICLES = 100;
    static constexpr int32 MAX_MOTORS_PER_VEHICLE = 8;
    static constexpr int32 MAX_SERVOS_PER_VEHICLE = 16;
    
    // Packet Sizes
    static constexpr int32 HEADER_SIZE = 3; // VehicleNum + VehicleType + MessageID
    static constexpr int32 POSE_DATA_SIZE = 24; // Position(12) + Rotation(12)
    static constexpr int32 MOTOR_SPEED_SIZE = 4; // Float per motor
    static constexpr int32 SERVO_DATA_SIZE = 28; // Position(12) + Rotation(16)
    
    // Scaling and Conversion
    static constexpr float GAZEBO_TO_UE_SCALE = 100.0f; // Convert meters to cm
    static constexpr float DEGREES_TO_RADIANS = PI / 180.0f;
    static constexpr float RADIANS_TO_DEGREES = 180.0f / PI;
    
    // Default IP Addresses
    static const FString DEFAULT_LISTEN_IP;
    static const FString LOCALHOST_IP;
    
    // Logging Categories
    static const FString LOG_CATEGORY_VEHICLE;
    static const FString LOG_CATEGORY_NETWORK;
    static const FString LOG_CATEGORY_CORE;
    
private:
    GazeboConstants() = delete; // Static class, prevent instantiation
};