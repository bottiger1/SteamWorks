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

#include "swgsdetours.h"

KHook::Return<void> SteamAPIShutdown()
{
	if (g_SteamWorks.pSWGameServer != NULL)
	{
		if (g_SteamWorks.pGSHooks != NULL)
		{
			g_SteamWorks.pGSHooks->RemoveHooks(g_SteamWorks.pSWGameServer->GetGameServer(), false);
		}
		
		g_SteamWorks.pSWGameServer->Reset();
	}

	KHook::GetCurrentValuePtr();
	return { KHook::Action::Ignore };
}
KHook::Function<void> g_SteamAPIShutdownDetour(SteamAPIShutdown, nullptr);

KHook::Return<bool> SteamGameServer_InitSafe(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString)
{
	bool bRet = *(bool*)KHook::GetCurrentValuePtr();
	if (g_SteamWorks.pSWGameServer != NULL && g_SteamWorks.pGSHooks != NULL)
	{
		g_SteamWorks.pGSHooks->AddHooks(g_SteamWorks.pSWGameServer->GetGameServer());
	}
	
	return { KHook::Action::Ignore, bRet };
}
KHook::Function<bool, uint32, uint16, uint16, uint16, EServerMode, const char*> g_SteamGameServer_InitSafeDetour(SteamGameServer_InitSafe, nullptr);

SteamWorksGSDetours::SteamWorksGSDetours()
{
	const char *pLibSteamPath = g_SteamWorks.pSWGameServer->GetLibraryPath();

	void *pSteamSafeInitAddress = NULL;
	void *pSteamShutdownAddress = NULL;

	const char *pInitSafeFuncName = "SteamGameServer_InitSafe";
	const char *pShutdownFuncName = "SteamGameServer_Shutdown";

	IGameConfig *pConfig = NULL;
	if (g_SteamWorks.pSWGameData)
	{
		pConfig = g_SteamWorks.pSWGameData->GetGameData();
		if (pConfig != NULL)
		{
			pConfig->GetMemSig(pShutdownFuncName, &pSteamShutdownAddress);
			pConfig->GetMemSig(pInitSafeFuncName, &pSteamSafeInitAddress);
		}
	}

	ILibrary *pLibrary = libsys->OpenLibrary(pLibSteamPath, NULL, 0);
	if (pLibrary != NULL)
	{
		if (pSteamShutdownAddress == NULL)
		{
			pSteamShutdownAddress = pLibrary->GetSymbolAddress(pShutdownFuncName);
		}
		
		if (pSteamSafeInitAddress == NULL)
		{
			pSteamSafeInitAddress = pLibrary->GetSymbolAddress(pInitSafeFuncName);
		}
		
		pLibrary->CloseLibrary();
	}

	if (pSteamShutdownAddress != NULL)
	{
		g_SteamAPIShutdownDetour.Configure(reinterpret_cast<void(*)()>(pSteamShutdownAddress));
	}
	else
	{
		this->m_pShutdownDetour = NULL;
	}

	if (pSteamSafeInitAddress != NULL)
	{
		g_SteamGameServer_InitSafeDetour.Configure(reinterpret_cast<bool (*)(uint32, uint16, uint16, uint16, EServerMode, const char *)>(pSteamSafeInitAddress));
	}
	else
	{
		this->m_pSafeInitDetour = NULL;
	}
}

SteamWorksGSDetours::~SteamWorksGSDetours()
{
	if (this->m_pShutdownDetour != NULL)
	{
		g_SteamAPIShutdownDetour.~Function();
		this->m_pShutdownDetour = NULL;
	}
	
	if (this->m_pSafeInitDetour != NULL)
	{
		g_SteamGameServer_InitSafeDetour.~Function();
		this->m_pSafeInitDetour = NULL;
	}
}
