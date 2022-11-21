/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "TextureShareD3D12PlatformWindows.h"

#include "Logging.h"

namespace UE::TouchEngine::D3DX12
{
	/**
	 * FD3D12CrossGPUHeapSecurityAttributes
	 */
	FD3D12CrossGPUHeapSecurityAttributes::FD3D12CrossGPUHeapSecurityAttributes()
	{
		m_winPSecurityDescriptor = (PSECURITY_DESCRIPTOR)calloc(1, SECURITY_DESCRIPTOR_MIN_LENGTH + 2 * sizeof(void**));
		check(m_winPSecurityDescriptor != (PSECURITY_DESCRIPTOR)NULL);

		PSID* ppSID = (PSID*)((PBYTE)m_winPSecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);
		PACL* ppACL = (PACL*)((PBYTE)ppSID + sizeof(PSID*));

		BOOL retval = InitializeSecurityDescriptor(m_winPSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
		(retval);

		SID_IDENTIFIER_AUTHORITY sidIdentifierAuthority = SECURITY_WORLD_SID_AUTHORITY;
		AllocateAndInitializeSid(&sidIdentifierAuthority, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, ppSID);

		EXPLICIT_ACCESS explicitAccess;
		ZeroMemory(&explicitAccess, sizeof(EXPLICIT_ACCESS));
		explicitAccess.grfAccessPermissions = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
		explicitAccess.grfAccessMode = SET_ACCESS;
		explicitAccess.grfInheritance = INHERIT_ONLY;
		explicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		explicitAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		explicitAccess.Trustee.ptstrName = (LPTSTR)*ppSID;

		SetEntriesInAcl(1, &explicitAccess, NULL, ppACL);

		retval = SetSecurityDescriptorDacl(m_winPSecurityDescriptor, true, *ppACL, false);
		(retval);

		m_winSecurityAttributes.nLength = sizeof(m_winSecurityAttributes);
		m_winSecurityAttributes.lpSecurityDescriptor = m_winPSecurityDescriptor;
		m_winSecurityAttributes.bInheritHandle = true;
	}

	FD3D12CrossGPUHeapSecurityAttributes::~FD3D12CrossGPUHeapSecurityAttributes()
	{
		PSID* ppSID = (PSID*)((PBYTE)m_winPSecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);
		PACL* ppACL = (PACL*)((PBYTE)ppSID + sizeof(PSID*));

		if (*ppSID)
			FreeSid(*ppSID);
		if (*ppACL)
			LocalFree(*ppACL);

		free(m_winPSecurityDescriptor);
	}

	/**
	 * FTextureShareD3D12SharedResourceSecurityAttributes
	 */
	void FTextureShareD3D12SharedResourceSecurityAttributes::Release()
	{
		if (pEveryoneSID)
			FreeSid(pEveryoneSID);
		if (pAdminSID)
			FreeSid(pAdminSID);
		if (pACL)
			LocalFree(pACL);
		if (pSD)
			LocalFree(pSD);
	}

	bool FTextureShareD3D12SharedResourceSecurityAttributes::Initialize()
	{
		DWORD dwRes;

		EXPLICIT_ACCESS ea[2];
		SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
		SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;

		// Create a well-known SID for the Everyone group.
		if (!AllocateAndInitializeSid(&SIDAuthWorld, 1,
			SECURITY_WORLD_RID,
			0, 0, 0, 0, 0, 0, 0,
			&pEveryoneSID))
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("AllocateAndInitializeSid Error %u"), GetLastError());
			Release();
			return false;
		}

		// Initialize an EXPLICIT_ACCESS structure for an ACE.
		// The ACE will allow Everyone read access to the key.
		ZeroMemory(&ea, 2 * sizeof(EXPLICIT_ACCESS));
		ea[0].grfAccessPermissions = KEY_ALL_ACCESS;
		ea[0].grfAccessMode = SET_ACCESS;
		ea[0].grfInheritance = NO_INHERITANCE;
		ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		//ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
		ea[0].Trustee.ptstrName = (LPTSTR)pEveryoneSID;

		// Create a SID for the BUILTIN\Administrators group.
		if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
			SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS,
			0, 0, 0, 0, 0, 0,
			&pAdminSID))
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("AllocateAndInitializeSid Error %u"), GetLastError());
			Release();
			return false;
		}

		// Initialize an EXPLICIT_ACCESS structure for an ACE.
		// The ACE will allow the Administrators group full access to
		// the key.
		ea[1].grfAccessPermissions = KEY_ALL_ACCESS;
		ea[1].grfAccessMode = SET_ACCESS;
		ea[1].grfInheritance = NO_INHERITANCE;
		ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
		ea[1].Trustee.ptstrName = (LPTSTR)pAdminSID;

		// Create a new ACL that contains the new ACEs.
		dwRes = SetEntriesInAcl(2, ea, NULL, &pACL);
		if (ERROR_SUCCESS != dwRes)
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("SetEntriesInAcl Error %u"), GetLastError());
			Release();
			return false;

		}

		// Initialize a security descriptor.  
		pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,
			SECURITY_DESCRIPTOR_MIN_LENGTH);
		if (NULL == pSD)
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("LocalAlloc Error %u"), GetLastError());
			Release();
			return false;

		}

		if (!InitializeSecurityDescriptor(pSD,
			SECURITY_DESCRIPTOR_REVISION))
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("InitializeSecurityDescriptor Error %u"), GetLastError());
			Release();
			return false;
		}

		// Add the ACL to the security descriptor. 
		if (!SetSecurityDescriptorDacl(pSD,
			true,     // bDaclPresent flag   
			pACL,
			false))   // not a default DACL 
		{
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("SetSecurityDescriptorDacl Error %u"), GetLastError());
			Release();
			return false;
		}

		// Initialize a security attributes structure.
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = pSD;
		sa.bInheritHandle = false;

		return true;
	}
}