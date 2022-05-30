//========= Copyright � 1996-2009, Valve LLC, All rights reserved. ============
//
// Purpose: Class for tracking friends list
//
//=============================================================================

#include "stdafx.h"
#include "Friends.h"
#include "BaseMenu.h"
#include <math.h>
#include <vector>
#include <algorithm>

//-----------------------------------------------------------------------------
// Purpose: Menu that shows your friends
//-----------------------------------------------------------------------------
class CFriendsListMenu : public CBaseMenu<FriendsListMenuItem_t>
{
	static const FriendsListMenuItem_t k_menuItemEmpty;

public:

	//-----------------------------------------------------------------------------
	// Purpose: Constructor
	//-----------------------------------------------------------------------------
	CFriendsListMenu( IGameEngine *pGameEngine ) : CBaseMenu<FriendsListMenuItem_t>( pGameEngine )
	{
	
	}

	//-----------------------------------------------------------------------------
	// Purpose: Creates friends list menu
	//-----------------------------------------------------------------------------
	void Rebuild()
	{
		PushSelectedItem();
		ClearMenuItems();

		// First add pending incoming requests
		AddFriendsByFlag( k_EFriendFlagFriendshipRequested, "Incoming Friend Requests" );

		// Add each Tag group and record the users with tags
		std::vector<CSteamID> vecTaggedSteamIDs;
		int nFriendsGroups = SteamFriends()->GetFriendsGroupCount();
		for ( int iFG = 0; iFG < nFriendsGroups; iFG++ )
		{
			FriendsGroupID_t friendsGroupID = SteamFriends()->GetFriendsGroupIDByIndex( iFG );
			if ( friendsGroupID == k_FriendsGroupID_Invalid )
				continue;

			int nFriendsGroupMemberCount = SteamFriends()->GetFriendsGroupMembersCount( friendsGroupID );
			if ( !nFriendsGroupMemberCount )
				continue;

			const char *pszFriendsGroupName = SteamFriends()->GetFriendsGroupName( friendsGroupID );
			if ( pszFriendsGroupName == NULL )
				pszFriendsGroupName = "";
			AddMenuItem( CFriendsListMenu::MenuItem_t( "", k_menuItemEmpty ) );
			AddMenuItem( CFriendsListMenu::MenuItem_t( pszFriendsGroupName, k_menuItemEmpty ) );

			std::vector<CSteamID> vecSteamIDMembers( nFriendsGroupMemberCount );
			SteamFriends()->GetFriendsGroupMembersList( friendsGroupID, &vecSteamIDMembers[0], nFriendsGroupMemberCount );
			for ( int iMember = 0; iMember < nFriendsGroupMemberCount; iMember++ )
			{
				const CSteamID &steamIDMember = vecSteamIDMembers[iMember];
				AddFriendToMenu( steamIDMember );
				vecTaggedSteamIDs.push_back( steamIDMember );
			}
		}

		// Add the "normal" Friends category, filtering out the ones with tags
		AddFriendsByFlag( k_EFriendFlagImmediate, "Friends", &vecTaggedSteamIDs );

		// Finally add the pending outgoing requests
		AddFriendsByFlag( k_EFriendFlagRequestingFriendship, "Outgoing Friend Requests" );

		PopSelectedItem();
	}

private:

	void AddFriendsByFlag( int iFriendFlag, const char *pszName, std::vector<CSteamID> *pVecIgnoredSteamIDs = NULL )
	{
		int iFriendCount = SteamFriends()->GetFriendCount(iFriendFlag);
		if ( !iFriendCount )
			return;

		AddMenuItem( CFriendsListMenu::MenuItem_t( "", k_menuItemEmpty ) );
 		AddMenuItem( CFriendsListMenu::MenuItem_t( pszName, k_menuItemEmpty ) );

		std::vector<FriendsListMenuItem_t> vecMenuItemFriends;
		vecMenuItemFriends.reserve(iFriendCount);

		for ( int iFriend = 0; iFriend < iFriendCount; iFriend++ )
		{			
			FriendsListMenuItem_t menuItemFriend = { SteamFriends()->GetFriendByIndex( iFriend, iFriendFlag ) };
 			menuItemFriend.m_bIsFriendOnline = SteamFriends()->GetFriendPersonaState(menuItemFriend.m_steamIDFriend) >= k_EPersonaStateOnline;
 			menuItemFriend.m_bIsFriendInGame = SteamFriends()->GetFriendGamePlayed( menuItemFriend.m_steamIDFriend, &menuItemFriend.m_steamFriendGameInfo );
 			
			// This mimicks the Steam client's feature where it only shows
			// untagged friends in the canonical Friends section by default
			if ( pVecIgnoredSteamIDs && ( std::find( pVecIgnoredSteamIDs->begin(), pVecIgnoredSteamIDs->end(), menuItemFriend.m_steamIDFriend) != pVecIgnoredSteamIDs->end() ) )
				continue;
			
			// If the friend is not already in our friends list, add them to the incoming/outgoing friend request menus
			// and if the friend is in already in our friends menu, add the friend to the vector for further processing
			if ( iFriendFlag != k_EFriendFlagImmediate )
			{
				AddFriendToMenu( menuItemFriend.m_steamIDFriend );
			}
			else
			{
				vecMenuItemFriends.push_back( menuItemFriend );
			}
		}
		
		// Add and sort friends by name, status and presence
		AddFriendsToMenuByStatus( &vecMenuItemFriends );
	}

