// Copyright Epic Games, Inc. All Rights Reserved.

#include "WindowsDualsense_ds5w/Public/WindowsDualsense_ds5w.h"
#include "InputCoreTypes.h"
#include "Runtime/ApplicationCore/Public/GenericPlatform/IInputInterface.h"
#include "Misc/Paths.h"
#include "FDualSenseInputDevice.h"

#define LOCTEXT_NAMESPACE "FWindowsDualsense_ds5wModule"
#define MAX_CONTROLLERS_SUPPORTED 8

void FWindowsDualsense_ds5wModule::StartupModule()
{
	IModularFeatures::Get().RegisterModularFeature(IInputDeviceModule::GetModularFeatureName(), this);

	FString EnginePluginPath = FPaths::Combine(FPaths::EnginePluginsDir(), TEXT("WindowsDualsense_ds5w/Binaries/Win64/ds5w_x64.dll"));
	if (EnginePluginPath.IsEmpty())
	{
		FString LocalPluginPath = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("WindowsDualsense_ds5w/Binaries/Win64/ds5w_x64.dll"));
		DS5WdllHandle = FPlatformProcess::GetDllHandle(*LocalPluginPath);
	}
	else
	{
		DS5WdllHandle = FPlatformProcess::GetDllHandle(*EnginePluginPath);
	}

	RegisterCustomKeys();
	DualSenseLibraryInstance = NewObject<UDualSenseLibrary>();
	DualSenseLibraryInstance->InitializeLibrary();

}

void FWindowsDualsense_ds5wModule::ShutdownModule()
{
	if (DualSenseLibraryInstance)
	{
		DualSenseLibraryInstance->ShutdownLibrary();
		DualSenseLibraryInstance = nullptr;
	}

	if (DS5WdllHandle)
	{
		FPlatformProcess::FreeDllHandle(DS5WdllHandle);
		DS5WdllHandle = nullptr;
	}
}

TSharedPtr<IInputDevice> FWindowsDualsense_ds5wModule::CreateInputDevice(
	const TSharedRef<FGenericApplicationMessageHandler>& InCustomMessageHandler)
{
	DeviceInstances = MakeShareable(new FDualSenseInputDevice(InCustomMessageHandler));

	for (int32 i = 0; i < DualSenseLibraryInstance->ControllersCount; i++)
	{
		RegisterDevice(i);
	}

	return DeviceInstances;
}

void FWindowsDualsense_ds5wModule::RegisterDevice(int32 ControllerId)
{
	FPlatformUserId UserId;
	FInputDeviceId InputDeviceId;
	if (ControllerId > 0)
	{
		UserId = DeviceInstances->AllocateNewUserId();
		InputDeviceId = DeviceInstances->AllocateNewInputDeviceId();
	}
	else
	{
		UserId = FPlatformUserId::CreateFromInternalId(ControllerId);
		InputDeviceId = FInputDeviceId::CreateFromInternalId(ControllerId);
	}


	EInputDeviceConnectionState ConnectionState = EInputDeviceConnectionState::Disconnected;
	if (DualSenseLibraryInstance->IsConnected(ControllerId))
	{
		ConnectionState = EInputDeviceConnectionState::Connected;
	}

	FPlatformInputDeviceState State;
	State.OwningPlatformUser = UserId;
	State.ConnectionState = ConnectionState;
	DeviceInstances->SetController(InputDeviceId, State);

	if (DeviceInstances->RemapUserAndDeviceToControllerId(UserId, ControllerId, InputDeviceId))
	{
		UE_LOG(LogTemp, Log, TEXT("Success mapper Device registred: %d and User %d"), ControllerId, UserId.GetInternalId());
	}
}

void FWindowsDualsense_ds5wModule::RegisterCustomKeys()
{
	const FKey Mic("PS_Mic");
	const FKey Menu("PS_Menu");
	const FKey Shared("PS_Share");
	const FKey TouchButtom("PS_TouchButtom");
	const FKey PlayStationButton("PS_Button");
	const FKey PS_PushLeftStick("PS_PushLeftStick");
	const FKey PS_PushRightStick("PS_PushRightStick");

	// touch
	const FKey Dualsense_Touch1_X("Dualsense_Touch1_X");
	const FKey Dualsense_Touch1_Y("Dualsense_Touch1_Y");
	const FKey Dualsense_Touch2_X("Dualsense_Touch2_X");
	const FKey Dualsense_Touch2_Y("Dualsense_Touch2_Y");


	EKeys::AddKey(FKeyDetails(
		Dualsense_Touch1_X,
		FText::FromString("PS Dualsense Touch1 X"),
		FKeyDetails::GamepadKey
	));

	EKeys::AddKey(FKeyDetails(
		Dualsense_Touch1_Y,
		FText::FromString("PS Dualsense Touch1 Y"),
		FKeyDetails::GamepadKey
	));

	EKeys::AddKey(FKeyDetails(
		Dualsense_Touch2_X,
		FText::FromString("PS Dualsense Touch2 X"),
		FKeyDetails::GamepadKey
	));

	EKeys::AddKey(FKeyDetails(
		Dualsense_Touch2_Y,
		FText::FromString("PS Dualsense Touch2 Y"),
		FKeyDetails::GamepadKey
	));

	EKeys::AddKey(FKeyDetails(
		PS_PushLeftStick,
		FText::FromString("PS Push Left Stick"),
		FKeyDetails::GamepadKey
	));

	EKeys::AddKey(FKeyDetails(
		PS_PushRightStick,
		FText::FromString("PS Push Right Stick"),
		FKeyDetails::GamepadKey
	));

	EKeys::AddKey(FKeyDetails(
		Shared,
		FText::FromString("PlayStation Share"),
		FKeyDetails::GamepadKey
	));

	EKeys::AddKey(FKeyDetails(
		Menu,
		FText::FromString("PlayStation Menu"),
		FKeyDetails::GamepadKey
	));

	EKeys::AddKey(FKeyDetails(
		PlayStationButton,
		FText::FromString("PlayStation Button"),
		FKeyDetails::GamepadKey
	));

	EKeys::AddKey(FKeyDetails(
		Mic,
		FText::FromString("PlayStation Mic"),
		FKeyDetails::GamepadKey
	));

	EKeys::AddKey(FKeyDetails(
		TouchButtom,
		FText::FromString("PlayStation Touch Buttom"),
		FKeyDetails::GamepadKey
	));
}

IMPLEMENT_MODULE(FWindowsDualsense_ds5wModule, WindowsDualsense_ds5w)
