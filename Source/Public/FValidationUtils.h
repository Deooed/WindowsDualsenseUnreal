// Copyright (c) 2025 Rafael Valotor/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class WINDOWSDUALSENSE_DS5W_API FValidationUtils
{
public:
	FValidationUtils();
	~FValidationUtils();

	static bool ValidateMaxForce(int32 Force)
	{
		return Force <= 9 && Force >= 0;
	}

	static bool ValidateMaxPosition(int32 Position)
	{
		return Position <= 8 && Position >= 0;
	}

	static bool ValidateMaxFrequency(float Frequency)
	{
		return Frequency <= 1.0 && Frequency >= 0.0;
	}
};