	void AddFriendsToMenuByStatus( std::vector<FriendsListMenuItem_t> *vecMenuFriends )
	{
		// Sort the friends list alphabetically
		std::sort( vecMenuFriends->begin(), vecMenuFriends->end() );
		
		std::vector<FriendsListMenuItem_t> vecInGameMenuItemFriends;
		std::vector<FriendsListMenuItem_t> vecOnlineMenuItemFriends;
		std::vector<FriendsListMenuItem_t> vecOfflineMenuItemFriends;	

/*		Post-C++11 
		std::copy_if(vecMenuFriends->begin(), vecMenuFriends->end(), std::back_inserter( vecInGameMenuItemFriends ), 
		[]( const FriendsListMenuItem_t& steamFriend ) { return steamFriend.m_bIsFriendInGame; } );

		std::copy_if(vecMenuFriends->begin(), vecMenuFriends->end(), std::back_inserter(vecOnlineMenuItemFriends ),
		[]( const FriendsListMenuItem_t& steamFriend ) { return ( steamFriend.m_bIsFriendOnline ) && !(steamFriend.m_bIsFriendInGame ); } );

		std::copy_if(vecMenuFriends->begin(), vecMenuFriends->end(), std::back_inserter( vecOfflineMenuItemFriends ),
		[]( const FriendsListMenuItem_t& steamFriend ) { return !( steamFriend.m_bIsFriendOnline ); } );
*/
		// Organize the friends list based on friend status ( In-Game, Online and Offline )
		std::vector<FriendsListMenuItem_t>::iterator iter;
		for( iter = vecMenuFriends->begin(); iter != vecMenuFriends->end(); iter++ )
		{
			// Add In-game friends to the vecIngGameMenuItemFriends vector, 
			// online friends to the vecOnlineMenuItemFriends vector
			// and offline friends to the vecOfflineMenuItemFriends vector
			( iter->m_bIsFriendInGame ) ? vecInGameMenuItemFriends.push_back( *iter ) :
			( iter->m_bIsFriendOnline && !iter->m_bIsFriendInGame ) ? vecOnlineMenuItemFriends.push_back( *iter ) :
			vecOfflineMenuItemFriends.push_back( *iter );
		}
		if ( !vecInGameMenuItemFriends.empty() )
		{
			AddMenuItem( CFriendsListMenu::MenuItem_t("In Game", k_menuItemEmpty ) );
			std::vector<FriendsListMenuItem_t>::iterator iter;
			for ( iter = vecInGameMenuItemFriends.begin(); iter != vecInGameMenuItemFriends.end(); )
			{
				// Get, store and show the name of the game the friend is in
				char rgchGameName[MAX_PATH];
				int steamFriendAppID = SteamAppList()->GetAppName( iter->m_steamFriendGameInfo.m_gameID.AppID(), rgchGameName, 256 );
				char rgchFriendAppNameBuffer[MAX_PATH];
				sprintf_safe(rgchFriendAppNameBuffer, "| %s |", rgchGameName, rgchFriendAppNameBuffer);
				AddMenuItem(CFriendsListMenu::MenuItem_t( rgchFriendAppNameBuffer, k_menuItemEmpty ) );

				// Group players in the same game under the same game menu
				const CGameID& steamFriendGameID = iter->m_steamFriendGameInfo.m_gameID;
				do
				{
					AddFriendToMenu( iter->m_steamIDFriend );
					iter++;
				} 
				while ( iter != vecInGameMenuItemFriends.end() && steamFriendGameID == iter->m_steamFriendGameInfo.m_gameID );
			}
			AddMenuItem( CFriendsListMenu::MenuItem_t( "", k_menuItemEmpty ) );
		}		
		if ( !vecOnlineMenuItemFriends.empty() )
		{
			// Create a list with the number of online friends and populate with friend names
			char rgchOnlineListBuffer[256] = { '\0' };
			sprintf_safe( rgchOnlineListBuffer, "Online Friends (%d)", vecOnlineMenuItemFriends.size() );
			AddMenuItem( CFriendsListMenu::MenuItem_t( rgchOnlineListBuffer, k_menuItemEmpty ) );
			std::vector<FriendsListMenuItem_t>::iterator iter;
			for ( iter = vecOnlineMenuItemFriends.begin(); iter != vecOnlineMenuItemFriends.end(); iter++ )
			{
				AddFriendToMenu( iter->m_steamIDFriend );
			}
			AddMenuItem( CFriendsListMenu::MenuItem_t("", k_menuItemEmpty ) );
		}
		if ( !vecOfflineMenuItemFriends.empty() )
		{
			// Create a list with the number of offline friends and populate with friend names
			char rgchOfflineListBuffer[256] = { '\0' };
			sprintf_safe( rgchOfflineListBuffer, "Offline (%d)", vecOfflineMenuItemFriends.size() );
			AddMenuItem( CFriendsListMenu::MenuItem_t( rgchOfflineListBuffer, k_menuItemEmpty ) );
			std::vector<FriendsListMenuItem_t>::iterator iter;
			for ( iter = vecOfflineMenuItemFriends.begin(); iter != vecOfflineMenuItemFriends.end(); iter++ )
			{
				AddFriendToMenu( iter->m_steamIDFriend );
			}
		}
	}

