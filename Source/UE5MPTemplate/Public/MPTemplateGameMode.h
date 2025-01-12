
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MPTemplateGameMode.generated.h"

UCLASS()
class UE5MPTEMPLATE_API AMPTemplateGameMode : public AGameModeBase
{
	GENERATED_BODY()
protected:
	virtual void BeginPlay() override;

public:
	void SpawnPlayers();	
};
