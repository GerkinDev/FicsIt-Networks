﻿#pragma once

#include "Network/FINAnyNetworkValue.h"
#include "FINSignalData.generated.h"

class UFIRSignal;

USTRUCT()
struct FICSITNETWORKS_API FFINSignalData {
	GENERATED_BODY()

	UPROPERTY()
	UFIRSignal* Signal = nullptr;

	UPROPERTY()
	TArray<FFINAnyNetworkValue> Data;

	FFINSignalData() = default;
	FFINSignalData(UFIRSignal* Signal, const FINArray& Data) : Signal(Signal), Data(Data) {}

	bool Serialize(FStructuredArchive::FSlot Slot);
};

inline FArchive& operator<<(FArchive& Ar, FFINSignalData& Data) {
	Data.Serialize(FStructuredArchiveFromArchive(Ar).GetSlot());
	return Ar;
}

inline void operator<<(FStructuredArchive::FSlot Slot, FFINSignalData& Data) {
	Data.Serialize(Slot);
}

template<>
struct TStructOpsTypeTraits<FFINSignalData> : TStructOpsTypeTraitsBase2<FFINSignalData> {
	enum {
		//WithSerializer = true,
		WithStructuredSerializer = true,
	};
};