#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MPTemplateGameInstance.generated.h"


UCLASS()
class UE5MPTEMPLATE_API UMPTemplateGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxPlayers = 1;
};
