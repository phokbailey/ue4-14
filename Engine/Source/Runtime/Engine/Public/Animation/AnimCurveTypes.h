// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimTypes.h"
#include "Animation/SmartName.h"
#include "Animation/Skeleton.h"
#include "Curves/RichCurve.h"
#include "AnimCurveTypes.generated.h"

/** This is curve flags that are saved in asset and **/
enum EAnimAssetCurveFlags
{
	// Used as morph target curve
	ACF_DriveMorphTarget_DEPRECATED		= 0x00000001, // This has moved to FAnimCurveType:bMorphTarget. Set per skeleton. DO NOT REMOVE UNTIL FrameworkObjectVersion.MoveCurveTypesToSkeleton expires.
	// Used as triggering event
	ACF_DriveAttribute_DEPRECATED		= 0x00000002, // Set per skeleton. DO NOT REMOVE UNTIL FrameworkObjectVersion.MoveCurveTypesToSkeleton expires.
	// Is editable in Sequence Editor
	ACF_Editable						= 0x00000004, // per asset
	// Used as a material curve
	ACF_DriveMaterial_DEPRECATED		= 0x00000008, // This has moved to FAnimCurveType:bMaterial. Set per skeleton. DO NOT REMOVE UNTIL FrameworkObjectVersion.MoveCurveTypesToSkeleton expires.
	// Is a metadata 'curve'
	ACF_Metadata						= 0x00000010, // per asset
	// motifies bone track
	ACF_DriveTrack						= 0x00000020, // @Todo: remove?
	// disabled, right now it's used by track
	ACF_Disabled						= 0x00000040, // per asset

	// default flag when created
	ACF_DefaultCurve					= ACF_Editable,
};

/** UI Curve Parameter type
 * This gets name, and cached UID and use it when needed
 * Also it contains curve types 
 */
USTRUCT()
struct ENGINE_API FAnimCurveParam
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FAnimCurveParam)
	FName Name;

	// name UID for fast access
	SmartName::UID_Type UID;

	FAnimCurveParam()
		: UID(SmartName::MaxUID)
	{}

	// initialize
	void Initialize(USkeleton* Skeleton);

	// this doesn't check CurveType flag
	// because it's possible you don't care about your curve types
	bool IsValid() const
	{
		return UID != SmartName::MaxUID;
	}

	bool IsValidToEvaluate() const
	{
		return IsValid();
	}
};
/**
 * Float curve data for one track
 */
USTRUCT()
struct ENGINE_API FAnimCurveBase
{
	GENERATED_USTRUCT_BODY()

	// Last observed name of the curve. We store this so we can recover from situations that
	// mean the skeleton doesn't have a mapped name for our UID (such as a user saving the an
	// animation but not the skeleton).
	UPROPERTY()
	FName		LastObservedName_DEPRECATED;

	UPROPERTY()
	FSmartName	Name;

private:
	// this flag is mostly used by editor only now
	// however I can't remove this to editor only because 
	// we need DEPRECATED Flag to be loaded in game
	// because those data are stored in asset, and skeleton might not be saved with it. 
	/** Curve Type Flags */
	UPROPERTY()
	int32		CurveTypeFlags;

public:
	FAnimCurveBase(){}

	FAnimCurveBase(FSmartName InName, int32 InCurveTypeFlags)
		: Name(InName)
		, CurveTypeFlags(InCurveTypeFlags)
	{	
	}

	// To be able to use typedef'd types we need to serialize manually
	void PostSerialize(FArchive& Ar);

	/**
	 * Set InFlag to bValue
	 */
	void SetCurveTypeFlag(EAnimAssetCurveFlags InFlag, bool bValue);

	/**
	 * Toggle the value of the specified flag
	 */
	void ToggleCurveTypeFlag(EAnimAssetCurveFlags InFlag);

