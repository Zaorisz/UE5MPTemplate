#include "MPTemplateGameMode.h"

#include "MPTemplateGameInstance.h"
#include "Kismet/GameplayStatics.h"

void AMPTemplateGameMode::BeginPlay()
{
	Super::BeginPlay();
	SpawnPlayers();
}

void AMPTemplateGameMode::SpawnPlayers()
{
	UMPTemplateGameInstance* GameInstance = Cast<UMPTemplateGameInstance>(GetGameInstance());
	for (int i = 0; i <= GameInstance->MaxPlayers - 1; i++)
	{
		if (i == 0)
			continue;
		APlayerController* NewPlayerController = UGameplayStatics::CreatePlayer(this, -1, true);
		FTransform SpawnTransform = FindPlayerStart(NewPlayerController, FString::FromInt(i))->GetTransform();
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = NewPlayerController;
		APawn* NewPawn = GetWorld()->SpawnActor<APawn>(DefaultPawnClass->GetClass(), SpawnTransform, SpawnParameters);
		NewPlayerController->Possess(NewPawn);
	}
	UE_LOG(LogTemp, Display, TEXT("SpawnedPlayers: %d"), GameInstance->MaxPlayers);
}
