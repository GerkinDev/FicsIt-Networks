﻿#pragma once

#include "MovieSceneVectorSection.h"
#include "Computer/FINComputerModule.h"
#include "FicsItNetworks/Graphics/FINGraphicsProcessor.h"

#include "FINComputerGPU.generated.h"

DECLARE_DELEGATE_RetVal_ThreeParams(FReply, FScreenCursorEventHandler, int, int, int);

class SScreenMonitor : public SLeafWidget {
	SLATE_BEGIN_ARGS(SScreenMonitor) : _Text(),
		_Font(),
		_ScreenSize()
		{
			_Clipping = EWidgetClipping::OnDemand;
		}
		SLATE_ATTRIBUTE(TArray<FString>, Text)
		SLATE_ATTRIBUTE(FSlateFontInfo, Font)
		SLATE_ATTRIBUTE(FVector2D, ScreenSize)
		SLATE_ATTRIBUTE(TArray<TArray<FLinearColor>>, Foreground)
		SLATE_ATTRIBUTE(TArray<TArray<FLinearColor>>, Background)

		SLATE_EVENT(FScreenCursorEventHandler, OnMouseDown)
		SLATE_EVENT(FScreenCursorEventHandler, OnMouseUp)
		SLATE_EVENT(FScreenCursorEventHandler, OnMouseMove)
	SLATE_END_ARGS()

public:
    void Construct( const FArguments& InArgs );

	/**
	 * Returns the currently displayed text grid.
	 *
	 * @return	the currently displayed text grid
	 */
	TArray<FString> GetText() const;

	/**
	 * Allows you to get information about the character screen size.
	 *
	 * @return	the screen size
	 */
	FVector2D GetScreenSize() const;

	/**
	 * Allows you to set the screen size of the display.
	 *
	 * @param[in]	ScreenSize	the new screen size for the display
	 */
	void SetScreenSize(FVector2D ScreenSize);

	/**
	 * This function returns the size of a single displayed character slot in the widgets local space
	 *
	 * @return	the character slot size in local space
	 */
	FVector2D GetCharSize() const;

	/**
	 * This function converts a local space coordinate to a character grid coordinate.
	 *
	 * @parm[in]	the local space coordinate you want to convert
	 * @return	the character coordinate
	 */
	FVector2D LocalToCharPos(FVector2D Pos) const;

	/**
	 * This function converts a mouse events button states to a int which is actually a bit field
	 * for each of the button states.
	 * The states from least significant bit to most:
	 * - Left Mouse Button down
	 * - Right Mouse Button down
	 * - control key down
	 * - shift key down
	 * - command key down
	 *
	 * @return	the integer holding the bit field
	 */
	static int MouseToInt(const FPointerEvent& MouseEvent);
	
private:
    TAttribute<TArray<FString>> Text;
	TAttribute<TArray<TArray<FLinearColor>>> Foreground;
	TAttribute<TArray<TArray<FLinearColor>>> Background;
	TAttribute<FSlateFontInfo> Font;
	TAttribute<FVector2D> ScreenSize;
	FScreenCursorEventHandler OnMouseDownEvent;
	FScreenCursorEventHandler OnMouseUpEvent;
	FScreenCursorEventHandler OnMouseMoveEvent;

	int lastMoveX = -1;
	int lastMoveY = -1;
    
public:
	SScreenMonitor();

	// Begin SWidget
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	// End SWidget
};

UCLASS()
class AFINComputerGraphicsProcessor : public AFINComputerModule, public IFINGraphicsProcessor {
	GENERATED_BODY()
private:
	UPROPERTY()
	UObject* Screen = nullptr;

	TArray<FString> TextGrid;
	TArray<TArray<FLinearColor>> Foreground;
	TArray<TArray<FLinearColor>> Background;
	FLinearColor CurrentForeground = FLinearColor(1,1,1,1);
	FLinearColor CurrentBackground = FLinearColor(0,0,0,0);
	FVector2D ScreenSize;

	TSharedPtr<SScaleBox> Widget;

	UPROPERTY()
	FSlateBrush boxBrush;

	bool shouldCreate = false;
	
public:
	AFINComputerGraphicsProcessor();

	// Begin AActor
	virtual void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	virtual void BeginPlay() override;
	// End AActor

	// Begin IFGSaveInterface
    virtual bool ShouldSave_Implementation() const override;
	// End IFGSaveInterface
	
	// Begin IFINGraphicsProcessor
	virtual void BindScreen(UObject* screen) override;
	virtual UObject* GetScreen() const override;
    virtual void RequestNewWidget() override;
    virtual void DropWidget() override;
	// End IFINGraphicsProcessor

	/**
	 * Creates a new widget for use in the screen.
	 * Does *not* tell the screen to use it.
	 * Recreates the widget if it exists already.
	 * Shoul get called by the client only
	 */
	void CreateWidget();

	/**
	 * Reallocates the TextGrid for the new given screen size.
	 *
	 * @param[in]	size	the new screen size
	 */
	void SetScreenSize(FVector2D size);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void netSig_OnMouseDown(int x, int y, int btn);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void netSig_OnMouseUp(int x, int y, int btn);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void netSig_OnMouseMove(int x, int y, int btn);

	UFUNCTION()
	void netFunc_bindScreen(UObject* NewScreen);

	UFUNCTION()
	UObject* netFunc_getScreen();

	UFUNCTION()
	void netFunc_setText(int x, int y, const FString& str);

	UFUNCTION()
	void netFunc_fill(int x, int y, int dx, int dy, const FString& str);

	UFUNCTION()
	void netFunc_getSize(int& w, int& h);

	UFUNCTION()
	void netFunc_setSize(int w, int h);

	UFUNCTION()
	void netFunc_setForeground(float r, float g, float b, float a);

	UFUNCTION()
	void netFunc_setBackground(float r, float g, float b, float a);
};
