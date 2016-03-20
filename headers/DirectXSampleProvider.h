#pragma once
//#define D3D_DEBUG_INFO
#include "SampleProvider.h"


class DirectXSampleProvider : public SampleProvider
{
public:
	DirectXSampleProvider(HWND hWnd);
	HRESULT GetSample(IMFSample ** ppSample) const override;
	HRESULT GetSample1(IMFSample** ppSample);
private:
	IDirect3DDevice9Ptr  g_pd3dDevice;
	IDirect3DSurface9Ptr g_pSurface;
	IDirect3D9Ptr        g_pD3D;
	D3DDISPLAYMODE	     ddm;
};
