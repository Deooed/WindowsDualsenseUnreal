#include "Public/DualSenseLibrary.h"

#include <Windows.h>
#include <iostream>
#include <algorithm>
#include "InputCoreTypes.h"

#define MAX_DEVICES 8
#define MAX_HISTORY_BATTERY 15

DS5W::_DeviceEnumInfo* UDualSenseLibrary::Infos;
TMap<int32, DS5W::_DS5InputState> UDualSenseLibrary::InputState;
TMap<int32, DS5W::DS5OutputState> UDualSenseLibrary::OutputState;
TMap<int32, DS5W::DeviceContext> UDualSenseLibrary::DeviceContexts;

TMap<int32, std::deque<uint8_t>> UDualSenseLibrary::BatteryHistories;
TMap<int32, bool> UDualSenseLibrary::EnableAccelerometer;
TMap<int32, bool> UDualSenseLibrary::EnableGyroscope;
TMap<int32, bool> UDualSenseLibrary::EnableTouch1;
TMap<int32, bool> UDualSenseLibrary::EnableTouch2;

TMap<int32, float> UDualSenseLibrary::BatteryLevel;
TMap<int32, int32> UDualSenseLibrary::LeftTriggerFeedback;
TMap<int32, int32> UDualSenseLibrary::RightTriggerFeedback;
TMap<FInputDeviceId, TMap<FName, bool>> UDualSenseLibrary::ButtonStates;

TSet<int32> UDualSenseLibrary::PendingOutputUpdates;

int32 UDualSenseLibrary::ControllersCount = 0;
TMap<int32, bool> UDualSenseLibrary::IsInitialized;

UDualSenseLibrary::FOnDeviceRegistered UDualSenseLibrary::DeviceRegisteredEvent;

bool UDualSenseLibrary::Reconnect(int32 ControllerId)
{
	if (!DeviceContexts.Contains(ControllerId))
	{
		if (ControllersCount == 0)
		{
			if (InitializeLibrary())
			{
				return IsConnected(ControllerId);
			}
		}

		return false;
	}

	if (DS5W_SUCCESS(DS5W::reconnectDevice(&DeviceContexts[ControllerId])))
	{
		return true;
	}

	return false;
}


bool UDualSenseLibrary::InitializeLibrary()
{
	Infos = new DS5W::_DeviceEnumInfo[MAX_DEVICES];

	unsigned int Count = 0;
	switch (DS5W::enumDevices(Infos, MAX_DEVICES, &Count))
	{
	case DS5W_OK:

	case DS5W_E_INSUFFICIENT_BUFFER:
		break;

	default:
		std::cout << "Error enumDevices default: " << GetLastError() << std::endl;
		return false;
	}

	ControllersCount = static_cast<int32>(Count);
	return Connection();
}

bool UDualSenseLibrary::Connection()
{
	for (int32 i = 0; i < ControllersCount; i++)
	{
		if (OutputState.Contains(i))
		{
			ZeroMemory(&OutputState[i], sizeof(DS5W::DS5OutputState));
		}

		if (InputState.Contains(i))
		{
			ZeroMemory(&InputState[i], sizeof(DS5W::_DS5InputState));
		}

		if (DeviceContexts.Contains(i))
		{
			DS5W::freeDeviceContext(&DeviceContexts[i]);
		}

		DS5W::DeviceContext Context;
		if (DS5W_SUCCESS(DS5W::initDeviceContext(&Infos[i], &Context)))
		{
			DS5W::_DS5InputState InState;
			if (!DS5W_SUCCESS(DS5W::getDeviceInputState(&Context, &InState)))
			{
				UE_LOG(LogTemp, Error, TEXT("Error DeviceInputState: %d"), i);
				continue;
			}

			OutputState.Add(i, DS5W::DS5OutputState());
			InputState.Add(i, InState);
			DeviceContexts.Add(i, Context);

			LeftTriggerFeedback.Add(i, 0);
			RightTriggerFeedback.Add(i, 0);

			EnableAccelerometer.Add(i, false);
			EnableGyroscope.Add(i, false);
			EnableTouch1.Add(i, false);
			EnableTouch2.Add(i, false);

			if (IsInitialized.Contains(i))
			{
				IsInitialized[i] = true;
				continue;
			}

			IsInitialized.Add(i, true);
			continue;
		}

		if (IsInitialized.Contains(i))
		{
			IsInitialized[i] = false;
			continue;
		}

		IsInitialized.Add(i, false);
	}

	return true;
}

void UDualSenseLibrary::ShutdownLibrary()
{
	for (int32 i = 0; i < ControllersCount; i++)
	{
		if (!IsInitialized.Contains(i))
		{
			continue;
		}

		if (IsInitialized[i])
		{
			ZeroMemory(&OutputState[i], sizeof(DS5W::DS5OutputState));
			ZeroMemory(&InputState[i], sizeof(DS5W::DS5InputState));

			DS5W::freeDeviceContext(&DeviceContexts[i]);
			UE_LOG(LogTemp, Log, TEXT("DualSense ControllerId %d disconnected with success."), i);
		}
	}

	EnableTouch1.Reset();
	EnableTouch2.Reset();
	EnableGyroscope.Reset();
	EnableAccelerometer.Reset();
	IsInitialized.Reset();
}

