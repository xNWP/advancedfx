#include "stdafx.h"

#include "ClientToolsCssV34.h"

#include "addresses.h"
#include "RenderView.h"

#include <cssV34/sdk_src/public/tier1/KeyValues.h>
#include <cssV34/sdk_src/public/tools/bonelist.h>

#include <shared/StringTools.h>

using namespace SOURCESDK::CSSV34;

CClientToolsCssV34 * CClientToolsCssV34::m_Instance = 0;

CClientToolsCssV34::CClientToolsCssV34(SOURCESDK::CSSV34::IClientTools * clientTools)
	: CClientTools()
	, m_ClientTools(clientTools)
{
	m_Instance = this;

	m_ClientTools->EnableRecordingMode(true);
}

CClientToolsCssV34::~CClientToolsCssV34()
{
	m_ClientTools->EnableRecordingMode(false);

	m_Instance = 0;
}

void CClientToolsCssV34::OnPostToolMessage(void * hEntity, void * msg)
{
	CClientTools::OnPostToolMessage(hEntity, msg);

	OnPostToolMessageCssV34(reinterpret_cast<SOURCESDK::CSSV34::HTOOLHANDLE>(hEntity), reinterpret_cast<SOURCESDK::CSSV34::KeyValues *>(msg));
}

void CClientToolsCssV34::OnPostToolMessageCssV34(SOURCESDK::CSSV34::HTOOLHANDLE hEntity, SOURCESDK::CSSV34::KeyValues * msg)
{
	if (!(hEntity != SOURCESDK::CSSV34::HTOOLHANDLE_INVALID && msg))
		return;

	char const * msgName = msg->GetName();

	if (!strcmp("entity_state", msgName))
	{
		if (GetRecording())
		{
			char const * className = m_ClientTools->GetClassname(hEntity);

			if (0 != Debug_get())
			{
				if (2 <= Debug_get())
				{
					Tier0_Msg("-- %s (%i) --\n", className, hEntity);
					for (SOURCESDK::CSSV34::KeyValues * subKey = msg->GetFirstSubKey(); 0 != subKey; subKey = subKey->GetNextKey())
						Tier0_Msg("%s,\n", subKey->GetName());
					Tier0_Msg("----\n");
				}

				if (SOURCESDK::CSSV34::BaseEntityRecordingState_t * pBaseEntityRs = (SOURCESDK::CSSV34::BaseEntityRecordingState_t *)(msg->GetPtr("baseentity")))
				{
					Tier0_Msg("%i: %s: %s\n", hEntity, className, pBaseEntityRs->m_pModelName);
				}
			}

			bool isPlayer =
				false
				|| className && (
					!strcmp(className, "class C_CSPlayer")
					|| !strcmp(className, "class C_CSRagdoll")
					)
				;

			bool isWeapon =
				false
				|| className && (
					StringBeginsWith(className, "weapon_")
					|| !strcmp(className, "class C_BreakableProp")
					)
				;

			bool isProjectile =
				className && !strcmp(className, "grenade")
				;

			bool isViewModel =
				className && (
					!strcmp(className, "viewmodel")
					)
				;

			if (false
				|| RecordPlayers_get() && isPlayer
				|| RecordWeapons_get() && isWeapon
				|| RecordProjectiles_get() && isProjectile
				|| RecordViewModel_get() && isViewModel
				)
			{
				SOURCESDK::CSSV34::BaseEntityRecordingState_t * pBaseEntityRs = (SOURCESDK::CSSV34::BaseEntityRecordingState_t *)(msg->GetPtr("baseentity"));

				if (!RecordInvisible_get() && !(pBaseEntityRs && pBaseEntityRs->m_bVisible))
				{
					// Entity not visible, avoid trash data:

					std::map<SOURCESDK::CSSV34::HTOOLHANDLE, bool>::iterator it = m_TrackedHandles.find(hEntity);
					if (it != m_TrackedHandles.end() && it->second)
					{
						MarkHidden((int)(it->first));

						it->second = false;
					}

					return;
				}

				bool wasVisible = false;

				WriteDictionary("entity_state");
				Write((int)hEntity);
				{
					SOURCESDK::CSSV34::BaseEntityRecordingState_t * pBaseEntityRs = (SOURCESDK::CSSV34::BaseEntityRecordingState_t *)(msg->GetPtr("baseentity"));
					if (pBaseEntityRs)
					{
						WriteDictionary("baseentity");
						//Write((float)pBaseEntityRs->m_flTime);
						WriteDictionary(pBaseEntityRs->m_pModelName);
						Write((bool)pBaseEntityRs->m_bVisible);
						Write(pBaseEntityRs->m_vecRenderOrigin);
						Write(pBaseEntityRs->m_vecRenderAngles);

						wasVisible = pBaseEntityRs->m_bVisible;
					}
				}

				m_TrackedHandles[hEntity] = wasVisible;

				{
					SOURCESDK::CSSV34::BaseAnimatingRecordingState_t * pBaseAnimatingRs = (SOURCESDK::CSSV34::BaseAnimatingRecordingState_t *)(msg->GetPtr("baseanimating"));
					if (pBaseAnimatingRs)
					{
						WriteDictionary("baseanimating");
						//Write((int)pBaseAnimatingRs->m_nSkin);
						//Write((int)pBaseAnimatingRs->m_nBody);
						//Write((int)pBaseAnimatingRs->m_nSequence);
						Write((bool)(0 != pBaseAnimatingRs->m_pBoneList));
						if (pBaseAnimatingRs->m_pBoneList)
						{
							Write(pBaseAnimatingRs->m_pBoneList);
						}
					}
				}

				WriteDictionary("/");

				bool viewModel = 0 != msg->GetInt("viewmodel");

				Write((bool)viewModel);
			}
		}
	}
	else if (!strcmp("deleted", msgName))
	{
		std::map<SOURCESDK::CSSV34::HTOOLHANDLE, bool>::iterator it = m_TrackedHandles.find(hEntity);
		if (it != m_TrackedHandles.end())
		{
			if (GetRecording())
			{
				WriteDictionary("deleted");
				Write((int)(it->first));
			}

			m_TrackedHandles.erase(it);
		}
	}
	else if (!strcmp("created", msgName))
	{
		if (0 != Debug_get() && hEntity != SOURCESDK::CSGO::HTOOLHANDLE_INVALID)
		{
			Tier0_Msg("%i n/a: %s\n", hEntity, m_ClientTools->GetClassname(hEntity));
		}

		if (hEntity != SOURCESDK::CSGO::HTOOLHANDLE_INVALID)// && m_ClientTools->ShouldRecord(hEntity))
		{
			m_TrackedHandles[hEntity] = false;
			if(GetRecording()) m_ClientTools->SetRecording(hEntity, true);
		}
	}
}