	/**
	 * Return true if InFlag is set, false otherwise 
	 */
	bool GetCurveTypeFlag(EAnimAssetCurveFlags InFlag) const;

	/**
	 * Set CurveTypeFlags to NewCurveTypeFlags
	 * This just overwrites CurveTypeFlags
	 */
	void SetCurveTypeFlags(int32 NewCurveTypeFlags);
	/** 
	 * returns CurveTypeFlags
	 */
	int32 GetCurveTypeFlags() const;
};

USTRUCT()
struct FFloatCurve : public FAnimCurveBase
{
	GENERATED_USTRUCT_BODY()

	/** Curve data for float. */
	UPROPERTY()
	FRichCurve	FloatCurve;

	FFloatCurve(){}

	FFloatCurve(FSmartName InName, int32 InCurveTypeFlags)
		: FAnimCurveBase(InName, InCurveTypeFlags)
	{
	}

	// we don't want to have = operator. This only copies curves, but leaving naming and everything else intact. 
	void CopyCurve(FFloatCurve& SourceCurve);
	ENGINE_API float Evaluate(float CurrentTime) const;
	void UpdateOrAddKey(float NewKey, float CurrentTime);
	void Resize(float NewLength, bool bInsert/* whether insert or remove*/, float OldStartTime, float OldEndTime);
};

USTRUCT()
struct FVectorCurve : public FAnimCurveBase
{
	GENERATED_USTRUCT_BODY()

	enum EIndex
	{
		X = 0, 
		Y, 
		Z, 
		Max
	};

	/** Curve data for float. */
	UPROPERTY()
	FRichCurve	FloatCurves[3];

	FVectorCurve(){}

	FVectorCurve(FSmartName InName, int32 InCurveTypeFlags)
		: FAnimCurveBase(InName, InCurveTypeFlags)
	{
	}

	// we don't want to have = operator. This only copies curves, but leaving naming and everything else intact. 
	void CopyCurve(FVectorCurve& SourceCurve);
	FVector Evaluate(float CurrentTime, float BlendWeight) const;
	void UpdateOrAddKey(const FVector& NewKey, float CurrentTime);
	bool DoesContainKey() const { return (FloatCurves[0].GetNumKeys() > 0 || FloatCurves[1].GetNumKeys() > 0 || FloatCurves[2].GetNumKeys() > 0);}
	void Resize(float NewLength, bool bInsert/* whether insert or remove*/, float OldStartTime, float OldEndTime);
};

USTRUCT()
struct FTransformCurve: public FAnimCurveBase
{
	GENERATED_USTRUCT_BODY()

	/** Curve data for each transform. */
	UPROPERTY()
	FVectorCurve	TranslationCurve;

	/** Rotation curve - right now we use euler because quat also doesn't provide linear interpolation - curve editor can't handle quat interpolation
	 * If you hit gimbal lock, you should add extra key to fix it. This will cause gimbal lock. 
	 * @TODO: Eventually we'll need FRotationCurve that would contain rotation curve - that will interpolate as slerp or as quaternion 
	 */
	UPROPERTY()
	FVectorCurve	RotationCurve;

	UPROPERTY()
	FVectorCurve	ScaleCurve;

	FTransformCurve(){}

	FTransformCurve(FSmartName InName, int32 InCurveTypeFlags)
		: FAnimCurveBase(InName, InCurveTypeFlags)
	{
	}

	// we don't want to have = operator. This only copies curves, but leaving naming and everything else intact. 
	void CopyCurve(FTransformCurve& SourceCurve);
	FTransform Evaluate(float CurrentTime, float BlendWeight) const;
	ENGINE_API void UpdateOrAddKey(const FTransform& NewKey, float CurrentTime);
	void Resize(float NewLength, bool bInsert/* whether insert or remove*/, float OldStartTime, float OldEndTime);
};

/**
* This is array of curves that run when collecting curves natively 
*/
struct FCurveElement
{
	/** Curve Value */
	float					Value;

