﻿#include "Script/Library/FIVSNode_SeparateStruct.h"

#include "Reflection/FINReflection.h"

void UFIVSNode_SeparateStruct::GetNodeActions(TArray<FFIVSNodeAction>& Actions) const {
	for (TTuple<UScriptStruct*, UFINStruct*> StructPair : FFINReflection::Get()->GetStructs()) {
		FFIVSNodeAction BreakAction;
		BreakAction.NodeType = UFIVSNode_SeparateStruct::StaticClass();
		BreakAction.Category = FText::FromString(TEXT("Struct"));
		BreakAction.Title = FText::FromString(TEXT("Break ") + StructPair.Value->GetDisplayName().ToString());
		for (UFINProperty* Prop : StructPair.Value->GetProperties()) {
			FFIVSFullPinType FullPinType(Prop);
			FullPinType.PinType = FIVS_PIN_DATA_OUTPUT;
			BreakAction.Pins.Add(FullPinType);
		}
		BreakAction.SearchableText = BreakAction.Title;
		BreakAction.OnExecute.BindLambda([StructPair](UFIVSNode* Node) {
			Cast<UFIVSNode_SeparateStruct>(Node)->bBreak = true;
			Cast<UFIVSNode_SeparateStruct>(Node)->Struct = StructPair.Value;
		});
		FFIVSNodeAction MakeAction(BreakAction);
		
		BreakAction.Pins.Add(FFIVSFullPinType(FIVS_PIN_DATA_INPUT, FFIVSPinDataType(FIN_STRUCT, StructPair.Value)));
		Actions.Add(BreakAction);

		for (FFIVSFullPinType& Pin : MakeAction.Pins) Pin.PinType = FIVS_PIN_DATA_INPUT;
		MakeAction.Title = FText::FromString(TEXT("Make ") + StructPair.Value->GetDisplayName().ToString());
		MakeAction.Pins.Add(FFIVSFullPinType(FIVS_PIN_DATA_OUTPUT, FFIVSPinDataType(FIN_STRUCT, StructPair.Value)));
		MakeAction.SearchableText = MakeAction.Title;
		MakeAction.OnExecute.BindLambda([StructPair](UFIVSNode* Node) {
			Cast<UFIVSNode_SeparateStruct>(Node)->bBreak = false;
			Cast<UFIVSNode_SeparateStruct>(Node)->Struct = StructPair.Value;
		});
		Actions.Add(MakeAction);
	}
}

void UFIVSNode_SeparateStruct::SerializeNodeProperties(FFIVSNodeProperties& Properties) const {
	Properties.Properties.Add("Struct", Struct->GetPathName());
	Properties.Properties.Add("What", bBreak ? TEXT("break") : TEXT("split"));
}

void UFIVSNode_SeparateStruct::DeserializeNodeProperties(const FFIVSNodeProperties& Properties) {
	Struct = Cast<UFINStruct>(FSoftObjectPath(Properties.Properties[TEXT("Struct")]).TryLoad());
	bBreak = Properties.Properties[TEXT("What")] == TEXT("break");
}

TArray<UFIVSPin*> UFIVSNode_SeparateStruct::PreExecPin(UFIVSPin* ExecPin, FFIVSRuntimeContext& Context) {
	TArray<UFIVSPin*> ValidatePins;
	InputPins.GenerateValueArray(ValidatePins);
	return ValidatePins;
}

TArray<UFIVSPin*> UFIVSNode_SeparateStruct::ExecPin(UFIVSPin* ExecPin, FFIVSRuntimeContext& Context) {
	if (bBreak) {
		FFINDynamicStructHolder StructObj = Context.GetValue(InputPins[TEXT("Struct")])->GetStruct();
		FFINExecutionContext Ctx(StructObj.GetData());
		for (UFINProperty* Prop : Struct->GetProperties()) {
			if (!(Prop->GetPropertyFlags() & FIN_Prop_Attrib)) continue;
			UFIVSPin** Pin = OutputPins.Find(Prop->GetInternalName());
			if (Pin) Context.SetValue(*Pin, Prop->GetValue(Ctx));
		}
	} else {
		FFINDynamicStructHolder StructObj(Cast<UScriptStruct>(Struct->GetOuter()));
		FFINExecutionContext Ctx(StructObj.GetData());
		for (UFINProperty* Prop : Struct->GetProperties()) {
			if (!(Prop->GetPropertyFlags() & FIN_Prop_Attrib)) continue;
			UFIVSPin** Pin = InputPins.Find(Prop->GetInternalName());
			if (Pin) Prop->SetValue(Ctx, *Context.GetValue(*Pin));
		}
		Context.SetValue(OutputPins[TEXT("Struct")], StructObj);
	}
	return {};
}

void UFIVSNode_SeparateStruct::SetStruct(UFINStruct* InStruct) {
	Struct = InStruct;

	if (bBreak) DisplayName = FText::FromString(TEXT("Break ") + Struct->GetDisplayName().ToString());
	else DisplayName = FText::FromString(TEXT("Make ") + Struct->GetDisplayName().ToString());

	for (auto [_, pin] : InputPins) DeletePin(pin);
	for (auto [_, pin] : OutputPins) DeletePin(pin);

	EFIVSPinType PinType = bBreak ? FIVS_PIN_DATA_OUTPUT : FIVS_PIN_DATA_INPUT;
	for (UFINProperty* Prop : Struct->GetProperties()) {
		UFIVSPin* Pin = CreatePin(PinType, Prop->GetInternalName(), Prop->GetDisplayName(), FFIVSPinDataType(Prop));
		(bBreak ? OutputPins : InputPins).Add(Prop->GetInternalName(), Pin);
	}
	UFIVSPin* Pin = CreatePin(bBreak ? FIVS_PIN_DATA_INPUT : FIVS_PIN_DATA_OUTPUT, TEXT("Struct"), FText::FromString(TEXT("Struct")), FFIVSPinDataType(FIN_STRUCT, Struct));
	(bBreak ? InputPins : OutputPins).Add(TEXT("Struct"), Pin);
}
