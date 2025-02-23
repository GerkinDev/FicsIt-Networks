﻿#include "Wireless/FINWirelessAccessPointActorRepresentation.h"
#include "Wireless/FINWirelessAccessPointConnection.h"

UFINWirelessAccessPointActorRepresentation::UFINWirelessAccessPointActorRepresentation() {}

void UFINWirelessAccessPointActorRepresentation::Setup(UFINWirelessAccessPointConnection* Connection) {
	this->mIsLocal = true;
	this->mShouldShowInCompass = false;
	this->mShouldShowOnMap = true;
	this->mRepresentationText =  Connection->GetRepresentationText();
	this->mRepresentationColor = Connection->Data.IsConnected && Connection->Data.IsInRange ? Green : Connection->Data.IsInRange ? Orange : Red;
	this->mRepresentationTexture = LoadObject<UTexture2D>(NULL, TEXT("/FicsItNetworks/Buildings/Network/WirelessAccessPoint/UI/Assets/TXUI_FIN_Wifi_MapCompassIcon.TXUI_FIN_Wifi_MapCompassIcon"));
	this->mRepresentationCompassMaterial = LoadObject<UMaterialInterface>(NULL, TEXT("/FicsItNetworks/Buildings/Network/WirelessAccessPoint/UI/Assets/MI_CompassIcon_Wifi.MI_CompassIcon_Wifi"));
	//this->mRealActor = Connection->RadarTower.Get();
	this->mIsStatic = true;
	this->mActorRotation = FRotator::ZeroRotator;
	this->mActorLocation = Connection->GetRepresentationLocation();
	this->mRealActorLocation = Connection->GetRepresentationLocation();

	// We use RT_StartingPod since RT_Default destroys automatically the ActorRepresentation after 10 seconds
	// Cannot use RT_MapMarker since it opens the marker editor when clicked.
	this->mRepresentationType = ERepresentationType::RT_MapMarker;
}

void UFINWirelessAccessPointActorRepresentation::SetupActorRepresentation(AActor* realActor, bool isLocal, float lifeSpan) {}

FVector UFINWirelessAccessPointActorRepresentation::GetActorLocation() const {
	return this->mRealActorLocation;
}

void UFINWirelessAccessPointActorRepresentation::TrySetupDestroyTimer(float lifeSpan) {
}

bool UFINWirelessAccessPointActorRepresentation::CanBeHighlighted() const {
	return true;
}