	void AddFriendToMenu(const CSteamID& steamIDFriend)
	{
		const FriendsListMenuItem_t menuItemFriend = { steamIDFriend };

		// Get the friend name
		const char* pszFriendName = SteamFriends()->GetFriendPersonaName(steamIDFriend);

		// Get the friend status
		const EPersonaState steamFriendState = SteamFriends()->GetFriendPersonaState(steamIDFriend);

		// Set the friend name and status (Online friends will have additional status appended to the username )
		SetFriendPersonaStatus(menuItemFriend, steamFriendState, pszFriendName);
	}

	void SetFriendPersonaStatus( const FriendsListMenuItem_t &menuItemFriend, const EPersonaState &steamFriendPersonaState, const char *pszFriendName )
	{
		// For friends already in our friends list, set the friend state to be displayed
		const char* pszFriendStatus = NULL;
		switch (steamFriendPersonaState)
		{
		case k_EPersonaStateBusy:
		{
			pszFriendStatus = "Busy";
		}
		break;
		case k_EPersonaStateAway:
		{
			pszFriendStatus = "Away";
		}
		break;
		case k_EPersonaStateSnooze:
		{
			pszFriendStatus = "Snooze";
		}
		break;
		case k_EPersonaStateMax:
			break;
		case k_EPersonaStateLookingToTrade:
			pszFriendStatus = "Looking To Trade";
			break;
		case k_EPersonaStateLookingToPlay:
			pszFriendStatus = "Looking To Play";
			break;
		default:
		{
			pszFriendStatus = "";
			break;
		}

		// If the friend is online, add Online friend name + status to the friends list
		char rgchFriendNameBuffer[256];
		if (steamFriendPersonaState > k_EPersonaStateOnline)
		{
			// If  friend has additional status info, append status (e.g friend name + (Away, Snooze, etc))
			sprintf_safe(rgchFriendNameBuffer, "%s (%s)", pszFriendName, pszFriendStatus);
		}
		else
		{
			// Only show the friend name for offline friends
			sprintf_safe(rgchFriendNameBuffer, "%s", pszFriendName);
		}
		AddMenuItem(CFriendsListMenu::MenuItem_t(rgchFriendNameBuffer, menuItemFriend));
	}
};

const FriendsListMenuItem_t CFriendsListMenu::k_menuItemEmpty = { k_steamIDNil };

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CFriendsList::CFriendsList( IGameEngine *pGameEngine ) : m_pGameEngine( pGameEngine )
{
	m_pFriendsListMenu = new CFriendsListMenu( pGameEngine );
		
	Show();
}

//-----------------------------------------------------------------------------
// Purpose: Run a frame for the CFriendsList
//-----------------------------------------------------------------------------
void CFriendsList::RunFrame()
{
	m_pFriendsListMenu->RunFrame();	
}

//-----------------------------------------------------------------------------
// Purpose: Handles menu actions when viewing a friends list
//-----------------------------------------------------------------------------
void CFriendsList::OnMenuSelection( FriendsListMenuItem_t selection )
{ 

	// Getting the friendship relationship
	selection.m_eSteamFriendRelationship = SteamFriends()->GetFriendRelationship(selection.m_steamIDFriend);
	
	// If the selected item is not a friend or if we sent a friend request, do nothing
	if (!selection.m_steamIDFriend.IsValid() || selection.m_eSteamFriendRelationship == k_EFriendRelationshipRequestInitiator)
	return;

	m_pFriendsListMenu->ClearMenuItems();
	m_pFriendsListMenu->RunFrame();

	// If we received a friend request, accept or ignore the request	
	if (selection.m_eSteamFriendRelationship == k_EFriendRelationshipRequestRecipient)
	{
		const FriendsListMenuItem_t menuItemAcceptFriendRequest = { selection.m_steamIDFriend, FriendsListMenuItem_t::k_EFriendsListMenuItemAcceptFriendRequest };
		const FriendsListMenuItem_t menuItemIgnoreFriendRequest = { selection.m_steamIDFriend, FriendsListMenuItem_t::k_EFriendsListMenuItemIgnoreFriendRequest };

		switch (selection.m_eCommand)
		{
		case FriendsListMenuItem_t::k_EFriendsListMenuItemAcceptFriendRequest:
			SteamFriends()->ActivateGameOverlayToUser("friendrequestaccept", selection.m_steamIDFriend);
			break;
		case FriendsListMenuItem_t::k_EFriendsListMenuItemIgnoreFriendRequest:
			SteamFriends()->ActivateGameOverlayToUser("friendrequestignore", selection.m_steamIDFriend);
			break;
		default:
			break;
		}
		m_pFriendsListMenu->AddMenuItem(CFriendsListMenu::MenuItem_t("Accept Friend Request", menuItemAcceptFriendRequest));
		m_pFriendsListMenu->AddMenuItem(CFriendsListMenu::MenuItem_t("Ignore Friend Request", menuItemIgnoreFriendRequest));
	}

	// If the friend is already in out friends list, enter the new menu and present friend interaction options
	else if (selection.m_eSteamFriendRelationship == k_EFriendRelationshipFriend)
	{
		const FriendsListMenuItem_t menuItemViewProfile = { selection.m_steamIDFriend, FriendsListMenuItem_t::k_EFriendsListMenuItemViewProfile };
		const FriendsListMenuItem_t menuItemChat = { selection.m_steamIDFriend, FriendsListMenuItem_t::k_EFriendsListMenuItemSendMessage };
		const FriendsListMenuItem_t menuItemBack = { selection.m_steamIDFriend, FriendsListMenuItem_t::k_EFriendsListMenuItemBack };
		const FriendsListMenuItem_t menuItemRemoveFriend = { selection.m_steamIDFriend, FriendsListMenuItem_t::k_EFriendsListMenuItemRemoveFriend };
		
		switch (selection.m_eCommand)
		{
		case FriendsListMenuItem_t::k_EFriendsListMenuItemSendMessage:
			SteamFriends()->ActivateGameOverlayToUser("chat", selection.m_steamIDFriend);
			break;
		case FriendsListMenuItem_t::k_EFriendsListMenuItemViewProfile:
			SteamFriends()->ActivateGameOverlayToUser("steamid", selection.m_steamIDFriend);
			break;
		case FriendsListMenuItem_t::k_EFriendsListMenuItemRemoveFriend:
			SteamFriends()->ActivateGameOverlayToUser("friendremove", selection.m_steamIDFriend);
			break;
		case FriendsListMenuItem_t::k_EFriendsListMenuItemBack:
			m_pFriendsListMenu->ClearMenuItems();
			Show();
			return;
			break;
		default:
			break;
		}

		// Selected friend menu options
		m_pFriendsListMenu->AddMenuItem(CFriendsListMenu::MenuItem_t( "", kk_menuItemEmpty ) );
		m_pFriendsListMenu->AddMenuItem(CFriendsListMenu::MenuItem_t( "Send Message", menuItemChat ) );
		m_pFriendsListMenu->AddMenuItem(CFriendsListMenu::MenuItem_t( "View Profile", menuItemViewProfile ) );
		m_pFriendsListMenu->AddMenuItem(CFriendsListMenu::MenuItem_t( "Remove as Friend", menuItemRemoveFriend ) );
		m_pFriendsListMenu->AddMenuItem(CFriendsListMenu::MenuItem_t( "", kk_menuItemEmpty ) );
		m_pFriendsListMenu->AddMenuItem(CFriendsListMenu::MenuItem_t( "Go Back", menuItemBack ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Immediately update friends status and presence change 
//-----------------------------------------------------------------------------
void CFriendsList::OnPersonaChanged( PersonaStateChange_t *callback )
{
	// Ignore myself
	if ( SteamUser()->GetSteamID() == callback->m_ulSteamID )
	return;
	Show();
}

//-----------------------------------------------------------------------------
// Purpose: Shows / Refreshes the friends list
//-----------------------------------------------------------------------------
void CFriendsList::Show()
{
	m_pFriendsListMenu->Rebuild();
}

const FriendsListMenuItem_t CFriendsList::kk_menuItemEmpty = { k_steamIDNil };
