/*
    This file is part of SourcePawn SteamWorks.

    SourcePawn SteamWorks is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, as per version 3 of the License.

    SourcePawn SteamWorks is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SourcePawn SteamWorks.  If not, see <http://www.gnu.org/licenses/>.
	
	Author: Kyle Sanderson (KyleS).
*/

#pragma once
#include "isteamgameserver.h"
#include "steam_gameserver.h"
#include "smsdk_ext.h"

class SteamWorksGSHooks
{
	public:
		SteamWorksGSHooks();
		~SteamWorksGSHooks();
	
	public:
		void AddHooks(ISteamGameServer *pGameServer);
		void RemoveHooks(ISteamGameServer *pGameServer, bool destroyed = false);
	
	public:
		KHook::Return<bool> WasRestartRequested(ISteamGameServer*);
		KHook::Return<void> LogOnAnonymous(ISteamGameServer*);
		KHook::Return<EBeginAuthSessionResult> BeginAuthSession(ISteamGameServer*, const void*, int, CSteamID);
		
	private:
		IForward *pFORR; /* On Restart Requested. */
		IForward *pFOTR; /* On Token Requested. */
		IForward *pOBAS; /* On Begin Auth Session. */
		unsigned char uHooked;

	protected:
		KHook::Virtual<ISteamGameServer, bool> m_WasRestartRequested;
		KHook::Virtual<ISteamGameServer, void> m_LogOnAnonymous;
		KHook::Virtual<ISteamGameServer, EBeginAuthSessionResult, const void*, int, CSteamID> m_BeginAuthSession;

};

void OurGameFrameHook(bool simulating);

#include "extension.h"