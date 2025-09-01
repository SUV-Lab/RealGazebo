// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"

// Declare custom logging categories
DECLARE_LOG_CATEGORY_EXTERN(LogRealGazeboCore, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogRealGazeboVehicle, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogRealGazeboNetwork, Log, All);

/**
 * Centralized logging system for RealGazebo plugin
 */
class REALGAZEBO_API GazeboLogger
{
public:
    // Core module logging with runtime verbosity
    template<typename... Args>
    static void LogCore(ELogVerbosity::Type Verbosity, const TCHAR* Format, Args... args)
    {
        if (LogRealGazeboCore.IsSuppressed(Verbosity))
        {
            return;
        }
        
        switch (Verbosity)
        {
        case ELogVerbosity::Error:
            UE_LOG(LogRealGazeboCore, Error, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::Warning:
            UE_LOG(LogRealGazeboCore, Warning, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::Display:
            UE_LOG(LogRealGazeboCore, Display, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::Log:
            UE_LOG(LogRealGazeboCore, Log, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::Verbose:
            UE_LOG(LogRealGazeboCore, Verbose, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::VeryVerbose:
            UE_LOG(LogRealGazeboCore, VeryVerbose, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        default:
            UE_LOG(LogRealGazeboCore, Log, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        }
    }
    
    // Vehicle system logging with runtime verbosity
    template<typename... Args>
    static void LogVehicle(ELogVerbosity::Type Verbosity, const TCHAR* Format, Args... args)
    {
        if (LogRealGazeboVehicle.IsSuppressed(Verbosity))
        {
            return;
        }
        
        switch (Verbosity)
        {
        case ELogVerbosity::Error:
            UE_LOG(LogRealGazeboVehicle, Error, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::Warning:
            UE_LOG(LogRealGazeboVehicle, Warning, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::Display:
            UE_LOG(LogRealGazeboVehicle, Display, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::Log:
            UE_LOG(LogRealGazeboVehicle, Log, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::Verbose:
            UE_LOG(LogRealGazeboVehicle, Verbose, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::VeryVerbose:
            UE_LOG(LogRealGazeboVehicle, VeryVerbose, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        default:
            UE_LOG(LogRealGazeboVehicle, Log, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        }
    }
    
    // Network system logging with runtime verbosity
    template<typename... Args>
    static void LogNetwork(ELogVerbosity::Type Verbosity, const TCHAR* Format, Args... args)
    {
        if (LogRealGazeboNetwork.IsSuppressed(Verbosity))
        {
            return;
        }
        
        switch (Verbosity)
        {
        case ELogVerbosity::Error:
            UE_LOG(LogRealGazeboNetwork, Error, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::Warning:
            UE_LOG(LogRealGazeboNetwork, Warning, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::Display:
            UE_LOG(LogRealGazeboNetwork, Display, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::Log:
            UE_LOG(LogRealGazeboNetwork, Log, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::Verbose:
            UE_LOG(LogRealGazeboNetwork, Verbose, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        case ELogVerbosity::VeryVerbose:
            UE_LOG(LogRealGazeboNetwork, VeryVerbose, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        default:
            UE_LOG(LogRealGazeboNetwork, Log, TEXT("%s"), *FString::Printf(Format, args...));
            break;
        }
    }
    
    // Convenience functions for different log levels
    template<typename... Args>
    static void LogCoreInfo(const TCHAR* Format, Args... args)
    {
        UE_LOG(LogRealGazeboCore, Display, TEXT("%s"), *FString::Printf(Format, args...));
    }
    
    template<typename... Args>
    static void LogCoreWarning(const TCHAR* Format, Args... args)
    {
        UE_LOG(LogRealGazeboCore, Warning, TEXT("%s"), *FString::Printf(Format, args...));
    }
    
    template<typename... Args>
    static void LogCoreError(const TCHAR* Format, Args... args)
    {
        UE_LOG(LogRealGazeboCore, Error, TEXT("%s"), *FString::Printf(Format, args...));
    }
    
    template<typename... Args>
    static void LogVehicleInfo(const TCHAR* Format, Args... args)
    {
        UE_LOG(LogRealGazeboVehicle, Display, TEXT("%s"), *FString::Printf(Format, args...));
    }
    
    template<typename... Args>
    static void LogVehicleWarning(const TCHAR* Format, Args... args)
    {
        UE_LOG(LogRealGazeboVehicle, Warning, TEXT("%s"), *FString::Printf(Format, args...));
    }
    
    template<typename... Args>
    static void LogVehicleError(const TCHAR* Format, Args... args)
    {
        UE_LOG(LogRealGazeboVehicle, Error, TEXT("%s"), *FString::Printf(Format, args...));
    }
    
    template<typename... Args>
    static void LogNetworkInfo(const TCHAR* Format, Args... args)
    {
        UE_LOG(LogRealGazeboNetwork, Display, TEXT("%s"), *FString::Printf(Format, args...));
    }
    
    template<typename... Args>
    static void LogNetworkWarning(const TCHAR* Format, Args... args)
    {
        UE_LOG(LogRealGazeboNetwork, Warning, TEXT("%s"), *FString::Printf(Format, args...));
    }
    
    template<typename... Args>
    static void LogNetworkError(const TCHAR* Format, Args... args)
    {
        UE_LOG(LogRealGazeboNetwork, Error, TEXT("%s"), *FString::Printf(Format, args...));
    }
    
private:
    GazeboLogger() = delete; // Static class, prevent instantiation
};