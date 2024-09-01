﻿#pragma once

#include "Reflection/FINFunction.h"
#include "FicsItNetworksModule.h"
#include "FINFuture.generated.h"

USTRUCT(BlueprintType)
struct FICSITNETWORKS_API FFINFuture {
	GENERATED_BODY()

	virtual ~FFINFuture() = default;

	/**
	 * This function will get called from the FicsIt-Kernel in the main thread if it is added to the
	 * future queue of the kernel.
	 */
	virtual void Execute() {}
	
	/**
	 * Checks if the future has finished and we can get values from it.
	 */
	virtual bool IsDone() const { return false; }

	/**
	 * Returns the output data of the future
	 */
	virtual TArray<FFINAnyNetworkValue> GetOutput() const { return {}; }
};

USTRUCT()
struct FICSITNETWORKS_API FFINFutureReflection : public FFINFuture {
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	bool bDone = false;

	UPROPERTY(SaveGame)
	TArray<FFINAnyNetworkValue> Input;

	UPROPERTY(SaveGame)
	TArray<FFINAnyNetworkValue> Output;

	UPROPERTY(SaveGame)
	FFIRExecutionContext Context;

	UPROPERTY(SaveGame)
	UFINFunction* Function = nullptr;

	UPROPERTY(SaveGame)
	UFIRProperty* Property = nullptr;

	static FCriticalSection Mutex;

	// TODO: Maybe do a LogScope snapshot?
	FFINFutureReflection() = default;
	FFINFutureReflection(UFINFunction* Function, const FFIRExecutionContext& Context, const TArray<FFINAnyNetworkValue>& Input) : Input(Input), Context(Context), Function(Function) {}
	FFINFutureReflection(UFIRProperty* Property, const FFIRExecutionContext& Context, const FFINAnyNetworkValue& Input) : Input({Input}), Context(Context), Property(Property) {}
	FFINFutureReflection(UFIRProperty* Property, const FFIRExecutionContext& Context) : Context(Context), Property(Property) {}

	virtual bool IsDone() const override {
		FScopeLock Lock(const_cast<FCriticalSection*>(&Mutex));
		return bDone;
	}

	virtual void Execute() override {
		FScopeLock Lock(&Mutex);

		if (!Context.IsValid()) {
			throw FFINException(TEXT("Execution context of future is invalid."));
		}
		
		if (Function) {
			Output = Function->Execute(Context, Input);
			bDone = true;
		} else if (Property) {
			if (Input.Num() > 0) {
				Property->SetValue(Context, Input[0]);
			} else {
				Output.Add(Property->GetValue(Context));
				bDone = true;
			}
		} else {
			UE_LOG(LogFicsItNetworks, Error, TEXT("Future unable to get executed due to invalid function/property pointer!"));
		}
	}

	virtual TArray<FFINAnyNetworkValue> GetOutput() const override {
		FScopeLock Lock(const_cast<FCriticalSection*>(&Mutex));
		return Output;
	}
};

USTRUCT()
struct FICSITNETWORKS_API FFINFunctionFuture : public FFINFuture {
	GENERATED_BODY()

	TFunction<void()> Func;
	bool bDone = false;

	FFINFunctionFuture() = default;
	FFINFunctionFuture(TFunction<void()> Func) : Func(Func) {}

	virtual void Execute() override {
		bDone = true;
		Func();
	}

	virtual bool IsDone() const override { return bDone; }
};
