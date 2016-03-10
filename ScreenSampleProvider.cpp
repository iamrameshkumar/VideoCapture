#include "ScreenSampleProvider.h"
#include <istream>
#include <string>
#include <evr.h>
#include <d3dx9tex.h>
#include <iostream>
#include <mfapi.h>
#include <common.h>
#include "dirtyhacks.h"

static const int VIDEO_WIDTH = 1600;
static const int VIDEO_HEIGHT = 900;


HRESULT ScreenSampleProvider::getSample(IMFSample ** ppSample) {
	HRESULT hr = g_pd3dDevice->GetFrontBufferData(0, g_pSurface);
	const LONG cbWidth = 4 * VIDEO_WIDTH;
	const DWORD cbBuffer = cbWidth * VIDEO_HEIGHT;
	IMFMediaBuffer *pBuffer = NULL;
	hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);
//	provider.getSample(nullptr);
	IMFMediaBuffer * dbuf;
	IMF2DBuffer * p2dBuf;
	BYTE * pPixels = nullptr;
	hr = pMFCreateDXSurfaceBuffer(IID_IDirect3DSurface9, g_pSurface, false, &dbuf); // kill me please
	hr = dbuf->QueryInterface(__uuidof(IMF2DBuffer), (void**)&p2dBuf);
	LONG mdalong;
	hr = p2dBuf->Lock2D(&pPixels, &mdalong);
	BYTE *pData = NULL;
	// Lock the buffer and copy the video frame to the buffer.
	if (SUCCEEDED(hr))
		hr = pBuffer->Lock(&pData, nullptr, nullptr);
	if (SUCCEEDED(hr)) {
		hr = MFCopyImage(
			pData,                      // Destination buffer.
			cbWidth,                    // Destination stride.
			pPixels,                    // First row in source image.
			cbWidth,                    // Source stride.
			cbWidth,                    // Image width in bytes.
			VIDEO_HEIGHT                // Image height in pixels.
			);
	}
	if (pBuffer)
		pBuffer->Unlock();
	p2dBuf->Unlock2D();

	// Set the data length of the buffer.
	if (SUCCEEDED(hr))
	{
		hr = pBuffer->SetCurrentLength(cbBuffer);
	}

	// Create a media sample and add the buffer to the sample.
	if (SUCCEEDED(hr))
	{
		//		hr = provider.getSample(&pSample);
		hr = MFCreateSample(ppSample);
	}
	if (FAILED(hr))
		std::cout << "getSample failed" << std::endl;
	if (SUCCEEDED(hr))
		hr = (*ppSample)->AddBuffer(pBuffer);
	SAFE_RELEASE(pBuffer);
//	D3DXSaveSurfaceToFile((std::wstring(L"ss") + std::to_wstring(i++) + L".bmp").c_str(), D3DXIFF_BMP, g_pSurface, nullptr, nullptr);
	return hr;
}

ScreenSampleProvider::ScreenSampleProvider(HWND hWnd)
	: g_pD3D(Direct3DCreate9(D3D_SDK_VERSION))
{
	freopen("log.out.txt", "w", stderr);
	if (!g_pD3D) {
		throw std::runtime_error("Could not create Direct3D9");
	}
	D3DDISPLAYMODE	ddm;
	g_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &ddm);
	D3DPRESENT_PARAMETERS	d3dpp{ 0 };
	d3dpp.Windowed = true;  //<-- What does it affect??
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	d3dpp.BackBufferFormat = ddm.Format;
	d3dpp.BackBufferHeight = ddm.Height;
	d3dpp.BackBufferWidth = ddm.Width;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	HRESULT hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice);
	if (FAILED(hr))
	{
		std::cerr << "hr = " << hr << std::endl;
		throw std::runtime_error("could not create D3DDevice");
	}

	g_pd3dDevice->CreateOffscreenPlainSurface(ddm.Width, ddm.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &g_pSurface, nullptr);
}