bool UDualSenseLibrary::IsConnected(int32 ControllerId)
{
	if (!IsInitialized.Contains(ControllerId))
	{
		return false;
	}

	return IsInitialized[ControllerId];
}

void UDualSenseLibrary::SetConnectionIsValid(int32 ControllerId, bool IsValid)
{
	if (!IsInitialized.Contains(ControllerId))
	{
		return;
	}

	IsInitialized[ControllerId] = IsValid;
}

int32 UDualSenseLibrary::GetTrirggersFeedback(int32 ControllerId, EControllerHand& HandTrigger)
{
	if (!LeftTriggerFeedback.Contains(ControllerId) && !RightTriggerFeedback.Contains(ControllerId))
	{
		return 0;
	}

	if (HandTrigger == EControllerHand::Left)
	{
		return LeftTriggerFeedback[ControllerId];
	}

	return RightTriggerFeedback[ControllerId];
}

void UDualSenseLibrary::CheckButtonInput(
	const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler,
	const FPlatformUserId UserId,
	const FInputDeviceId InputDeviceId,
	const FName ButtonName,
	const bool IsButtonPressed)
{
	TMap<FName, bool>& ButtonStateMap = ButtonStates.FindOrAdd(InputDeviceId);
	const bool PreviousState = ButtonStateMap.FindRef(ButtonName);

	if (IsButtonPressed && !PreviousState)
	{
		InMessageHandler.Get().OnControllerButtonPressed(ButtonName, UserId, InputDeviceId, false);
	}
	if (!IsButtonPressed && PreviousState)
	{
		InMessageHandler.Get().OnControllerButtonReleased(ButtonName, UserId, InputDeviceId, false);
	}

	ButtonStateMap.Add(ButtonName, IsButtonPressed);
}

