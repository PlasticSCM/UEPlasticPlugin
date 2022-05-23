// Copyright (c) 2016-2022 Codice Software

#include "PlasticSourceControlMenuCommands.h"

#define LOCTEXT_NAMESPACE "FPlasticSourceControlModule"

void FPlasticSourceControlMenuCommands::RegisterCommands()
{
	UI_COMMAND(OpenTab, "PlasticSourceControl", "Open Plastic SCM window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
