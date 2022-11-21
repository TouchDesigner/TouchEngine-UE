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

namespace UE::TouchEngine
{
	/**
	 * Leverages C++ destruction mechanics to executing a callback when a bunch of futures has finished executing.
	 *
	 * Usage:
	 *	1. SyncPoint = FFutureSyncPoint::CreateSyncPoint()
	 *	2. For each future, Next([SyncPoint](auto){})
	 *	3. Once all futures complete, the pointer will become
	 */
	class FFutureSyncPoint
	{
	public:

		static TSharedRef<FFutureSyncPoint, ESPMode::ThreadSafe> CreateSyncPoint(TUniqueFunction<void()> OnFuturesDone)
		{
			return MakeShared<FFutureSyncPoint, ESPMode::ThreadSafe>(MoveTemp(OnFuturesDone));
		}

		template<typename T>
		static void SyncFutureCompletion(TArrayView<TFuture<T>> Futures, TUniqueFunction<void()> OnFuturesDone)
		{
			const TSharedRef<FFutureSyncPoint, ESPMode::ThreadSafe> SyncPoint = CreateSyncPoint(MoveTemp(OnFuturesDone));
			for (TFuture<T>& Future : Futures)
			{
				Future.Next([SyncPoint](auto){});
			}
		}
			
		FFutureSyncPoint(TUniqueFunction<void()> OnFuturesDone)
			: OnFuturesDone(MoveTemp(OnFuturesDone))
		{}

		~FFutureSyncPoint()
		{
			OnFuturesDone();
		}
		
	private:
			
		TUniqueFunction<void()> OnFuturesDone;
	};
}