bool UDualSenseLibrary::UpdateInput(
	const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler,
	const FPlatformUserId UserId,
	const FInputDeviceId InputDeviceId
)
{
	if (!DeviceContexts.Contains(InputDeviceId.GetId()) && !InputState.Contains(InputDeviceId.GetId()))
	{
		return false;
	}

	if (
		DS5W_SUCCESS(DS5W::getDeviceInputState(&DeviceContexts[InputDeviceId.GetId()], &InputState[InputDeviceId.GetId()])))
	{
		const auto ButtonsAndDpad = InputState[InputDeviceId.GetId()].buttonsAndDpad;
		const auto ButtonsA = InputState[InputDeviceId.GetId()].buttonsA;
		const auto ButtonsB = InputState[InputDeviceId.GetId()].buttonsB;

		uint8_t BatteryValue = ((DeviceContexts[InputDeviceId.GetId()]._internal.hidBuffer[0x34] & 0x0F) * 100) / 15;
		SmoothBatteryLevel(InputDeviceId.GetId(), BatteryValue);

		if (
			LeftTriggerFeedback.Contains(InputDeviceId.GetId()) &&
			RightTriggerFeedback.Contains(InputDeviceId.GetId())
		)
		{
			LeftTriggerFeedback[InputDeviceId.GetId()] = InputState[InputDeviceId.GetId()].leftTriggerFeedback;
			RightTriggerFeedback[InputDeviceId.GetId()] = InputState[InputDeviceId.GetId()].rightTriggerFeedback;
		}

		// Buttons
		const bool bCross = ButtonsAndDpad & DS5W_ISTATE_BTX_CROSS;
		const bool bCircle = ButtonsAndDpad & DS5W_ISTATE_BTX_CIRCLE;
		const bool bSquare = ButtonsAndDpad & DS5W_ISTATE_BTX_SQUARE;
		const bool bTriangle = ButtonsAndDpad & DS5W_ISTATE_BTX_TRIANGLE;
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FGamepadKeyNames::FaceButtonTop, bTriangle);
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FGamepadKeyNames::FaceButtonBottom, bCross);
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FGamepadKeyNames::FaceButtonRight, bCircle);
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FGamepadKeyNames::FaceButtonLeft, bSquare);

		// DPad
		const bool bDPadUp = ButtonsAndDpad & DS5W_ISTATE_DPAD_UP;
		const bool bDPadDown = ButtonsAndDpad & DS5W_ISTATE_DPAD_DOWN;
		const bool bDPadLeft = ButtonsAndDpad & DS5W_ISTATE_DPAD_LEFT;
		const bool bDPadRight = ButtonsAndDpad & DS5W_ISTATE_DPAD_RIGHT;
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FGamepadKeyNames::DPadUp, bDPadUp);
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FGamepadKeyNames::DPadDown, bDPadDown);
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FGamepadKeyNames::DPadLeft, bDPadLeft);
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FGamepadKeyNames::DPadRight, bDPadRight);

		// Shoulders
		const bool bLeftBumper = ButtonsA & DS5W_ISTATE_BTN_A_LEFT_BUMPER;
		const bool bRightBumper = ButtonsA & DS5W_ISTATE_BTN_A_RIGHT_BUMPER;
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FGamepadKeyNames::LeftShoulder, bLeftBumper);
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FGamepadKeyNames::RightShoulder,bRightBumper);

		// Push Stick
		const bool PushLeftStick = ButtonsA & DS5W_ISTATE_BTN_A_LEFT_STICK;
		const bool PushRightStick = ButtonsA & DS5W_ISTATE_BTN_A_RIGHT_STICK;
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FGamepadKeyNames::LeftThumb, PushLeftStick);
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FGamepadKeyNames::RightThumb, PushRightStick);

		
		// Specials Actions
		const bool Mic = ButtonsB & DS5W_ISTATE_BTN_B_MIC_BUTTON;
		const bool TouchPad = ButtonsB & DS5W_ISTATE_BTN_B_PAD_BUTTON;
		const bool Playstation = ButtonsB & DS5W_ISTATE_BTN_B_PLAYSTATION_LOGO;
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FName("PS_Mic"), Mic);
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FName("PS_TouchButton"), TouchPad);
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FName("PS_Button"), Playstation);

		const bool Start = ButtonsA & DS5W_ISTATE_BTN_A_MENU;
		const bool Select = ButtonsA & DS5W_ISTATE_BTN_A_SELECT;
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FName("PS_Menu"), Start);
		CheckButtonInput(InMessageHandler, UserId, InputDeviceId, FName("PS_Share"), Select);


		// Analog left
		const float LeftX = InputState[InputDeviceId.GetId()].leftStick.x / 128.f;
		const float LeftY = InputState[InputDeviceId.GetId()].leftStick.y / 128.f;
		InMessageHandler.Get().OnControllerAnalog(FGamepadKeyNames::LeftAnalogX, UserId, InputDeviceId,
		                                          LeftX);
		InMessageHandler.Get().OnControllerAnalog(FGamepadKeyNames::LeftAnalogY, UserId, InputDeviceId, LeftY);
		

		// Analog right
		const float RightX = InputState[InputDeviceId.GetId()].rightStick.x / 128.0f;
		const float RightY = InputState[InputDeviceId.GetId()].rightStick.y / 128.0f;
		InMessageHandler.Get().OnControllerAnalog(FGamepadKeyNames::RightAnalogX, UserId, InputDeviceId,
		                                          RightX);
		InMessageHandler.Get().OnControllerAnalog(FGamepadKeyNames::RightAnalogY, UserId, InputDeviceId,
		                                          RightY);

		// Triggers
		const float TriggerL = InputState[InputDeviceId.GetId()].leftTrigger / 256.0f;
		const float TriggerR = InputState[InputDeviceId.GetId()].rightTrigger / 256.0f;
		InMessageHandler.Get().OnControllerAnalog(FGamepadKeyNames::LeftTriggerAnalog, UserId, InputDeviceId,
		                                          TriggerL);
		InMessageHandler.Get().OnControllerAnalog(FGamepadKeyNames::RightTriggerAnalog, UserId, InputDeviceId,
		                                          TriggerR);


		if (EnableTouch1.Contains(InputDeviceId.GetId()) && EnableTouch2.Contains(InputDeviceId.GetId()))
		{
			// Accelerometer, Gyroscope and Touchs
			const unsigned int MaxX = 2000;
			const unsigned int MaxY = 2048;

			if (EnableTouch1[InputDeviceId.GetId()])
			{
				DS5W::Touch TouchPoint1 = InputState[InputDeviceId.GetId()].touchPoint1;
				float Touch1X = (2.0f * TouchPoint1.x / MaxX) - 1.0f;
				float Touch1Y = (2.0f * TouchPoint1.y / MaxY) - 1.0f;
				InMessageHandler->OnControllerAnalog(FName("DualSense_Touch1_X"), UserId, InputDeviceId, Touch1X);
				InMessageHandler->OnControllerAnalog(FName("DualSense_Touch1_Y"), UserId, InputDeviceId, Touch1Y);
			}

			if (EnableTouch2[InputDeviceId.GetId()])
			{
				DS5W::Touch TouchPoint2 = InputState[InputDeviceId.GetId()].touchPoint2;
				float Touch2X = (2.0f * TouchPoint2.x / MaxX) - 1.0f;
				float Touch2Y = (2.0f * TouchPoint2.y / MaxY) - 1.0f;
				InMessageHandler->OnControllerAnalog(FName("DualSense_Touch1_X"), UserId, InputDeviceId, Touch2X);
				InMessageHandler->OnControllerAnalog(FName("DualSense_Touch2_Y"), UserId, InputDeviceId, Touch2Y);
			}
		}


		if (EnableAccelerometer.Contains(InputDeviceId.GetId()) && EnableGyroscope.Contains(InputDeviceId.GetId()))
		{
			DS5W::Vector3 Accelerometer = InputState[InputDeviceId.GetId()].accelerometer;
			DS5W::Vector3 Gyroscope = InputState[InputDeviceId.GetId()].gyroscope;

			if (EnableAccelerometer[InputDeviceId.GetId()])
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelerometer: %hd, %hd, %hd"), Accelerometer.x, Accelerometer.y,
				       Accelerometer.z);
				InMessageHandler.Get().OnControllerAnalog(EKeys::Acceleration.GetFName(), UserId, InputDeviceId,
				                                          Accelerometer.x);
				InMessageHandler.Get().OnControllerAnalog(EKeys::Acceleration.GetFName(), UserId, InputDeviceId,
				                                          Accelerometer.y);
				InMessageHandler.Get().OnControllerAnalog(EKeys::Acceleration.GetFName(), UserId, InputDeviceId,
				                                          Accelerometer.z);
			}

			if (EnableGyroscope[InputDeviceId.GetId()])
			{
				UE_LOG(LogTemp, Warning, TEXT("Gyroscope: %hd, %hd, %hd"), Gyroscope.x, Gyroscope.y, Gyroscope.z);
				InMessageHandler.Get().OnControllerAnalog(EKeys::RotationRate.GetFName(), UserId, InputDeviceId,
				                                          Gyroscope.x);
				InMessageHandler.Get().OnControllerAnalog(EKeys::RotationRate.GetFName(), UserId, InputDeviceId,
				                                          Gyroscope.y);
				InMessageHandler.Get().OnControllerAnalog(EKeys::RotationRate.GetFName(), UserId, InputDeviceId,
				                                          Gyroscope.z);
			}

			if (EnableGyroscope[InputDeviceId.GetId()] && EnableAccelerometer[InputDeviceId.GetId()])
			{
				FVector Tilt = FVector(Accelerometer.x, Accelerometer.y, Accelerometer.z) + FVector(
					Gyroscope.x, Gyroscope.y, Gyroscope.z);
				UE_LOG(LogTemp, Warning, TEXT("Tilt: %s"), *Tilt.ToString());

				InMessageHandler.Get().OnControllerAnalog(EKeys::Tilt.GetFName(), UserId, InputDeviceId, Tilt.X);
				InMessageHandler.Get().OnControllerAnalog(EKeys::Tilt.GetFName(), UserId, InputDeviceId, Tilt.Y);
				InMessageHandler.Get().OnControllerAnalog(EKeys::Tilt.GetFName(), UserId, InputDeviceId, Tilt.Z);
			}
		}

		for (int32 ControllerId : PendingOutputUpdates)
		{
			if (DeviceContexts.Contains(ControllerId) && OutputState.Contains(ControllerId))
			{
				DS5W::setDeviceOutputState(&DeviceContexts[ControllerId], &OutputState[ControllerId]);
			}
		}

		PendingOutputUpdates.Empty(); // Clear after processing

		return true;
	}
	return false;
}

