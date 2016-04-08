#pragma once
#include <mfobjects.h>
//#include <boost/noncopyable.hpp>

struct SampleProvider// : boost::noncopyable
{
	virtual HRESULT GetSample(IMFSample** ppSample) const = 0;
	virtual ~SampleProvider() = 0;
};

inline SampleProvider::~SampleProvider() {}