void CClientToolsCssV34::OnBeforeFrameRenderStart(void)
{
	CClientTools::OnBeforeFrameRenderStart();

}

void CClientToolsCssV34::OnAfterFrameRenderEnd(void)
{

	CClientTools::OnAfterFrameRenderEnd();
}

void CClientToolsCssV34::StartRecording(wchar_t const * fileName)
{
	CClientTools::StartRecording(fileName);

	if (GetRecording())
	{
		for (std::map<SOURCESDK::CSSV34::HTOOLHANDLE, bool>::iterator it = m_TrackedHandles.begin(); it != m_TrackedHandles.end(); ++it)
		{
			m_ClientTools->SetRecording(it->first, true);
		}
	}
}

void CClientToolsCssV34::EndRecording()
{
	if (GetRecording())
	{
		for (std::map<SOURCESDK::CSSV34::HTOOLHANDLE, bool>::iterator it = m_TrackedHandles.begin(); it != m_TrackedHandles.end(); ++it)
		{
			m_ClientTools->SetRecording(it->first, false);
		}
	}

	CClientTools::EndRecording();
}

void CClientToolsCssV34::Write(SOURCESDK::CSSV34::CBoneList const * value)
{
	Write((int)value->m_nBones);

	for (int i = 0; i < value->m_nBones; ++i)
	{
		Write(value->m_vecPos[i]);
		Write(value->m_quatRot[i]);
	}
}