void UDualSenseLibrary::UpdateColorOutput(const int32 ControllerId, const FColor Color)
{
	if (!OutputState.Contains(ControllerId))
	{
		return;
	}

	OutputState[ControllerId].lightbar.r = Color.R;
	OutputState[ControllerId].lightbar.g = Color.G;
	OutputState[ControllerId].lightbar.b = Color.B;
}

void UDualSenseLibrary::SetVibration(int32 ControllerId, const FForceFeedbackValues& Vibration)
{
	if (!OutputState.Contains(ControllerId))
	{
		return;
	}

	unsigned char NewLeftRumble = CalculateLeftRumble(Vibration);
	unsigned char NewRightRumble = CalculateRightRumble(Vibration);

	if (OutputState[ControllerId].leftRumble == NewLeftRumble &&
		OutputState[ControllerId].rightRumble == NewRightRumble)
	{
		return; // No need to update
	}

	OutputState[ControllerId].leftRumble = NewLeftRumble;
	OutputState[ControllerId].rightRumble = NewRightRumble;

	PendingOutputUpdates.Add(ControllerId); // Mark for update
}


unsigned char UDualSenseLibrary::CalculateLeftRumble(const FForceFeedbackValues& Values)
{
	float CombinedLeft = Values.LeftLarge * 0.8f + Values.LeftSmall * 0.2f;
	return static_cast<unsigned char>(FMath::Clamp(CombinedLeft * 255.0f, 0.0f, 255.0f));
}

unsigned char UDualSenseLibrary::CalculateRightRumble(const FForceFeedbackValues& Values)
{
	float CombinedRight = Values.RightLarge * 0.8f + Values.RightSmall * 0.2f;
	return static_cast<unsigned char>(FMath::Clamp(CombinedRight * 255.0f, 0.0f, 255.0f));
}

void UDualSenseLibrary::SetHapticFeedbackValues(int32 ControllerId, int32 Hand, const FHapticFeedbackValues* Values)
{
	if (!OutputState.Contains(ControllerId)) return;

	bool bChanged = false;

	if (Hand == static_cast<int32>(EControllerHand::Left) || Hand == static_cast<int32>(EControllerHand::AnyHand))
	{
		unsigned char NewFreq = ConvertTo255(Values->Frequency);
		if (OutputState[ControllerId].leftTriggerEffect.EffectEx.frequency != NewFreq)
		{
			OutputState[ControllerId].leftTriggerEffect.EffectEx.frequency = NewFreq;
			bChanged = true;
		}
	}

	if (Hand == static_cast<int32>(EControllerHand::Right) || Hand == static_cast<int32>(EControllerHand::AnyHand))
	{
		unsigned char NewFreq = ConvertTo255(Values->Frequency);
		if (OutputState[ControllerId].rightTriggerEffect.EffectEx.frequency != NewFreq)
		{
			OutputState[ControllerId].rightTriggerEffect.EffectEx.frequency = NewFreq;
			bChanged = true;
		}
	}

	if (bChanged)
	{
		PendingOutputUpdates.Add(ControllerId);
	}
}



