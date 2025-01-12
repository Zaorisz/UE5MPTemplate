#include "MultiplayerSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"

DEFINE_LOG_CATEGORY_STATIC(MultiplayerSessionSubsystem, Log, All)

UMultiplayerSessionSubsystem::UMultiplayerSessionSubsystem()
{
	UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("UMultiplayerSessionSubsystem()"));
	CreateServerAfterDestroy = false;
	DestroyServerName = "";
	ServerNameToFind = "";
	MySessionName = FName("MultiplayerTemplate");
}

void UMultiplayerSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("UMultiplayerSessionSubsystem::Initialize()"));
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		FString SubsystemName = OnlineSubsystem->GetSubsystemName().ToString();
		UE_LOG(MultiplayerSessionSubsystem, Log, TEXT(" Online Subsystem: %s "), *SubsystemName);

		SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(
				this, &UMultiplayerSessionSubsystem::OnCreateSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(
				this, &UMultiplayerSessionSubsystem::OnDestroySessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(
				this, &UMultiplayerSessionSubsystem::OnFindSessionsComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(
				this, &UMultiplayerSessionSubsystem::OnJoinSessionComplete);
		}
	}
}

void UMultiplayerSessionSubsystem::Deinitialize()
{
	UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("UMultiplayerSessionSubsystem::Deinitialize()"));
	CreateServerAfterDestroy = false;
	SessionInterface->EndSession(MySessionName);
	SessionInterface->DestroySession(MySessionName);
}

void UMultiplayerSessionSubsystem::CreateServer(FString ServerName)
{
	UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("UMultiplayerSessionSubsystem::CreateServer"));
	if (ServerName.IsEmpty())
	{
		UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("Server name cannot be empty!"));
		ServerCreateDelegate.Broadcast(false);
		return;
	}

	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(MySessionName);
	if (ExistingSession)
	{
		UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("Session with name %s already exists, destroying it."),
		       *MySessionName.ToString());
		CreateServerAfterDestroy = true;
		DestroyServerName = ServerName;
		SessionInterface->DestroySession(MySessionName);
		return;
	}

	FOnlineSessionSettings SessionSettings;

	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bIsDedicated = false;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.NumPublicConnections = 2;
	/* Unreal Engine 5.5 treats bUsesPresence and bUseLobbiesIfAvailable as equivalent,
	 * and they must have matching values to avoid errors when joining sessions */
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bAllowInvites = true;

	bool IsLAN = false;
	if (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL")
	{
		IsLAN = true;
	}
	SessionSettings.bIsLANMatch = IsLAN;

	SessionSettings.Set(FName("SERVER_NAME"), ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	SessionInterface->CreateSession(0, MySessionName, SessionSettings);
	UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("Session created"));
}

void UMultiplayerSessionSubsystem::FindServer(FString ServerName)
{
	UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("UMultiplayerSessionSubsystem::FindServer"));
	if (ServerName.IsEmpty())
	{
		UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("Server name cannot be empty!"));
		ServerJoinDelegate.Broadcast(false);
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	bool IsLAN = false;
	if (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL")
	{
		IsLAN = true;
	}
	SessionSearch->bIsLanQuery = IsLAN;
	SessionSearch->MaxSearchResults = 9999;
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	ServerNameToFind = ServerName;

	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

void UMultiplayerSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool WasSuccessful)
{
	UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("UMultiplayerSessionSubsystem::OnCreateSessionComplete"));
	UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("Creating was successful %d"), WasSuccessful);
	ServerCreateDelegate.Broadcast(WasSuccessful);

	if (WasSuccessful)
	{
		FString Path = FString::Printf(TEXT("%s?listen"), *GameMapPath);
		GetWorld()->ServerTravel(Path);
	}
}

void UMultiplayerSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool WasSuccessful)
{
	UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("UMultiplayerSessionSubsystem::OnDestroySessionComplete"));
	UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("Destroying SessionName: %s, Success: %d"),
	       *SessionName.ToString(), WasSuccessful);

	if (CreateServerAfterDestroy)
	{
		CreateServerAfterDestroy = false;
		CreateServer(DestroyServerName);
	}
}

void UMultiplayerSessionSubsystem::OnFindSessionsComplete(bool WasSuccessful)
{
	UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("UMultiplayerSessionSubsystem::OnFindSessionsComplete"));
	if (!WasSuccessful) return;
	if (ServerNameToFind.IsEmpty()) return;

	TArray<FOnlineSessionSearchResult> Results = SessionSearch->SearchResults;

	if (Results.Num() > 0)
	{
		UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("%d sessions found."), Results.Num());
		FOnlineSessionSearchResult* CorrectResult = 0;
		for (FOnlineSessionSearchResult Result : Results)
		{
			if (Result.IsValid())
			{
				FString ServerName = "";
				Result.Session.SessionSettings.Get(FName("SERVER_NAME"), ServerName);

				if (ServerName.Equals(ServerNameToFind))
				{
					CorrectResult = &Result;
					UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("Found server with name: %s"), *ServerName);
					break;
				}
			}
		}

		if (CorrectResult)
		{
			UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("Joining session"));
			// internet says these 4 lines are needed to fix problems with joining
			FOnlineSessionSearchResult& ModifiableSessionResult = const_cast<FOnlineSessionSearchResult&>(*
				CorrectResult);
			FOnlineSessionSettings& SessionSettings = ModifiableSessionResult.Session.SessionSettings;
			SessionSettings.bUsesPresence = true;
			SessionSettings.bUseLobbiesIfAvailable = true;

			SessionInterface->JoinSession(0, MySessionName, *CorrectResult);
		}
		else
		{
			UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("Couldn't find server: %s"), *ServerNameToFind);
			ServerNameToFind = "";
			ServerJoinDelegate.Broadcast(false);
		}
	}
	else
	{
		UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("Zero sessions found."));
		ServerJoinDelegate.Broadcast(false);
	}
}

void UMultiplayerSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("UMultiplayerSessionSubsystem::OnJoinSessionComplete"));

	ServerJoinDelegate.Broadcast(Result == EOnJoinSessionCompleteResult::Success);

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("Successfully joined session %s"), *SessionName.ToString());
		FString Address = "";
		bool Success = SessionInterface->GetResolvedConnectString(MySessionName, Address);
		if (Success)
		{
			UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("Address: %s"), *Address);

			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			if (PlayerController)
			{
				UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("Start Client travel"));
				PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
			}
		}
		else
		{
			UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("GetResolvedConnectString returned false!"));
		}
	}
	else
	{
		UE_LOG(MultiplayerSessionSubsystem, Log, TEXT("OnJoinSessionComplete failed"));
	}
}
