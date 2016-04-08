#include "stdafx.h"
/*
#include "DesktopDuplicationSampleProvider.h"

_COM_SMARTPTR_TYPEDEF(IDXGIDevice,   __uuidof(IDXGIDevice));
_COM_SMARTPTR_TYPEDEF(IDXGIAdapter,  __uuidof(IDXGIAdapter));
_COM_SMARTPTR_TYPEDEF(IDXGIOutput,   __uuidof(IDXGIOutput));
_COM_SMARTPTR_TYPEDEF(IDXGIOutput1,  __uuidof(IDXGIOutput1));
_COM_SMARTPTR_TYPEDEF(IDXGISurface,  __uuidof(IDXGISurface));



DesktopDuplicationSampleProvider::DesktopDuplicationSampleProvider() {
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device_, nullptr, nullptr);

	IDXGIDevicePtr dxgi_device = device_;
	IDXGIAdapterPtr dxgi_adapter;

	if (SUCCEEDED(hr))
		hr = dxgi_device->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgi_adapter));
	IDXGIOutputPtr dxgi_output;
	if (SUCCEEDED(hr))
		hr = dxgi_adapter->EnumOutputs(0, &dxgi_output);
	IDXGIOutput1Ptr dxgi_output1 = dxgi_output;
	if (SUCCEEDED(hr))
		hr = dxgi_output1->DuplicateOutput(device_, &duplication_);
	if (SUCCEEDED(hr))
		duplication_->GetDesc(&duplication_desc_);
	D3D11_TEXTURE2D_DESC texture_desc;
	texture_desc.Width = duplication_desc_.ModeDesc.Width;
	texture_desc.Height = duplication_desc_.ModeDesc.Height;
	texture_desc.MiscFlags = 0;
	texture_desc.BindFlags = 0;
	texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texture_desc.MipLevels = 1;
	texture_desc.ArraySize = 1;
	texture_desc.SampleDesc.Count = 1;
	texture_desc.SampleDesc.Quality = 0;
	texture_desc.Usage = D3D11_USAGE_STAGING;
	if (SUCCEEDED(hr))
		hr = device_->CreateTexture2D(&texture_desc, nullptr, &texture2D_);
	if (SUCCEEDED(hr))
		hr = MFCreateMemoryBuffer(texture_desc.Width * texture_desc.Height * 4, &buffer_);
	if (SUCCEEDED(hr))
		hr = buffer_->SetCurrentLength(texture_desc.Width * texture_desc.Height * 4);
	if (SUCCEEDED(hr))
		device_->GetImmediateContext(&сontext_);
	if (FAILED(hr))
		throw std::runtime_error("could not construct DesktopDuplicationSampleProvider");
}


DesktopDuplicationSampleProvider::~DesktopDuplicationSampleProvider() {
	if (duplication_)
		duplication_->ReleaseFrame();
}

HRESULT DesktopDuplicationSampleProvider::GetSample(IMFSample** ppSample) const
{
	static bool prev = false;
	HRESULT hr = S_OK;
	if (prev)
		hr = duplication_->ReleaseFrame();
	prev = false;
	IDXGIResourcePtr pDesktopResource;
	DXGI_OUTDUPL_FRAME_INFO frameInfo;
	if (SUCCEEDED(hr)) {
		hr = duplication_->AcquireNextFrame(INFINITE, &frameInfo, &pDesktopResource);
		prev = true;
	}
	
	ID3D11Texture2DPtr pAcquiredDesktopImage;
	if (SUCCEEDED(hr))
		pAcquiredDesktopImage = pDesktopResource;
	
	сontext_->CopyResource(texture2D_, pAcquiredDesktopImage);
	if (SUCCEEDED(hr))
		hr = MFCreateVideoSampleFromSurface(nullptr, ppSample);

	IMFMediaBufferPtr media_buffer;
	if (SUCCEEDED(hr))
		hr = MFCreateDXGISurfaceBuffer(IID_ID3D11Texture2D, texture2D_, 0, false, &media_buffer);

	IMF2DBufferPtr buffer2D;
	if (SUCCEEDED(hr))
		buffer2D = media_buffer;

	if (SUCCEEDED(hr)) {
		BYTE * pPixels;
		LONG pitch;
		hr = buffer2D->Lock2D(&pPixels, &pitch);

		BYTE *pData;
		// Lock the buffer and copy the video frame to the buffer.
		if (SUCCEEDED(hr))
			hr = buffer_->Lock(&pData, nullptr, nullptr);
		if (SUCCEEDED(hr)) {
			hr = MFCopyImage(
				pData,                                // Destination buffer.
				duplication_desc_.ModeDesc.Width * 4, // Destination stride.
				pPixels,                              // First row in source image.
				pitch,                                // Source stride.
				duplication_desc_.ModeDesc.Width * 4, // Image width in bytes.
				duplication_desc_.ModeDesc.Height     // Image height in pixels.
				);
			buffer_->Unlock();
		}
		buffer2D->Unlock2D();
	}
	if (SUCCEEDED(hr))
		hr = (*ppSample)->AddBuffer(buffer_);
	return hr;
}
*/