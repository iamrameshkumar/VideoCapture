#pragma once
#define D3D_DEBUG_INFO
#include <d3d9.h>
#include <mfobjects.h>
#include <atlbase.h>
#include "SampleProvider.h"


class ScreenSampleProvider : SampleProvider
{
public:
	ScreenSampleProvider(HWND hWnd);
	HRESULT GetSample(IMFSample ** ppSample) override;
private:
	CComPtr<IDirect3DDevice9>  g_pd3dDevice;
	CComPtr<IDirect3DSurface9> g_pSurface;
	CComPtr<IDirect3D9>        g_pD3D;
	D3DDISPLAYMODE	           ddm;
};