void UDualSenseLibrary::SetTriggers(int32 ControllerId, const FInputDeviceProperty* Property)
{
    if (!OutputState.Contains(ControllerId)) return;

    bool bChanged = false;

    if (Property->Name == FName("InputDeviceTriggerResistance"))
    {
        const FInputDeviceTriggerResistanceProperty* Resistance = static_cast<const FInputDeviceTriggerResistanceProperty*>(Property);
        
        if (Resistance->AffectedTriggers == EInputDeviceTriggerMask::Left || Resistance->AffectedTriggers == EInputDeviceTriggerMask::All)
        {
            if (OutputState[ControllerId].leftTriggerEffect._u1_raw[0] != ConvertTo255(Resistance->StartPosition, 8) ||
                OutputState[ControllerId].leftTriggerEffect._u1_raw[1] != ConvertTo255(Resistance->EndPosition, 8) ||
                OutputState[ControllerId].leftTriggerEffect._u1_raw[2] != ConvertTo255(Resistance->StartStrengh, 9) ||
                OutputState[ControllerId].leftTriggerEffect._u1_raw[3] != ConvertTo255(Resistance->EndStrengh, 9))
            {
                OutputState[ControllerId].leftTriggerEffect.effectType = DS5W::_TriggerEffectType::SectionResitance;
                OutputState[ControllerId].leftTriggerEffect._u1_raw[0] = ConvertTo255(Resistance->StartPosition, 8);
                OutputState[ControllerId].leftTriggerEffect._u1_raw[1] = ConvertTo255(Resistance->EndPosition, 8);
                OutputState[ControllerId].leftTriggerEffect._u1_raw[2] = ConvertTo255(Resistance->StartStrengh, 9);
                OutputState[ControllerId].leftTriggerEffect._u1_raw[3] = ConvertTo255(Resistance->EndStrengh, 9);
                bChanged = true;
            }
        }

        if (Resistance->AffectedTriggers == EInputDeviceTriggerMask::Right || Resistance->AffectedTriggers == EInputDeviceTriggerMask::All)
        {
            if (OutputState[ControllerId].rightTriggerEffect._u1_raw[0] != ConvertTo255(Resistance->StartPosition, 8) ||
                OutputState[ControllerId].rightTriggerEffect._u1_raw[1] != ConvertTo255(Resistance->EndPosition, 8) ||
                OutputState[ControllerId].rightTriggerEffect._u1_raw[2] != ConvertTo255(Resistance->StartStrengh, 9) ||
                OutputState[ControllerId].rightTriggerEffect._u1_raw[3] != ConvertTo255(Resistance->EndStrengh, 9))
            {
                OutputState[ControllerId].rightTriggerEffect.effectType = DS5W::_TriggerEffectType::SectionResitance;
                OutputState[ControllerId].rightTriggerEffect._u1_raw[0] = ConvertTo255(Resistance->StartPosition, 8);
                OutputState[ControllerId].rightTriggerEffect._u1_raw[1] = ConvertTo255(Resistance->EndPosition, 8);
                OutputState[ControllerId].rightTriggerEffect._u1_raw[2] = ConvertTo255(Resistance->StartStrengh, 9);
                OutputState[ControllerId].rightTriggerEffect._u1_raw[3] = ConvertTo255(Resistance->EndStrengh, 9);
                bChanged = true;
            }
        }
    }

    if (bChanged)
    {
        PendingOutputUpdates.Add(ControllerId);
    }
}


void UDualSenseLibrary::SetAcceleration(int32 ControllerId, bool bIsAccelerometer)
{
	if (!EnableAccelerometer.Contains(ControllerId))
	{
		return;
	}

	EnableAccelerometer.Add(ControllerId, bIsAccelerometer);
}

void UDualSenseLibrary::SetGyroscope(int32 ControllerId, bool bIsGyroscope)
{
	if (!EnableGyroscope.Contains(ControllerId))
	{
		return;
	}

	EnableGyroscope.Add(ControllerId, bIsGyroscope);
}

void UDualSenseLibrary::SetTouch1(int32 ControllerId, bool bIsGyroscope)
{
	if (!EnableTouch1.Contains(ControllerId))
	{
		return;
	}

	EnableTouch1.Add(ControllerId, bIsGyroscope);
}

void UDualSenseLibrary::SetTouch2(int32 ControllerId, bool bIsGyroscope)
{
	if (!EnableTouch2.Contains(ControllerId))
	{
		return;
	}

	EnableTouch2.Add(ControllerId, bIsGyroscope);
}

