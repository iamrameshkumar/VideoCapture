#include "stdafx.h"

#include "DirectXSampleProvider.h"

static const int VIDEO_WIDTH = GetSystemMetrics(SM_CXVIRTUALSCREEN);
static const int VIDEO_HEIGHT = GetSystemMetrics(SM_CYVIRTUALSCREEN);
static const int cbWidth = VIDEO_WIDTH * 4;


HRESULT DirectXSampleProvider::GetSample(IMFSample ** ppSample) const
{
	HRESULT hr = g_pd3dDevice->GetFrontBufferData(0, g_pSurface);
	if (FAILED(hr))
		std::cout << "couldn't get front buffer data" << std::endl;

	IMFMediaBufferPtr dbuf;
	IMF2DBufferPtr p2dBuf;
	BYTE * pPixels = nullptr;
	DWORD length;
	//if (SUCCEEDED(hr))
		//hr = MFCreateDXSurfaceBuffer(IID_IDirect3DSurface9, g_pSurface, false, &dbuf);
	if (SUCCEEDED(hr)) {
		p2dBuf = dbuf;
		hr = p2dBuf->GetContiguousLength(&length);
	}
	
	IMFMediaBufferPtr pBuffer;
	if (SUCCEEDED(hr))
		MFCreateMemoryBuffer(length, &pBuffer);

	LONG pitch;
	if (SUCCEEDED(hr))
		hr = p2dBuf->Lock2D(&pPixels, &pitch);
	BYTE *pData;
	// Lock the buffer and copy the video frame to the buffer.
	if (SUCCEEDED(hr))
		hr = pBuffer->Lock(&pData, nullptr, nullptr);
	if (SUCCEEDED(hr)) {
		hr = MFCopyImage(
			pData,           // Destination buffer.
			cbWidth,         // Destination stride.
			pPixels,         // First row in source image.
			cbWidth,         // Source stride.
			cbWidth,         // Image width in bytes.
			VIDEO_HEIGHT     // Image height in pixels.
			);
	}
	pBuffer->Unlock();
	p2dBuf->Unlock2D();
	// Set the data length of the buffer.
	if (SUCCEEDED(hr))
		hr = pBuffer->SetCurrentLength(cbWidth * VIDEO_HEIGHT);

	// Create a media sample and add the buffer to the sample.
	if (SUCCEEDED(hr))
		hr = MFCreateVideoSampleFromSurface(nullptr, ppSample);
	if (FAILED(hr))
		std::cout << "getSample failed" << std::endl;
	if (SUCCEEDED(hr))
		hr = (*ppSample)->AddBuffer(pBuffer); // pBuffer
	return hr;
}


HRESULT DirectXSampleProvider::GetSample1(IDirect3DSurface9 ** ppSurface) {
	//	return GetSample1(ppSample);
	HRESULT hr = g_pd3dDevice->GetFrontBufferData(0, g_pSurface);
	assert(SUCCEEDED(hr));
	*ppSurface = g_pSurface;
	return hr;
}

DirectXSampleProvider::DirectXSampleProvider(HWND hWnd)
	: g_pD3D(Direct3DCreate9(D3D_SDK_VERSION))
{
	if (!g_pD3D)
		throw std::runtime_error("Could not create Direct3D9");

	g_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &ddm);

	D3DPRESENT_PARAMETERS	d3dpp{ 0 };
	d3dpp.Windowed = true;
	d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferHeight = 480;
	d3dpp.BackBufferWidth = 640;
	d3dpp.BackBufferCount = 1;
//	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
//	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	HRESULT hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT
		, D3DDEVTYPE_HAL
		, hWnd
		, D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING
		, &d3dpp
		, &g_pd3dDevice);

	if (FAILED(hr))	{
		std::cerr << "hr = " << hr << std::endl;
		throw std::runtime_error("could not create D3DDevice");
	}

	hr = g_pd3dDevice->CreateOffscreenPlainSurface(ddm.Width, ddm.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &g_pSurface, nullptr);
}
