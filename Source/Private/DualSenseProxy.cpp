// Copyright (c) 2025 Rafael Valotor/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025


#include "DualSenseProxy.h"
#include "DualSenseLibrary.h"
#include "FValidationUtils.h"
#include "Runtime/ApplicationCore/Public/GenericPlatform/IInputInterface.h"
#include "Runtime/ApplicationCore/Public/GenericPlatform/GenericApplicationMessageHandler.h"


bool UDualSenseProxy::DeviceIsConnected(int32 ControllerId)
{
	return UDualSenseLibrary::IsConnected(ControllerId);
}

bool UDualSenseProxy::DeviceReconnect(int32 ControllerId)
{
	if (UDualSenseLibrary::IsConnected(ControllerId))
	{
		return true;
	}

	return UDualSenseLibrary::Reconnect(ControllerId);
}

float UDualSenseProxy::LevelBatteryDevice(int32 ControllerId)
{
	if (!UDualSenseLibrary::IsConnected(ControllerId))
	{
		return 0;
	}

	return UDualSenseLibrary::GetLevelBattery(ControllerId);
}

int32 UDualSenseProxy::GetTriggerLeftForceFeedback(int32 ControllerId)
{
	if (!UDualSenseLibrary::IsConnected(ControllerId))
	{
		return 0;
	}

	EControllerHand HandToUse = EControllerHand::Left;
	return UDualSenseLibrary::GetTrirggersFeedback(ControllerId, HandToUse);
}

int32 UDualSenseProxy::GetTriggerRightForceFeedback(int32 ControllerId)
{
	if (!UDualSenseLibrary::IsConnected(ControllerId))
	{
		return 0;
	}

	EControllerHand HandToUse = EControllerHand::Right;
	return UDualSenseLibrary::GetTrirggersFeedback(ControllerId, HandToUse);
}

void UDualSenseProxy::SetTriggerHapticFeedbackEffect(int32 ControllerId, int32 StartPosition, int32 BeginForce,
                                                     int32 MiddleForce, int32 EndForce, EControllerHand Hand,
                                                     bool KeepEffect)
{
	if (!UDualSenseLibrary::IsConnected(ControllerId))
	{
		return;
	}

	if (
		!FValidationUtils::ValidateMaxForce(BeginForce) &&
		!FValidationUtils::ValidateMaxForce(MiddleForce) &&
		!FValidationUtils::ValidateMaxForce(EndForce) &&
		!FValidationUtils::ValidateMaxPosition(StartPosition)
	)
	{
		return;
	}


	UDualSenseLibrary::ConfigTriggerHapticFeedbackEffect(ControllerId, StartPosition, BeginForce, MiddleForce, EndForce,
	                                                     Hand, KeepEffect);
}

void UDualSenseProxy::EffectNoResitance(int32 ControllerId, EControllerHand HandResistence)
{
	if (!UDualSenseLibrary::IsConnected(ControllerId))
	{
		return;
	}

	UDualSenseLibrary::NoResitance(ControllerId, HandResistence);
}

void UDualSenseProxy::EffectContinuousResitance(int32 ControllerId, int32 StartPosition, int32 Force,
                                                EControllerHand HandContinous)
{
	if (!UDualSenseLibrary::IsConnected(ControllerId))
	{
		return;
	}

	if (
		!FValidationUtils::ValidateMaxForce(Force) &&
		!FValidationUtils::ValidateMaxPosition(StartPosition)
	)
	{
		return;
	}

	UDualSenseLibrary::ContinuousResitance(ControllerId, StartPosition, Force, HandContinous);
}

void UDualSenseProxy::EffectSectionResitance(int32 ControllerId, int32 StartPosition, int32 EndPosition,
                                             EControllerHand Hand)
{
	if (!UDualSenseLibrary::IsConnected(ControllerId))
	{
		return;
	}

	if (
		!FValidationUtils::ValidateMaxPosition(StartPosition) &&
		!FValidationUtils::ValidateMaxPosition(EndPosition)
	)
	{
		return;
	}

	UDualSenseLibrary::SectionResitance(ControllerId, StartPosition, EndPosition, Hand);
}

void UDualSenseProxy::StopTriggerEffect(const int32 ControllerId, EControllerHand HandStop)
{
	if (!UDualSenseLibrary::IsConnected(ControllerId))
	{
		return;
	}

	UDualSenseLibrary::StopEffect(ControllerId, HandStop);
}

void UDualSenseProxy::StopAllTriggersEffects(const int32 ControllerId)
{
	if (!UDualSenseLibrary::IsConnected(ControllerId))
	{
		return;
	}

	UDualSenseLibrary::StopAllEffects(ControllerId);
}

void UDualSenseProxy::ResetEffects(const int32 ControllerId)
{
	if (!UDualSenseLibrary::IsConnected(ControllerId))
	{
		return;
	}

	UDualSenseLibrary::StopAll(ControllerId);
}


void UDualSenseProxy::LedPlayerEffects(int32 ControllerId, ELedPlayerEnum Value, ELedBrightnessEnum Brightness)
{
	if (!UDualSenseLibrary::IsConnected(ControllerId))
	{
		return;
	}

	int32 BrightnessValue = static_cast<int32>(Brightness);
	int32 LedValue = static_cast<int32>(Value);
	UDualSenseLibrary::SetLedPlayerEffects(ControllerId, LedValue, BrightnessValue);
}

void UDualSenseProxy::LedMicEffects(int32 ControllerId, ELedMicEnum Value)
{
	if (!UDualSenseLibrary::IsConnected(ControllerId))
	{
		return;
	}

	int32 LedNumber = 0;
	if (Value == ELedMicEnum::MicOn)
	{
		LedNumber = 1;
	}

	if (Value == ELedMicEnum::Pulse)
	{
		LedNumber = 2;
	}

	UDualSenseLibrary::SetLedMicEffects(ControllerId, LedNumber);
}

void UDualSenseProxy::LedColorEffects(int32 ControllerId, FColor Color)
{
	if (!UDualSenseLibrary::IsConnected(ControllerId))
	{
		return;
	}

	UDualSenseLibrary::UpdateColorOutput(ControllerId, Color);
}

void UDualSenseProxy::EnableAccelerometerValues(int32 ControllerId, bool bEnableAccelerometer)
{
	UDualSenseLibrary::SetAcceleration(ControllerId, bEnableAccelerometer);
}

void UDualSenseProxy::EnableGyroscopeValues(int32 ControllerId, bool bEnableGyroscope)
{
	UDualSenseLibrary::SetGyroscope(ControllerId, bEnableGyroscope);
}

void UDualSenseProxy::EnableTouch1(int32 ControllerId, bool bEnableTouch1)
{
	UDualSenseLibrary::SetTouch1(ControllerId, bEnableTouch1);
}

void UDualSenseProxy::EnableTouch2(int32 ControllerId, bool bEnableTouch1)
{
	UDualSenseLibrary::SetTouch2(ControllerId, bEnableTouch1);
}