void UDualSenseLibrary::ConfigTriggerHapticFeedbackEffect(
    int32 ControllerId, 
    int32 StartPosition, 
    int32 BeginForce,
    int32 MiddleForce, 
    int32 EndForce, 
    const EControllerHand& Hand, 
    bool KeepEffect)
{
    if (!OutputState.Contains(ControllerId)) return;

    bool bChanged = false;

    if (Hand == EControllerHand::Left || Hand == EControllerHand::AnyHand)
    {

        // Ensure effect type is set
        if (OutputState[ControllerId].leftTriggerEffect.effectType != DS5W::_TriggerEffectType::EffectEx)
        {
            OutputState[ControllerId].leftTriggerEffect.effectType = DS5W::_TriggerEffectType::EffectEx;
            bChanged = true;
        }

        // Check if values actually changed
        if (OutputState[ControllerId].leftTriggerEffect.EffectEx.startPosition != ConvertTo255(StartPosition, 8) ||
            OutputState[ControllerId].leftTriggerEffect.EffectEx.beginForce != ConvertTo255(BeginForce, 9) ||
            OutputState[ControllerId].leftTriggerEffect.EffectEx.middleForce != ConvertTo255(MiddleForce, 9) ||
            OutputState[ControllerId].leftTriggerEffect.EffectEx.endForce != ConvertTo255(EndForce, 9) ||
            OutputState[ControllerId].leftTriggerEffect.EffectEx.keepEffect != KeepEffect)
        {
            OutputState[ControllerId].leftTriggerEffect.EffectEx.startPosition = ConvertTo255(StartPosition, 8);
            OutputState[ControllerId].leftTriggerEffect.EffectEx.keepEffect = KeepEffect;
            OutputState[ControllerId].leftTriggerEffect.EffectEx.beginForce = ConvertTo255(BeginForce, 9);
            OutputState[ControllerId].leftTriggerEffect.EffectEx.middleForce = ConvertTo255(MiddleForce, 9);
            OutputState[ControllerId].leftTriggerEffect.EffectEx.endForce = ConvertTo255(EndForce, 9);
            OutputState[ControllerId].leftTriggerEffect.EffectEx.frequency = 0;  // Reset frequency to ensure consistency

            bChanged = true;
        }
    }

    if (Hand == EControllerHand::Right || Hand == EControllerHand::AnyHand)
    {


        // Ensure effect type is set
        if (OutputState[ControllerId].rightTriggerEffect.effectType != DS5W::_TriggerEffectType::EffectEx)
        {
            OutputState[ControllerId].rightTriggerEffect.effectType = DS5W::_TriggerEffectType::EffectEx;
            bChanged = true;
        }

        // Check if values actually changed
        if (OutputState[ControllerId].rightTriggerEffect.EffectEx.startPosition != ConvertTo255(StartPosition, 8) ||
            OutputState[ControllerId].rightTriggerEffect.EffectEx.beginForce != ConvertTo255(BeginForce, 9) ||
            OutputState[ControllerId].rightTriggerEffect.EffectEx.middleForce != ConvertTo255(MiddleForce, 9) ||
            OutputState[ControllerId].rightTriggerEffect.EffectEx.endForce != ConvertTo255(EndForce, 9) ||
            OutputState[ControllerId].rightTriggerEffect.EffectEx.keepEffect != KeepEffect)
        {
            OutputState[ControllerId].rightTriggerEffect.EffectEx.startPosition = ConvertTo255(StartPosition, 8);
            OutputState[ControllerId].rightTriggerEffect.EffectEx.keepEffect = KeepEffect;
            OutputState[ControllerId].rightTriggerEffect.EffectEx.beginForce = ConvertTo255(BeginForce, 9);
            OutputState[ControllerId].rightTriggerEffect.EffectEx.middleForce = ConvertTo255(MiddleForce, 9);
            OutputState[ControllerId].rightTriggerEffect.EffectEx.endForce = ConvertTo255(EndForce, 9);
            OutputState[ControllerId].rightTriggerEffect.EffectEx.frequency = 0;  // Reset frequency

            bChanged = true;
        }
    }

    if (bChanged)
    {
        PendingOutputUpdates.Add(ControllerId);
    }
}


void UDualSenseLibrary::NoResitance(int32 ControllerId, const EControllerHand& Hand)
{
	if (!OutputState.Contains(ControllerId)) return;

	bool bChanged = false;

	if (Hand == EControllerHand::Left || Hand == EControllerHand::AnyHand)
	{
		if (OutputState[ControllerId].leftTriggerEffect.effectType != DS5W::_TriggerEffectType::NoResitance)
		{
			OutputState[ControllerId].leftTriggerEffect.effectType = DS5W::_TriggerEffectType::NoResitance;
			bChanged = true;
		}
	}

	if (Hand == EControllerHand::Right || Hand == EControllerHand::AnyHand)
	{
		if (OutputState[ControllerId].rightTriggerEffect.effectType != DS5W::_TriggerEffectType::NoResitance)
		{
			OutputState[ControllerId].rightTriggerEffect.effectType = DS5W::_TriggerEffectType::NoResitance;
			bChanged = true;
		}
	}

	if (bChanged)
	{
		PendingOutputUpdates.Add(ControllerId);
	}
}


