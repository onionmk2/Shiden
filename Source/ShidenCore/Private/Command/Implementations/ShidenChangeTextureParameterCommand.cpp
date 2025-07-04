// Copyright (c) 2025 HANON. All Rights Reserved.

#include "Command/Implementations/ShidenChangeTextureParameterCommand.h"
#include "System/ShidenBlueprintLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Scenario/ShidenScenarioBlueprintLibrary.h"

void UShidenChangeTextureParameterCommand::ParseFromCommand(const FShidenCommand& Command, FChangeTextureParameterCommandArgs& Args)
{
	Args.Target = Command.GetArg("Target");
	Args.TargetName = Command.GetArg("TargetName");
	Args.ParameterName = Command.GetArg("ParameterName");
	Args.TexturePath = Command.GetArg("Texture");
}

void UShidenChangeTextureParameterCommand::RestoreFromSaveData_Implementation(const TMap<FString, FShidenScenarioProperty>& ScenarioProperties,
                                                                              UShidenWidget* ShidenWidget,
                                                                              const TScriptInterface<IShidenManagerInterface>& ShidenManager,
                                                                              UObject* CallerObject, EShidenInitFromSaveDataStatus& Status,
                                                                              FString& ErrorMessage)
{
	for (const TPair<FString, FShidenScenarioProperty>& Pair : ScenarioProperties)
	{
		FString TargetType, TargetName, ParameterName;
		Tie(TargetType, TargetName, ParameterName) = ParseScenarioPropertyKey(Pair.Key);

		Args = FChangeTextureParameterCommandArgs
		{
			.Target = TargetType,
			.TargetName = TargetName,
			.ParameterName = ParameterName,
			.TexturePath = Pair.Value.GetValueAsString()
		};

		UTexture* Texture;
		if (Args.TexturePath.IsEmpty() || Args.TexturePath == TEXT("None"))
		{
			Texture = ClearTexture;
		}
		else
		{
			if (!TryLoadTexture(Args, Texture, ErrorMessage))
			{
				Status = EShidenInitFromSaveDataStatus::Error;
				return;
			}
		}

		if (!TryChangeTextureParameter(Args, ShidenWidget, Texture, ErrorMessage))
		{
			Status = EShidenInitFromSaveDataStatus::Error;
			return;
		}
	}

	Status = EShidenInitFromSaveDataStatus::Complete;
}

void UShidenChangeTextureParameterCommand::ProcessCommand_Implementation(const FString& ProcessName,
                                                                         const FShidenCommand& Command, UShidenWidget* ShidenWidget,
                                                                         const TScriptInterface<IShidenManagerInterface>& ShidenManager,
                                                                         const float DeltaTime, UObject* CallerObject, EShidenProcessStatus& Status,
                                                                         FString& BreakReason, FString& NextScenarioName, FString& ErrorMessage)
{
	ParseFromCommand(Command, Args);

	UTexture* Texture;
	if (Args.TexturePath.IsEmpty() || Args.TexturePath == TEXT("None"))
	{
		Texture = ClearTexture;
	}
	else
	{
		if (!TryLoadTexture(Args, Texture, ErrorMessage))
		{
			Status = EShidenProcessStatus::Error;
			return;
		}
	}

	if (!TryChangeTextureParameter(Args, ShidenWidget, Texture, ErrorMessage))
	{
		Status = EShidenProcessStatus::Error;
		return;
	}

	const FString Key = MakeScenarioPropertyKey(Args.Target, Args.TargetName, Args.ParameterName);
	UShidenScenarioBlueprintLibrary::RegisterScenarioProperty(Command.CommandName, Key, Args.TexturePath);
	Status = EShidenProcessStatus::Next;
}

void UShidenChangeTextureParameterCommand::PreviewCommand_Implementation(const FShidenCommand& Command, UShidenWidget* ShidenWidget,
                                                                         const TScriptInterface<IShidenManagerInterface>& ShidenManager,
                                                                         bool bIsCurrentCommand, EShidenPreviewStatus& Status, FString& ErrorMessage)
{
	ParseFromCommand(Command, Args);

	UTexture* Texture;
	if (Args.TexturePath.IsEmpty() || Args.TexturePath == TEXT("None"))
	{
		Texture = ClearTexture;
	}
	else
	{
		if (!TryLoadTexture(Args, Texture, ErrorMessage))
		{
			Status = EShidenPreviewStatus::Error;
			return;
		}
	}

	Status = TryChangeTextureParameter(Args, ShidenWidget, Texture, ErrorMessage)
		         ? EShidenPreviewStatus::Complete
		         : EShidenPreviewStatus::Error;
}

