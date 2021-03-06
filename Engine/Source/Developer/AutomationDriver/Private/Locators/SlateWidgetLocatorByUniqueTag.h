// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

class IElementLocator;
class FDriverUniqueTagMetaData;

class FSlateWidgetLocatorByUniqueTagFactory
{
public:

	static TSharedRef<IElementLocator, ESPMode::ThreadSafe> Create(
		const TSharedRef<FDriverUniqueTagMetaData>& UniqueMetaData);
};