	FCurveElement(float InValue, int32	InFlags)
		:  Value(InValue)
	{}
};

/**
 * This struct is used to create curve snap shot of current time when extracted
 */
template <typename InAllocator>
struct FBaseBlendedCurve
{
	typedef InAllocator   Allocator;
	/**
	* List of curve elements for this pose
	*/
	TArray<FCurveElement, Allocator> Elements;

	/**
	* List of SmartName UIDs, retrieved from AnimInstanceProxy (which keeps authority)
	*/
	TArray<SmartName::UID_Type> const * UIDList;


	/**
	 * constructor
	 */
	FBaseBlendedCurve()
		: UIDList(nullptr)
		, bInitialized(false)
	{
	}

	/** Initialize Curve Data from following data */
	void InitFrom(const FBoneContainer& RequiredBones)
	{
		InitFrom(&RequiredBones.GetAnimCurveNameUids());
	}

	void InitFrom(TArray<SmartName::UID_Type> const * InSmartNameUIDs)
	{
		check(InSmartNameUIDs != nullptr);
		UIDList = InSmartNameUIDs;
		Elements.Reset();
		Elements.AddZeroed(UIDList->Num());

		// no name, means no curve
		bInitialized = true;
	}

	template <typename OtherAllocator>
	void InitFrom(const FBaseBlendedCurve<OtherAllocator>& InCurveToInitFrom)
	{
		// make sure this doesn't happen
		check(InCurveToInitFrom.UIDList != nullptr);
		UIDList = InCurveToInitFrom.UIDList;
		Elements.Reset();
		Elements.AddZeroed(UIDList->Num());
		
		bInitialized = true;
	}

	void InitFrom(const FBaseBlendedCurve<Allocator>& InCurveToInitFrom)
	{
		// make sure this doesn't happen
		if (ensure(&InCurveToInitFrom != this))
		{
			check(InCurveToInitFrom.UIDList != nullptr);
			UIDList = InCurveToInitFrom.UIDList;
			Elements.Reset();
			Elements.AddZeroed(UIDList->Num());

			bInitialized = true;
		}
	}

	/** Set value of InUID to InValue */
	void Set(USkeleton::AnimCurveUID InUid, float InValue)
	{
		int32 ArrayIndex;

		check(bInitialized);

		if (UIDList->Find(InUid, ArrayIndex))
		{
			Elements[ArrayIndex].Value = InValue;
		}
	}

	/** Get Value of InUID */
	float Get(USkeleton::AnimCurveUID InUid) const
	{
		int32 ArrayIndex;

		check(bInitialized);

		if (UIDList->Find(InUid, ArrayIndex))
		{
			return Elements[ArrayIndex].Value;
		}

		return 0.f;
	}

	/**
	 * Blend (A, B) using Alpha, same as Lerp
	 */
	//@Todo curve flags won't transfer over - it only overwrites
	void Lerp(const FBaseBlendedCurve& A, const FBaseBlendedCurve& B, float Alpha)
	{
		check(A.Num() == B.Num());
		if (FMath::Abs(Alpha) <= ZERO_ANIMWEIGHT_THRESH)
		{
			// if blend is all the way for child1, then just copy its bone atoms
			Override(A);
		}
		else if (FMath::Abs(Alpha - 1.0f) <= ZERO_ANIMWEIGHT_THRESH)
		{
			// if blend is all the way for child2, then just copy its bone atoms
			Override(B);
		}
		else
		{
			InitFrom(A);
			for (int32 CurveId = 0; CurveId < A.Elements.Num(); ++CurveId)
			{
				Elements[CurveId].Value = FMath::Lerp(A.Elements[CurveId].Value, B.Elements[CurveId].Value, Alpha);
			}
		}
	}