bool UShidenChangeTextureParameterCommand::TryLoadTexture(const FChangeTextureParameterCommandArgs& Args, UTexture*& Texture, FString& ErrorMessage)
{
	UObject* Object;
	if (!UShidenBlueprintLibrary::TryGetOrLoadAsset(Args.TexturePath, Object))
	{
		ErrorMessage = FString::Printf(TEXT("Failed to change texture parameter. Failed to load texture asset %s."), *Args.TexturePath);
		return false;
	}

	Texture = Cast<UTexture>(Object);
	if (!Texture)
	{
		ErrorMessage = FString::Printf(TEXT("Failed to change texture parameter. Asset %s is not a texture."), *Args.TexturePath);
		return false;
	}

	return true;
}

bool UShidenChangeTextureParameterCommand::TryChangeTextureParameter(const FChangeTextureParameterCommandArgs& Args,
                                                                     const UShidenWidget* ShidenWidget,
                                                                     UTexture* Texture, FString& ErrorMessage)
{
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	if (Args.Target == TEXT("Image"))
	{
		UImage* Image;
		if (!ShidenWidget->TryFindImage(Args.TargetName, Image))
		{
			ErrorMessage = FString::Printf(TEXT("Failed to change texture parameter. Target %s is not found."), *Args.TargetName);
			return false;
		}
		DynamicMaterial = Image->GetDynamicMaterial();
	}
	else if (Args.Target == TEXT("RetainerBox"))
	{
		URetainerBox* RetainerBox;
		if (!ShidenWidget->TryFindRetainerBox(Args.TargetName, RetainerBox))
		{
			ErrorMessage = FString::Printf(TEXT("Failed to change texture parameter. Target %s is not found."), *Args.TargetName);
			return false;
		}
		const TObjectPtr<UObject> Resource = IsValid(RetainerBox) ? RetainerBox->GetEffectMaterial() : nullptr;
		DynamicMaterial = Cast<UMaterialInstanceDynamic>(Resource);
	}
	else
	{
		ErrorMessage = FString::Printf(TEXT("Failed to change texture parameter. Target %s is not supported."), *Args.Target);
		return false;
	}

	if (!DynamicMaterial)
	{
		ErrorMessage = FString::Printf(TEXT("Failed to change texture parameter. Target %s is not a dynamic material."), *Args.TargetName);
		return false;
	}

	DynamicMaterial->SetTextureParameterValue(FName(Args.ParameterName), Texture);
	return true;
}

FString UShidenChangeTextureParameterCommand::MakeScenarioPropertyKey(const FString& TargetType,
                                                                      const FString& TargetName, const FString& ParameterName)
{
	return FString::Printf(TEXT("%s::%s::%s"),
	                       *TargetType.Replace(TEXT(":"), TEXT("\\:")),
	                       *TargetName.Replace(TEXT(":"), TEXT("\\:")),
	                       *ParameterName.Replace(TEXT(":"), TEXT("\\:")));
}

TTuple<FString, FString, FString> UShidenChangeTextureParameterCommand::ParseScenarioPropertyKey(const FString& Key)
{
	TArray<FString> TempArray;
	Key.ParseIntoArray(TempArray, TEXT("::"), true);
	if (TempArray.Num() != 3)
	{
		return TTuple<FString, FString, FString>(TEXT(""), TEXT(""), TEXT(""));
	}

	const FString TargetType = TempArray[0].Replace(TEXT("\\:"), TEXT(":"));
	const FString TargetName = TempArray[1].Replace(TEXT("\\:"), TEXT(":"));
	const FString ParameterName = TempArray[2].Replace(TEXT("\\:"), TEXT(":"));
	return TTuple<FString, FString, FString>(TargetType, TargetName, ParameterName);
}
