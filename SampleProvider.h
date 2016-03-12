#pragma once
#include <mfobjects.h>

struct SampleProvider
{
	virtual HRESULT GetSample(IMFSample** ppSample) = 0;
	virtual ~SampleProvider() = 0;
};

inline SampleProvider::~SampleProvider() {}