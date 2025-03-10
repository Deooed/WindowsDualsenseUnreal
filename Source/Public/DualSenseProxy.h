// Copyright (c) 2025 Rafael Valotor/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InputCoreTypes.h"
#include "Runtime/ApplicationCore/Public/GenericPlatform/IInputInterface.h"
#include "DualSenseProxy.generated.h"

UENUM(BlueprintType)
enum class ELedMicEnum : uint8
{
	MicOff UMETA(DisplayName = "Mic Off"),
	MicOn UMETA(DisplayName = "Mic On"),
	Pulse UMETA(DisplayName = "Pulse")
};

UENUM(BlueprintType)
enum class ELedPlayerEnum : uint8
{
	Off UMETA(DisplayName = "Led Player Off"),
	One UMETA(DisplayName = "Led One"),
	Two UMETA(DisplayName = "Led Two"),
	Three UMETA(DisplayName = "Led Three")
};

UENUM(BlueprintType)
enum class ELedBrightnessEnum : uint8
{
	Low UMETA(DisplayName = "Brightness Low"),
	Medium UMETA(DisplayName = "Brightness Medium"),
	High UMETA(DisplayName = "Brightness High")
};


/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class WINDOWSDUALSENSE_DS5W_API UDualSenseProxy : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DualSense")
	static bool DeviceIsConnected(int32 ControllerId);

	UFUNCTION(BlueprintCallable, Category = "DualSense")
	static bool DeviceReconnect(int32 ControllerId);

	UFUNCTION(BlueprintCallable, Category = "DualSense")
	static float LevelBatteryDevice(int32 ControllerId);
	
	// return feedback trigger
	UFUNCTION(BlueprintCallable, Category = "DualSense Triggers Feedback")
	static int32 GetTriggerLeftForceFeedback(int32 ControllerId);

	// return feedback trigger
	UFUNCTION(BlueprintCallable, Category = "DualSense Triggers Feedback")
	static int32 GetTriggerRightForceFeedback(int32 ControllerId);

	UFUNCTION(BlueprintCallable, Category = "DualSense Trigger Haptic Effects")
	static void SetTriggerHapticFeedbackEffect(int32 ControllerId, int32 StartPosition, int32 BeginForce,
	                                           int32 MiddleForce, int32 EndForce, EControllerHand Hand,
	                                           bool KeepEffect);

	UFUNCTION(BlueprintCallable, Category = "DualSense Effects")
	static void EffectNoResitance(int32 ControllerId, EControllerHand HandResistence);

	UFUNCTION(BlueprintCallable, Category = "DualSense Effects")
	static void EffectContinuousResitance(int32 ControllerId, int32 StartPosition, int32 Force, EControllerHand Hand);

	UFUNCTION(BlueprintCallable, Category = "DualSense Effects")
	static void EffectSectionResitance(int32 ControllerId, int32 StartPosition, int32 EndPosition,
	                                   EControllerHand Hand);

	UFUNCTION(BlueprintCallable, Category = "DualSense Effects")
	static void StopTriggerEffect(int32 ControllerId, EControllerHand HandStop);

	UFUNCTION(BlueprintCallable, Category = "DualSense Effects")
	static void StopAllTriggersEffects(int32 ControllerId);

	UFUNCTION(BlueprintCallable, Category = "DualSense Effects")
	static void ResetEffects(int32 ControllerId);

	UFUNCTION(BlueprintCallable, Category = "DualSense Led Effects")
	static void LedColorEffects(int32 ControllerId, FColor Color);

	UFUNCTION(BlueprintCallable, Category = "DualSense Led Effects")
	static void LedPlayerEffects(int32 ControllerId, ELedPlayerEnum Value, ELedBrightnessEnum Brightness);

	UFUNCTION(BlueprintCallable, Category = "DualSense Led Effects")
	static void LedMicEffects(int32 ControllerId, ELedMicEnum Value);

	UFUNCTION(BlueprintCallable, Category = "DualSense Gyroscope and Accelerometer")
	static void EnableAccelerometerValues(int32 ControllerId, bool bEnableAccelerometer);

	UFUNCTION(BlueprintCallable, Category = "DualSense Gyroscope and Accelerometer")
	static void EnableGyroscopeValues(int32 ControllerId, bool bEnableGyroscope);

	UFUNCTION(BlueprintCallable, Category = "DualSense Touch Pad")
	static void EnableTouch1(int32 ControllerId, bool bEnableGyroscope);

	UFUNCTION(BlueprintCallable, Category = "DualSense Touch Pad")
	static void EnableTouch2(int32 ControllerId, bool bEnableGyroscope);

	
};
