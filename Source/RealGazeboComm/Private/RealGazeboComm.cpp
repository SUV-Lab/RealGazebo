#include "RealGazeboComm.h"

#define LOCTEXT_NAMESPACE "FRealGazeboCommModule"

DEFINE_LOG_CATEGORY(LogRealGazeboComm);

void FRealGazeboCommModule::StartupModule()
{
    UE_LOG(LogRealGazeboComm, Log, TEXT("RealGazeboComm module started"));
}

void FRealGazeboCommModule::ShutdownModule()
{
    UE_LOG(LogRealGazeboComm, Log, TEXT("RealGazeboComm module shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRealGazeboCommModule, RealGazeboComm)