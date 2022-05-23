// Copyright (c) 2016-2022 Codice Software

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "PlasticSourceControlStyle.h"

class FPlasticSourceControlMenuCommands : public TCommands<FPlasticSourceControlMenuCommands>
{
public:

	FPlasticSourceControlMenuCommands()
		: TCommands<FPlasticSourceControlMenuCommands>(TEXT("PlasticSourceControl"), NSLOCTEXT("Contexts", "PlasticSourceControl", "PlasticSourceControl Plugin"), NAME_None, FPlasticSourceControlStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> OpenTab;
};