// Copyright (c) 2025 Rafael Valotor/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025
#pragma once

#include "ds5w.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InputCoreTypes.h"
#include "Runtime/ApplicationCore/Public/GenericPlatform/IInputInterface.h"
#include "Runtime/ApplicationCore/Public/GenericPlatform/GenericApplicationMessageHandler.h"
#include <deque>
#include "DualSenseLibrary.generated.h"

/**
 * 
 */
UCLASS()
class WINDOWSDUALSENSE_DS5W_API UDualSenseLibrary : public UObject
{
	GENERATED_BODY()
public:
	DECLARE_EVENT_OneParam(UDualSenseLibrary, FOnDeviceRegistered, int32 ControllerId);
	static FOnDeviceRegistered& OnDeviceRegistered() { return DeviceRegisteredEvent; }

	static bool InitializeLibrary();
	static void ShutdownLibrary();

	// States and Connection Devices
	static int32 ControllersCount;
	static bool Reconnect(int32 ControllerId);
	static bool IsConnected(int32 ControllerId);
	static void SetConnectionIsValid(int32 ControllerId, bool IsValid);

	// Input
	static bool UpdateInput(
		const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler,
		const FPlatformUserId UserId,
		const FInputDeviceId InputDeviceId
	);

	// Haptic Feedback and Effects
	static void SetHapticFeedbackValues(int32 ControllerId, int32 Hand, const FHapticFeedbackValues* Values);
	static int32 GetTrirggersFeedback(int32 ControllerId, EControllerHand& HandTrigger);
	static void ConfigTriggerHapticFeedbackEffect(int32 ControllerId, int32 StartPosition, int32 BeginForce, int32 MiddleForce, int32 EndForce, const EControllerHand& Hand, bool KeepEffect);

	// Effects
	static void NoResitance(int32 ControllerId, const EControllerHand& Hand);
	static void ContinuousResitance(int32 ControllerId, int32 StartPosition, int32 Force, const EControllerHand& Hand);
	static void SectionResitance(int32 ControllerId, int32 StartPosition, int32 EndPosition, const EControllerHand& Hand);
	static void StopEffect(int32 ControllerId, const EControllerHand& Hand);
	static void StopAllEffects(int32 ControllerId);
	static void StopAll(int32 ControllerId);
	static void SetLedPlayerEffects(int32 ControllerId, int32 NumberLeds, int32 BrightnessValue);
	static void SetLedMicEffects(int32 ControllerId, int32 LedMic);

	// Status
	static float GetLevelBattery(int32 ControllerId);

	// Colors, vibration and triggers config
	static void UpdateColorOutput(int32 ControllerId, FColor Color);
	static void SetVibration(int32 ControllerId, const FForceFeedbackValues& Vibration);
	static void SetTriggers(int32 ControllerId, const FInputDeviceProperty* Property);

	static void SetAcceleration(int32 ControllerId, bool bIsAccelerometer);
	static void SetGyroscope(int32 ControllerId, bool bIsGyroscope);
	static void SetTouch1(int32 ControllerId, bool bIsGyroscope);
	static void SetTouch2(int32 ControllerId, bool bIsGyroscope);

private:
	static DS5W::_DeviceEnumInfo* Infos;
	static TMap<int32, DS5W::_DS5InputState> InputState;
	static TMap<int32, DS5W::DS5OutputState> OutputState;
	static TMap<int32, DS5W::DeviceContext> DeviceContexts;

	static TMap<int32, bool> IsInitialized;
	static TMap<int32, float> BatteryLevel;
	static TMap<int32, int32> LeftTriggerFeedback;
	static TMap<int32, int32> RightTriggerFeedback;
	
	static TMap<FInputDeviceId, TMap<FName, bool>> ButtonStates;

	static TMap<int32, bool> EnableAccelerometer;
	static TMap<int32, bool> EnableGyroscope;
	static TMap<int32, bool> EnableTouch1;
	static TMap<int32, bool> EnableTouch2;

	static TSet<int32> PendingOutputUpdates;

	static FOnDeviceRegistered DeviceRegisteredEvent;

	static bool Connection();
	static void CheckButtonInput(
		const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler,
		const FPlatformUserId UserId,
		const FInputDeviceId InputDeviceId,
		const FName ButtonName,
		const bool IsButtonPressed
	);

	static unsigned char CalculateLeftRumble(const FForceFeedbackValues& Values);
	static unsigned char CalculateRightRumble(const FForceFeedbackValues& Values);

	// Output

	static void SendOut(int32 ControllerId);
	static void SmoothBatteryLevel(int32 ControllerId, uint8_t NewValue);
	static int ConvertTo255(float Value);
	static unsigned char ConvertTo255(unsigned char value, unsigned char maxInput);
};
