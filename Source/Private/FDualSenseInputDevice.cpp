// Copyright (c) 2025 Rafael Valotor/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025


#include "FDualSenseInputDevice.h"
#include "DualSenseLibrary.h"
#include "Windows/WindowsApplication.h"


FDualSenseInputDevice::FDualSenseInputDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler):
	FGenericPlatformInputDeviceMapper(true, false),
	MessageHandler(InMessageHandler)
{
}

void FDualSenseInputDevice::Tick(float DeltaTime)
{
	TArray<FInputDeviceId> DeviceIds;
	Get().GetAllInputDevices(DeviceIds);
	
	for (FInputDeviceId& Device : DeviceIds)
	{
		if (MappedInputDevices.Contains(Device))
		{
			if (!UDualSenseLibrary::IsConnected(Device.GetId()))
			{
				Disconnect(Device);
			}

			if (
				UDualSenseLibrary::IsConnected(Device.GetId()) &&
				MappedInputDevices[Device].ConnectionState != EInputDeviceConnectionState::Connected
			)
			{
				Reconnect(Device);
			}
			
			if (
				MappedInputDevices[Device].ConnectionState == EInputDeviceConnectionState::Connected
			)
			{
				const FPlatformUserId UserId = GetUserForInputDevice(Device);
				UDualSenseLibrary::UpdateInput(MessageHandler, UserId, Device);
			}
		}
	}
}

void FDualSenseInputDevice::SetDeviceProperty(int32 ControllerId, const FInputDeviceProperty* Property)
{
	if (!Property)
	{
		return;
	}

	if (Property->Name == "InputDeviceLightColor")
	{
		const FInputDeviceLightColorProperty* ColorProperty = static_cast<const FInputDeviceLightColorProperty*>(Property);
		SetLightColor(ControllerId, ColorProperty->Color);
	}
	
	if (Property->Name != "InputDeviceLightColor")
	{
		UDualSenseLibrary::SetTriggers(ControllerId, Property);
	}
}

void FDualSenseInputDevice::SetLightColor(const int32 ControllerId, const FColor Color)
{
	UDualSenseLibrary::UpdateColorOutput(ControllerId, Color);
}

void FDualSenseInputDevice::ResetLightColor(const int32 ControllerId)
{
	UDualSenseLibrary::UpdateColorOutput(ControllerId, FColor::Black);
}

void FDualSenseInputDevice::Reconnect(FInputDeviceId& Device)
{
	MappedInputDevices[Device].ConnectionState = EInputDeviceConnectionState::Connected;
}

void FDualSenseInputDevice::Disconnect(FInputDeviceId& Device)
{
	MappedInputDevices[Device].ConnectionState = EInputDeviceConnectionState::Disconnected;
}

void FDualSenseInputDevice::SetHapticFeedbackValues(const int32 ControllerId, const int32 Hand, const FHapticFeedbackValues& Values)
{
	UDualSenseLibrary::SetHapticFeedbackValues(ControllerId, Hand, &Values);
}

void FDualSenseInputDevice::GetHapticFrequencyRange(float& MinFrequency, float& MaxFrequency) const
{
	MinFrequency = 0.0f;
	MaxFrequency = 1.0f;
}

const TSharedRef<FGenericApplicationMessageHandler>& FDualSenseInputDevice::GetMessageHandler() const
{
	return MessageHandler;
}

bool FDualSenseInputDevice::SupportsForceFeedback(int32 ControllerId)
{
	return true;
}

void FDualSenseInputDevice::SetChannelValues(int32 ControllerId, const FForceFeedbackValues& values)
{
	UDualSenseLibrary::SetVibration(ControllerId, values);
}