	/**
	 * Blend with Other using Alpha, same as Lerp 
	 */
	void LerpTo(const FBaseBlendedCurve& Other, float Alpha)
	{
		check(Num() == Other.Num());
		if (FMath::Abs(Alpha) <= ZERO_ANIMWEIGHT_THRESH)
		{
			return;
		}
		else if (FMath::Abs(Alpha - 1.0f) <= ZERO_ANIMWEIGHT_THRESH)
		{
			// if blend is all the way for child2, then just copy its bone atoms
			Override(Other);
		}
		else
		{
			for (int32 CurveId = 0; CurveId < Elements.Num(); ++CurveId)
			{
				Elements[CurveId].Value = FMath::Lerp(Elements[CurveId].Value, Other.Elements[CurveId].Value, Alpha);
			}
		}
	}
	/**
	 * Convert current curves to Additive (this - BaseCurve) if same found
	 */
	void ConvertToAdditive(const FBaseBlendedCurve& BaseCurve)
	{
		check(bInitialized);
		check(Num() == BaseCurve.Num());

		for (int32 CurveId = 0; CurveId < Elements.Num(); ++CurveId)
		{
			Elements[CurveId].Value -= BaseCurve.Elements[CurveId].Value;
		}
	}
	/**
	 * Accumulate the input curve with input Weight
	 */
	void Accumulate(const FBaseBlendedCurve& AdditiveCurve, float Weight)
	{
		check(bInitialized);
		check(Num() == AdditiveCurve.Num());

		if (Weight > ZERO_ANIMWEIGHT_THRESH)
		{
			for (int32 CurveId = 0; CurveId < Elements.Num(); ++CurveId)
			{
				Elements[CurveId].Value += AdditiveCurve.Elements[CurveId].Value * Weight;
			}
		}
	}

	/**
	 * This doesn't blend but combine MAX(current weight, curvetocombine weight)
	 */
	void Combine(const FBaseBlendedCurve& CurveToCombine)
	{
		check(bInitialized);
		check(Num() == CurveToCombine.Num());

		for (int32 CurveId = 0; CurveId < CurveToCombine.Elements.Num(); ++CurveId)
		{
			// if target value is non zero, we accpet target's value
			// originally this code was doing max, but that doesn't make sense since the values can be negative
			// we could try to pick non-zero, but if target value is non-zero, I think we should accept that value 
			// if source is non zero, it will be overriden
			if (CurveToCombine.Elements[CurveId].Value != 0.f)
			{
				Elements[CurveId].Value = CurveToCombine.Elements[CurveId].Value;
			}

		}
	}

	/**
	 * Override with inupt curve * weight
	 */
	void Override(const FBaseBlendedCurve& CurveToOverrideFrom, float Weight)
	{
		InitFrom(CurveToOverrideFrom);

		if (FMath::IsNearlyEqual(Weight, 1.f))
		{
			Override(CurveToOverrideFrom);
		}
		else
		{
			for (int32 CurveId = 0; CurveId < CurveToOverrideFrom.Elements.Num(); ++CurveId)
			{
				Elements[CurveId].Value = CurveToOverrideFrom.Elements[CurveId].Value * Weight;
			}
		}
	}

	/**
	 * Override with inupt curve 
	 */
	void Override(const FBaseBlendedCurve& CurveToOverrideFrom)
	{
		InitFrom(CurveToOverrideFrom);
		Elements.Reset();
		Elements.Append(CurveToOverrideFrom.Elements);
	}

	/** Return number of elements */
	int32 Num() const { return Elements.Num(); }

	/** CopyFrom as expected. */
	template <typename OtherAllocator>
	void CopyFrom(const FBaseBlendedCurve<OtherAllocator>& CurveToCopyFrom)
	{
		checkf(CurveToCopyFrom.IsValid(), TEXT("Copying data from an invalid curve UIDList: 0x%x  (Sizes %i/%i)"), CurveToCopyFrom.UIDList, (CurveToCopyFrom.UIDList ? CurveToCopyFrom.UIDList->Num() : -1), CurveToCopyFrom.Elements.Num());
		UIDList = CurveToCopyFrom.UIDList;
		Elements.Reset();
		Elements.Append(CurveToCopyFrom.Elements);
		bInitialized = true;
	}