void UDualSenseLibrary::ContinuousResitance(int32 ControllerId, int32 StartPosition, int32 Force, const EControllerHand& Hand)
{
	if (!OutputState.Contains(ControllerId)) return;

	bool bChanged = false;

	if (Hand == EControllerHand::Left || Hand == EControllerHand::AnyHand)
	{
		if (OutputState[ControllerId].leftTriggerEffect.effectType != DS5W::_TriggerEffectType::ContinuousResitance ||
			OutputState[ControllerId].leftTriggerEffect.Continuous.startPosition != ConvertTo255(StartPosition, 8) ||
			OutputState[ControllerId].leftTriggerEffect.Continuous.force != ConvertTo255(Force, 9))
		{
			OutputState[ControllerId].leftTriggerEffect.effectType = DS5W::_TriggerEffectType::ContinuousResitance;
			OutputState[ControllerId].leftTriggerEffect.Continuous.startPosition = ConvertTo255(StartPosition, 8);
			OutputState[ControllerId].leftTriggerEffect.Continuous.force = ConvertTo255(Force, 9);
			bChanged = true;
		}
	}

	if (Hand == EControllerHand::Right || Hand == EControllerHand::AnyHand)
	{
		if (OutputState[ControllerId].rightTriggerEffect.effectType != DS5W::_TriggerEffectType::ContinuousResitance ||
			OutputState[ControllerId].rightTriggerEffect.Continuous.startPosition != ConvertTo255(StartPosition, 8) ||
			OutputState[ControllerId].rightTriggerEffect.Continuous.force != ConvertTo255(Force, 9))
		{
			OutputState[ControllerId].rightTriggerEffect.effectType = DS5W::_TriggerEffectType::ContinuousResitance;
			OutputState[ControllerId].rightTriggerEffect.Continuous.startPosition = ConvertTo255(StartPosition, 8);
			OutputState[ControllerId].rightTriggerEffect.Continuous.force = ConvertTo255(Force, 9);
			bChanged = true;
		}
	}

	if (bChanged)
	{
		PendingOutputUpdates.Add(ControllerId);
	}
}


void UDualSenseLibrary::SectionResitance(int32 ControllerId, int32 StartPosition, int32 EndPosition, const EControllerHand& Hand)
{
	if (!OutputState.Contains(ControllerId)) return;

	bool bChanged = false;

	if (Hand == EControllerHand::Left || Hand == EControllerHand::AnyHand)
	{
		if (OutputState[ControllerId].leftTriggerEffect.effectType != DS5W::_TriggerEffectType::SectionResitance ||
			OutputState[ControllerId].leftTriggerEffect.Section.startPosition != ConvertTo255(StartPosition, 8) ||
			OutputState[ControllerId].leftTriggerEffect.Section.endPosition != ConvertTo255(EndPosition, 8))
		{
			OutputState[ControllerId].leftTriggerEffect.effectType = DS5W::_TriggerEffectType::SectionResitance;
			OutputState[ControllerId].leftTriggerEffect.Section.startPosition = ConvertTo255(StartPosition, 8);
			OutputState[ControllerId].leftTriggerEffect.Section.endPosition = ConvertTo255(EndPosition, 8);
			bChanged = true;
		}
	}

	if (Hand == EControllerHand::Right || Hand == EControllerHand::AnyHand)
	{
		if (OutputState[ControllerId].rightTriggerEffect.effectType != DS5W::_TriggerEffectType::SectionResitance ||
			OutputState[ControllerId].rightTriggerEffect.Section.startPosition != ConvertTo255(StartPosition, 8) ||
			OutputState[ControllerId].rightTriggerEffect.Section.endPosition != ConvertTo255(EndPosition, 8))
		{
			OutputState[ControllerId].rightTriggerEffect.effectType = DS5W::_TriggerEffectType::SectionResitance;
			OutputState[ControllerId].rightTriggerEffect.Section.startPosition = ConvertTo255(StartPosition, 8);
			OutputState[ControllerId].rightTriggerEffect.Section.endPosition = ConvertTo255(EndPosition, 8);
			bChanged = true;
		}
	}

	if (bChanged)
	{
		PendingOutputUpdates.Add(ControllerId);
	}
}


void UDualSenseLibrary::StopEffect(int32 ControllerId, const EControllerHand& Hand)
{
	if (!OutputState.Contains(ControllerId)) return;

	bool bChanged = false;

	if (Hand == EControllerHand::Left || Hand == EControllerHand::AnyHand)
	{
		if (OutputState[ControllerId].leftTriggerEffect.effectType != DS5W::_TriggerEffectType::NoResitance)
		{
			OutputState[ControllerId].leftTriggerEffect.effectType = DS5W::_TriggerEffectType::NoResitance;
			bChanged = true;
		}
	}

	if (Hand == EControllerHand::Right || Hand == EControllerHand::AnyHand)
	{
		if (OutputState[ControllerId].rightTriggerEffect.effectType != DS5W::_TriggerEffectType::NoResitance)
		{
			OutputState[ControllerId].rightTriggerEffect.effectType = DS5W::_TriggerEffectType::NoResitance;
			bChanged = true;
		}
	}

	if (bChanged)
	{
		PendingOutputUpdates.Add(ControllerId);
	}
}


