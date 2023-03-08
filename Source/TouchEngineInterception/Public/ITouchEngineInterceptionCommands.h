// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

class UTouchEngineContainer;

#include "CoreMinimal.h"
#include "TouchEngineDynamicVariableStruct.h"


/**
 * Interception flags that define how TouchEngine should behave after a message was intercepted.
 */
enum class ETEIResponse : uint8
{
	// Set/reset property on TouchEngine side even though the message was intercepted
	Apply,
	// Do not process the RC command on TouchEngine side. An interceptor will decide what to do with it.
	Intercept,
};

/**
 * Payload serialization type (proxy type for ERCPayloadType to avoid RC module dependency)
 */
enum class ETEIPayloadType : uint8
{
	Cbor,
	Json,
};

/**
 * Remote property access mode  (proxy type for ERCAccess to avoid RC module dependency)
 */
enum class ETEIAccess : uint8
{
	NO_ACCESS,
	READ_ACCESS,
	WRITE_ACCESS,
	WRITE_TRANSACTION_ACCESS,
	WRITE_MANUAL_TRANSACTION_ACCESS,
};

/**
 * Type of operation to perform when setting a remote property's value (proxy type for ERCModifyOperation to avoid module dependency)
 */
enum class ETEIModifyOperation : uint8
{
	EQUAL,
	ADD,
	SUBTRACT,
	MULTIPLY,
	DIVIDE
};

struct FTEToxAssetMetadata
{
	FTEToxAssetMetadata() = default;

	FTEToxAssetMetadata(const FTouchEngineDynamicVariableContainer& InDynamicVariables, const FString& InObjectPath)
		: DynamicVariables(InDynamicVariables)
		, ObjectPath(InObjectPath)
	{
	}
	/** Structure Name */
	static constexpr TCHAR const* Name = TEXT("TEToxAssetMetadata");

	FTouchEngineDynamicVariableContainer DynamicVariables;

	/** Owner object path */
	FString ObjectPath;

	bool bApplyDynamicVariables = false;
};


/**
 * The list of remote control commands available for interception
 */
template <class TResponseType>
class ITouchEngineInterceptionCommands
{
public:
	virtual ~ITouchEngineInterceptionCommands() = default;

	virtual TResponseType VarsSetInputs(FTEToxAssetMetadata& InTEToxAssetMetadata) = 0;

	/**
	 */
	virtual TResponseType VarsGetOutputs(FTEToxAssetMetadata& InTEToxAssetMetadata) = 0;
};


// /**
//  * Object metadata for custom interception/replication/processing purposes
//  */
// struct FRCIObjectMetadata
// {
// public:
// 	FRCIObjectMetadata()
// 	{ }
//
// 	FRCIObjectMetadata(const FString& InObjectPath, const FString& InPropertyPath, const FString& InPropertyPathInfo, const ERCIAccess InAccess)
// 		: ObjectPath(InObjectPath)
// 		, PropertyPath(InPropertyPath)
// 		, PropertyPathInfo(InPropertyPathInfo)
// 		, Access(InAccess)
// 	{
// 		UniquePath = *FString::Printf(TEXT("%s:%s"), *ObjectPath, *PropertyPath);
// 	}
//
// 	virtual ~FRCIObjectMetadata() = default;
//
// public:
// 	/** Get Unique Path for Object metadata */
// 	const FName& GetUniquePath() const
// 	{ return UniquePath; }
// 	
// 	/** Returns true if reference is valid */
// 	virtual bool IsValid() const
// 	{
// 		return !ObjectPath.IsEmpty() && !PropertyPath.IsEmpty() && !PropertyPathInfo.IsEmpty();
// 	}
//
// 	/**
// 	 * FRCObjectMetadata serialization
// 	 *
// 	 * @param Ar       - The archive
// 	 * @param Interception - Instance to serialize/deserialize
// 	 *
// 	 * @return FArchive instance.
// 	 */
// 	friend FArchive& operator<<(FArchive& Ar, FRCIObjectMetadata& Interception)
// 	{
// 		Ar << Interception.ObjectPath;
// 		Ar << Interception.PropertyPath;
// 		Ar << Interception.PropertyPathInfo;
// 		Ar << Interception.Access;
// 		return Ar;
// 	}
//
// public:
// 	/** Owner object path */
// 	FString ObjectPath;
// 	
// 	/** FProperty path */
// 	FString PropertyPath;
//
// 	/** Full property path, including path to inner structs, arrays, maps, etc */
// 	FString PropertyPathInfo;
//
// 	/** Property access type */
// 	ERCIAccess Access = ERCIAccess::NO_ACCESS;
//
// public:
// 	/** Structure Name */
// 	static constexpr TCHAR const* Name = TEXT("RCIObjectMetadata");
// 	
// private:
// 	/** Object Path + Property Path */
//     FName UniquePath;
// };