	void CopyFrom(const FBaseBlendedCurve<Allocator>& CurveToCopyFrom)
	{
		if (&CurveToCopyFrom != this)
		{
			checkf(CurveToCopyFrom.IsValid(), TEXT("Copying data from an invalid curve UIDList: 0x%x  (Sizes %i/%i)"), CurveToCopyFrom.UIDList, (CurveToCopyFrom.UIDList ? CurveToCopyFrom.UIDList->Num() : -1), CurveToCopyFrom.Elements.Num());
			UIDList = CurveToCopyFrom.UIDList;
			Elements.Reset();
			Elements.Append(CurveToCopyFrom.Elements);
			bInitialized = true;
		}
	}
	/** Empty */
	void Empty()
	{
		// Set to nullptr as we only received a ptr reference from USkeleton
		UIDList = nullptr;
		Elements.Reset();
		bInitialized = false;
	}

	/**  Whether initialized or not */
	bool bInitialized;
	/** Empty and allocate Count number */
	void Reset(int32 Count)
	{
		Elements.Reset();
		Elements.Reserve(Count);
	}

	// Only checks bare minimal validity. (namely that we have a UID list and that it 
	// is the same size as our element list
	bool IsValid() const
	{
		return UIDList && (Elements.Num() == UIDList->Num());
	}
};

struct FBlendedHeapCurve;

struct ENGINE_API FBlendedCurve : public FBaseBlendedCurve<FAnimStackAllocator>
{
};

struct ENGINE_API FBlendedHeapCurve : public FBaseBlendedCurve<FDefaultAllocator>
{
	/** Once moved, source is invalid */
	void MoveFrom(FBlendedHeapCurve& CurveToMoveFrom)
	{
    	UIDList = CurveToMoveFrom.UIDList;
		CurveToMoveFrom.UIDList = nullptr;
		Elements = MoveTemp(CurveToMoveFrom.Elements);
		bInitialized = true;
		CurveToMoveFrom.bInitialized = false;
	}

};

/**
 * Raw Curve data for serialization
 */
USTRUCT()
struct FRawCurveTracks
{
	GENERATED_USTRUCT_BODY()

	enum ESupportedCurveType
	{
		FloatType,
		VectorType,
		TransformType,
		Max, 
	};

	UPROPERTY()
	TArray<FFloatCurve>		FloatCurves;

#if WITH_EDITORONLY_DATA
	/**
	 * @note : Currently VectorCurves are not evaluated or used for anything else but transient data for modifying bone track
	 *			Note that it doesn't have UPROPERTY tag yet. In the future, we'd like this to be serialized, but not for now
	 **/
	UPROPERTY(transient)
	TArray<FVectorCurve>	VectorCurves;

	/**
	 * @note : TransformCurves are used to edit additive animation in editor. 
	 **/
	UPROPERTY()
	TArray<FTransformCurve>	TransformCurves;
#endif // #if WITH_EDITOR_DATA

	/**
	 * Evaluate curve data at the time CurrentTime, and add to Instance. It only evaluates Float Curve for now
	 *
	 * return true if curve exists, false otherwise
	 */
	void EvaluateCurveData( FBlendedCurve& Curves, float CurrentTime ) const;

#if WITH_EDITOR
	/**
	 *	Evaluate transform curves 
	 */
	ENGINE_API void EvaluateTransformCurveData(USkeleton * Skeleton, TMap<FName, FTransform>&OutCurves, float CurrentTime, float BlendWeight) const;

