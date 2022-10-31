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

#pragma once

#include "CoreMinimal.h"

//
// Windows only include
//

#if PLATFORM_WINDOWS
THIRD_PARTY_INCLUDES_START
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include <mftransform.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <codecapi.h>
#include <shlwapi.h>
#include <mfreadwrite.h>
#include <aclapi.h>
#include <winnt.h>
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"
THIRD_PARTY_INCLUDES_END

namespace UE::TouchEngine::Vulkan
{
	inline const FString GetComErrorDescription(HRESULT Res)
	{
		const uint32 BufSize = 4096;
		WIDECHAR buffer[4096];
		if (::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
			nullptr,
			Res,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
			buffer,
			sizeof(buffer) / sizeof(*buffer),
			nullptr))
		{
			return buffer;
		}
		else
		{
			return TEXT("[cannot find error description]");
		}
	}

	class FVulkanCrossGPUHeapSecurityAttributes
	{
	public:
		FVulkanCrossGPUHeapSecurityAttributes();
		~FVulkanCrossGPUHeapSecurityAttributes();

		const SECURITY_ATTRIBUTES* operator*() const
		{ return &m_winSecurityAttributes; }

	protected:
		SECURITY_ATTRIBUTES m_winSecurityAttributes;
		PSECURITY_DESCRIPTOR m_winPSecurityDescriptor;
	};

	class FVulkanSharedResourceSecurityAttributes
	{
	public:
		FVulkanSharedResourceSecurityAttributes()
		{
			Initialize();
		}
		~FVulkanSharedResourceSecurityAttributes()
		{
			Release();
		}

		const SECURITY_ATTRIBUTES* Get() const
		{ return &sa; }

	protected:
		bool Initialize();
		void Release();

	private:
		PSID pEveryoneSID = nullptr;
		PSID pAdminSID = nullptr;
		PACL pACL = nullptr;
		PSECURITY_DESCRIPTOR pSD = nullptr;
		SECURITY_ATTRIBUTES sa;
	};
}

#endif