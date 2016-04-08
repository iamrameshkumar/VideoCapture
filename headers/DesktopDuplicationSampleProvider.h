#pragma once
#include <DXGI1_2.h>

#include "SampleProvider.h"

_COM_SMARTPTR_TYPEDEF(IDXGIOutputDuplication, __uuidof(IDXGIOutputDuplication));
_COM_SMARTPTR_TYPEDEF(ID3D11DeviceContext, __uuidof(ID3D11DeviceContext));
_COM_SMARTPTR_TYPEDEF(ID3D11Texture2D, __uuidof(ID3D11Texture2D));

class DesktopDuplicationSampleProvider : public SampleProvider
{
public:
	DesktopDuplicationSampleProvider();
	~DesktopDuplicationSampleProvider();
	HRESULT GetSample(IMFSample** ppSample) const override;
private:
	IMFMediaBufferPtr		   buffer_;
	IDXGIOutputDuplicationPtr  duplication_;
	DXGI_OUTDUPL_DESC          duplication_desc_;
	ID3D11DevicePtr            device_;
	ID3D11DeviceContextPtr     сontext_;
	ID3D11Texture2DPtr         texture2D_;
};
