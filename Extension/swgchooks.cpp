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

#include "swgchooks.h"

enum
{
	eUnhooked = 0,
	eHooking,
	eHooked
};

static ISteamGameCoordinator *GetSteamGCPointer()
{
	return g_SteamWorks.pSWGameServer->GetGameCoordinator();
}

SteamWorksGCHooks::SteamWorksGCHooks() : m_SendMessage(&ISteamGameCoordinator::SendMessage, this, &SteamWorksGCHooks::SendMessage, nullptr), m_IsMessageAvailable(&ISteamGameCoordinator::IsMessageAvailable, this, nullptr, &SteamWorksGCHooks::IsMessageAvailable), m_RetrieveMessage(&ISteamGameCoordinator::RetrieveMessage, this, &SteamWorksGCHooks::RetrieveMessage, nullptr)
{
	this->uHooked = eHooking;
	this->pGCSendMsg = forwards->CreateForward("SteamWorks_GCSendMessage", ET_Event, 3, NULL, Param_Cell, Param_String, Param_Cell);
	this->pGCMsgAvail = forwards->CreateForward("SteamWorks_GCMsgAvailable", ET_Ignore, 1, NULL, Param_Cell);
	this->pGCRetMsg = forwards->CreateForward("SteamWorks_GCRetrieveMessage", ET_Event, 4, NULL, Param_Cell, Param_String, Param_Cell, Param_Cell);
	
	ISteamGameCoordinator *pGC = GetSteamGCPointer();
	if (pGC)
	{
		this->AddHooks(pGC);
	}
	else
	{
		smutils->AddGameFrameHook(OurGCGameFrameHook);
	}
}

SteamWorksGCHooks::~SteamWorksGCHooks()
{
	this->RemoveHooks(GetSteamGCPointer(), true);
	smutils->RemoveGameFrameHook(OurGCGameFrameHook);
	forwards->ReleaseForward(this->pGCSendMsg);
	forwards->ReleaseForward(this->pGCMsgAvail);
	forwards->ReleaseForward(this->pGCRetMsg);
}

KHook::Return<EGCResults> SteamWorksGCHooks::SendMessage(ISteamGameCoordinator* pGC, uint32 unMsgType, const void *pubData, uint32 cubData)
{
	if (this->pGCSendMsg->GetFunctionCount() == 0)
	{
		return { KHook::Action::Ignore, k_EGCResultOK };
	}

	cell_t Result = k_EGCResultOK;
	this->pGCSendMsg->PushCell(unMsgType);
	this->pGCSendMsg->PushStringEx(reinterpret_cast<char *>(const_cast<void *>(pubData)), cubData, SM_PARAM_STRING_BINARY | SM_PARAM_STRING_COPY, 0);
	this->pGCSendMsg->PushCell(cubData);
	this->pGCSendMsg->Execute(&Result);

	if (Result != k_EGCResultOK)
	{
		if (Result == -1)
		{
			Result = k_EGCResultOK;
		}

		return { KHook::Action::Supersede, static_cast<EGCResults>(Result) };
	}

	return { KHook::Action::Ignore, k_EGCResultOK };
}

KHook::Return<bool> SteamWorksGCHooks::IsMessageAvailable(ISteamGameCoordinator* pGC, uint32_t *pcubMsgSize)
{
	if (this->pGCMsgAvail->GetFunctionCount() == 0)
	{
		return { KHook::Action::Ignore, false };
	}

	bool res = *(bool*)KHook::GetCurrentValuePtr();
	if (!res)
	{
		return { KHook::Action::Ignore, false };
	}

	uint32_t shill;
	if (!pcubMsgSize)
	{
		shill = 0;
		m_IsMessageAvailable.CallOriginal(pGC, &shill);
		pcubMsgSize = &shill;
	}

	this->pGCMsgAvail->PushCell(*pcubMsgSize);
	this->pGCMsgAvail->Execute(NULL);
	return { KHook::Action::Ignore, true };
}

KHook::Return<EGCResults> SteamWorksGCHooks::RetrieveMessage(ISteamGameCoordinator* pGC, uint32 *punMsgType, void *pubDest, uint32 cubDest, uint32 *pcubMsgSize)
{
	if (this->pGCRetMsg->GetFunctionCount() == 0)
	{
		return { KHook::Action::Ignore, k_EGCResultOK };
	}

	/* Don't trust a bitch, except for 2GD. https://www.youtube.com/watch?v=MgePh_YJgrc */
	cell_t Result = k_EGCResultOK;
	EGCResults res = m_RetrieveMessage.CallOriginal(pGC, punMsgType, pubDest, cubDest, pcubMsgSize);
	if (punMsgType)
		this->pGCRetMsg->PushCell(*punMsgType);
	else
		this->pGCRetMsg->PushCell(0);

	if (pubDest)
		this->pGCRetMsg->PushStringEx(reinterpret_cast<char *>(pubDest), cubDest, SM_PARAM_STRING_BINARY | SM_PARAM_STRING_COPY, 0);
	else
		this->pGCRetMsg->PushStringEx(const_cast<char *>(""), 1, SM_PARAM_STRING_BINARY | SM_PARAM_STRING_COPY, 0);

	this->pGCRetMsg->PushCell(cubDest);

	if (pcubMsgSize)
		this->pGCRetMsg->PushCell(*pcubMsgSize);
	else
		this->pGCRetMsg->PushCell(0);

	this->pGCRetMsg->Execute(&Result);

	if (Result != k_EGCResultOK)
	{
		if (Result == -1)
		{
			Result = k_EGCResultOK;
		}

		return { KHook::Action::Supersede, static_cast<EGCResults>(Result) };
	}

	return { KHook::Action::Supersede, res };
}

void SteamWorksGCHooks::AddHooks(ISteamGameCoordinator *pGC)
{
	if (this->uHooked == eHooked || pGC == NULL)
	{
		return;
	}

	this->uHooked = eHooked;
	m_SendMessage.Add(pGC);
	m_IsMessageAvailable.Add(pGC);
	m_RetrieveMessage.Add(pGC);
}

void SteamWorksGCHooks::RemoveHooks(ISteamGameCoordinator *pGC, bool destroyed)
{
	if (this->uHooked != eHooked || pGC == NULL)
	{
		return;
	}

	m_SendMessage.Remove(pGC);
	m_IsMessageAvailable.Remove(pGC);
	m_RetrieveMessage.Remove(pGC);

	if (destroyed)
	{
		this->uHooked = eUnhooked;
		return;
	}

	this->uHooked = eHooking;
	smutils->AddGameFrameHook(OurGCGameFrameHook);
}

void OurGCGameFrameHook(bool simulating) /* What we do for SDK independence. */
{
	ISteamGameCoordinator *pGC = GetSteamGCPointer();
	if (pGC == NULL)
	{
		return;
	}

	g_SteamWorks.pGCHooks->AddHooks(pGC);
	smutils->RemoveGameFrameHook(OurGCGameFrameHook);
}
