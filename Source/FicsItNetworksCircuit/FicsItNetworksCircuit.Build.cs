﻿using UnrealBuildTool;

public class FicsItNetworksCircuit : ModuleRules
{
    public FicsItNetworksCircuit(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableExceptions = true;
        bWarningsAsErrors = true;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "FactoryGame",
                "SML",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "FicsItReflection",
            }
        );
    }
}