	/**
	* Add new float curve from the given UID if not existing and add the key with time/value
	*/
	ENGINE_API void AddFloatCurveKey(const FSmartName& NewCurve, int32 CurveFlags, float Time, float Value);
	ENGINE_API void RemoveRedundantKeys();

#endif // WITH_EDITOR
	/**
	 * Find curve data based on the curve UID
	 */
	ENGINE_API FAnimCurveBase * GetCurveData(USkeleton::AnimCurveUID Uid, ESupportedCurveType SupportedCurveType = FloatType);
	/**
	 * Add new curve from the provided UID and return true if success
	 * bVectorInterpCurve == true, then it will create FVectorCuve, otherwise, FFloatCurve
	 */
	ENGINE_API bool AddCurveData(const FSmartName& NewCurve, int32 CurveFlags = ACF_DefaultCurve, ESupportedCurveType SupportedCurveType = FloatType);

	/**
	 * Delete curve data 
	 */
	ENGINE_API bool DeleteCurveData(const FSmartName& CurveToDelete, ESupportedCurveType SupportedCurveType = FloatType);

	/**
	 * Delete all curve data 
	 */
	ENGINE_API void DeleteAllCurveData(ESupportedCurveType SupportedCurveType = FloatType);

	/**
	 * Duplicate curve data
	 * 
	 */
	ENGINE_API bool DuplicateCurveData(const FSmartName& CurveToCopy, const FSmartName& NewCurve, ESupportedCurveType SupportedCurveType = FloatType);

	/**
	 * Updates the DisplayName field of the curves from the provided name container
	 */
	ENGINE_API void RefreshName(const FSmartNameMapping* NameMapping, ESupportedCurveType SupportedCurveType = FloatType);

	/** 
	 * Serialize
	 */
	void PostSerialize(FArchive& Ar);

	/*
	 * resize curve length. If longer, it doesn't do any. If shorter, remove previous keys and add new key to the end of the frame. 
	 */
	void Resize(float TotalLength, bool bInsert/* whether insert or remove*/, float OldStartTime, float OldEndTime);
	/** 
	 * Clear all keys
	 */
	void Empty()
	{
		FloatCurves.Empty();
#if WITH_EDITORONLY_DATA
		VectorCurves.Empty();
		TransformCurves.Empty();
#endif
	}

	void SortFloatCurvesByUID()
	{
		struct FCurveSortByUid
		{
			FORCEINLINE bool operator()(const FFloatCurve& A, const FFloatCurve& B) const
			{
				return (A.Name.UID < B.Name.UID);
			}
		};

		FloatCurves.Sort(FCurveSortByUid());
	}
private:
	/** 
	 * Adding vector curve support - this is all transient data for now. This does not save and all these data will be baked into RawAnimationData
	 */

	/**
	 * Find curve data based on the curve UID
	 */
	template <typename DataType>
	DataType * GetCurveDataImpl(TArray<DataType>& Curves, USkeleton::AnimCurveUID Uid);

	/**
	 * Add new curve from the provided UID and return true if success
	 * bVectorInterpCurve == true, then it will create FVectorCuve, otherwise, FFloatCurve
	 */
	template <typename DataType>
	bool AddCurveDataImpl(TArray<DataType>& Curves, const FSmartName& NewCurve, int32 CurveFlags);
	/**
	 * Delete curve data 
	 */
	template <typename DataType>
	bool DeleteCurveDataImpl(TArray<DataType>& Curves, const FSmartName& CurveToDelete);
	/**
	 * Duplicate curve data
	 * 
	 */
	template <typename DataType>
	bool DuplicateCurveDataImpl(TArray<DataType>& Curves, const FSmartName& CurveToCopy, const FSmartName& NewCurve);

	/**
	 * Updates the DisplayName field of the curves from the provided name container
	 */
	template <typename DataType>
	void UpdateLastObservedNamesImpl(TArray<DataType>& Curves, const FSmartNameMapping* NameMapping);
};

FArchive& operator<<(FArchive& Ar, FRawCurveTracks& D);
