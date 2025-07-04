// Copyright (c) 2025 HANON. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Command/ShidenCommandDefinitions.h"
#include "SGraphPinNameList.h"
#include "UObject/SoftObjectPtr.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UShidenCommandDefinitions;
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
class UEdGraphPin;

class SHIDENK2NODE_API SGraphPinShidenCommandName final : public SGraphPinNameList
{
public:
	SLATE_BEGIN_ARGS(SGraphPinShidenCommandName)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& _, UEdGraphPin* InGraphPinObj, const TObjectPtr<UShidenCommandDefinitions> InCommandDefinitions);

	SGraphPinShidenCommandName();
	virtual ~SGraphPinShidenCommandName() override;

protected:
	void RefreshNameList(const TObjectPtr<UShidenCommandDefinitions>& CommandDefinitions);
};