void UDualSenseLibrary::StopAllEffects(const int32 ControllerId)
{
	if (!OutputState.Contains(ControllerId)) return;

	if (OutputState[ControllerId].leftTriggerEffect.effectType == DS5W::_TriggerEffectType::NoResitance &&
		OutputState[ControllerId].rightTriggerEffect.effectType == DS5W::_TriggerEffectType::NoResitance)
	{
		return; // No update needed
	}

	OutputState[ControllerId].leftTriggerEffect.effectType = DS5W::_TriggerEffectType::NoResitance;
	OutputState[ControllerId].rightTriggerEffect.effectType = DS5W::_TriggerEffectType::NoResitance;

	PendingOutputUpdates.Add(ControllerId);
}


void UDualSenseLibrary::StopAll(int32 ControllerId)
{
	if (!OutputState.Contains(ControllerId)) return;

	DS5W::DS5OutputState& State = OutputState[ControllerId];

	// Check if state is already zeroed
	if (State.leftRumble == 0 && State.rightRumble == 0 &&
		State.leftTriggerEffect.effectType == DS5W::_TriggerEffectType::NoResitance &&
		State.rightTriggerEffect.effectType == DS5W::_TriggerEffectType::NoResitance &&
		State.lightbar.r == 0 && State.lightbar.g == 0 && State.lightbar.b == 0 &&
		State.playerLeds.bitmask == 0 && State.microphoneLed == DS5W::MicLed::OFF)
	{
		return; // No update needed
	}

	ZeroMemory(&State, sizeof(DS5W::DS5OutputState));

	PendingOutputUpdates.Add(ControllerId);
}


void UDualSenseLibrary::SetLedPlayerEffects(int32 ControllerId, int32 NumberLeds, int32 BrightnessValue)
{
	if (!OutputState.Contains(ControllerId)) return;

	uint8_t NewBitmask = 0x00;
	NumberLeds = FMath::Clamp(NumberLeds, 0, 3);
    
	if (NumberLeds >= 1) NewBitmask |= DS5W_OSTATE_PLAYER_LED_LEFT | DS5W_OSTATE_PLAYER_LED_RIGHT;
	if (NumberLeds >= 2) NewBitmask |= DS5W_OSTATE_PLAYER_LED_MIDDLE_RIGHT | DS5W_OSTATE_PLAYER_LED_MIDDLE_LEFT;
	if (NumberLeds == 3) NewBitmask |= DS5W_OSTATE_PLAYER_LED_MIDDLE;

	DS5W::LedBrightness NewBrightness = DS5W::LedBrightness::LOW;
	if (BrightnessValue == 1) NewBrightness = DS5W::LedBrightness::MEDIUM;
	if (BrightnessValue == 2) NewBrightness = DS5W::LedBrightness::HIGH;

	if (OutputState[ControllerId].playerLeds.bitmask != NewBitmask ||
		OutputState[ControllerId].playerLeds.brightness != NewBrightness)
	{
		OutputState[ControllerId].playerLeds.bitmask = NewBitmask;
		OutputState[ControllerId].playerLeds.brightness = NewBrightness;
		PendingOutputUpdates.Add(ControllerId);
	}
}


void UDualSenseLibrary::SetLedMicEffects(int32 ControllerId, int32 LedMic)
{
    if (!OutputState.Contains(ControllerId)) return;

    DS5W::MicLed NewMicLedState = DS5W::MicLed::OFF;
    if (LedMic == 1) NewMicLedState = DS5W::MicLed::ON;
    if (LedMic == 2) NewMicLedState = DS5W::MicLed::PULSE;

    if (OutputState[ControllerId].microphoneLed != NewMicLedState)
    {
        OutputState[ControllerId].microphoneLed = NewMicLedState;
        PendingOutputUpdates.Add(ControllerId);
    }
}



void UDualSenseLibrary::SmoothBatteryLevel(int32 ControllerId, uint8_t NewValue)
{
	if (!BatteryHistories.Contains(ControllerId))
	{
		BatteryHistories.Add(ControllerId, std::deque<uint8_t>(0));
	}
	
	auto& History = BatteryHistories[ControllerId];
	History.push_back(NewValue);

	if (History.size() > MAX_HISTORY_BATTERY)
	{
		History.pop_front();
	}

	int32 Sum = 0;
	for (int32 Value : History)
	{
		Sum += Value;
	}

	if (!BatteryLevel.Contains(ControllerId))
	{
		BatteryLevel.Add(ControllerId, 0);
	}

	float BatterValue = Sum / History.size();
	BatteryLevel[ControllerId] =  BatterValue;
}

float UDualSenseLibrary::GetLevelBattery(int32 ControllerId)
{
	if (!BatteryLevel.Contains(ControllerId))
	{
		BatteryLevel.Add(ControllerId, 0.0f);
	}

	return BatteryLevel[ControllerId];
}

unsigned char UDualSenseLibrary::ConvertTo255(unsigned char value, unsigned char maxInput)
{
	return static_cast<unsigned char>((value * 255) / maxInput);
}


int UDualSenseLibrary::ConvertTo255(const float Value)
{
	const float Min = 0;
	const float Max = 1.0;
	const float NormalizedPosition = (Value - Min) / (Max - Min);
	const int Value255 = static_cast<int>(NormalizedPosition * 255);
	return std::clamp(Value255, 0, 255);
}
