#include "stdafx.h"

#include "SampleProvider.h"
//#include "GDISampleProvider.h"
//#include "DesktopDuplicationSampleProvider.h"
#include "DirectXSampleProvider.h"
#include "NvencEncoder.h"

#include <dxva2api.h>

using namespace std;

#pragma comment (lib, "evr")
#pragma comment (lib, "mfreadwrite")
#pragma comment (lib, "mfplat")
#pragma comment (lib, "mfuuid")
#pragma comment (lib, "d3d9")
//#pragma comment (lib, "d3dx9")
#pragma comment (lib, "dxguid")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "Dxva2") 

_COM_SMARTPTR_TYPEDEF(IMFSample,     __uuidof(IMFSample));
_COM_SMARTPTR_TYPEDEF(IMFMediaType,  __uuidof(IMFMediaType));
_COM_SMARTPTR_TYPEDEF(IMFSinkWriter, __uuidof(IMFSinkWriter));


// Format constants
const int VIDEO_WIDTH = GetSystemMetrics(SM_CXVIRTUALSCREEN);
const int VIDEO_HEIGHT = GetSystemMetrics(SM_CYVIRTUALSCREEN);
const UINT32 VIDEO_FPS = 30;
const UINT64 VIDEO_FRAME_DURATION = 10000000 / VIDEO_FPS;
const UINT32 VIDEO_BIT_RATE = 8000000;
const GUID   VIDEO_ENCODING_FORMAT = MFVideoFormat_H264;
const GUID   VIDEO_INPUT_FORMAT = MFVideoFormat_ARGB32;
const UINT32 VIDEO_FRAME_COUNT = 20 * VIDEO_FPS;

struct perf_counter
{
	perf_counter() {
		QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&freq));
	}
	int64_t get_freq() const {
		return freq;
	}
	volatile int64_t value() {
		int64_t val;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&val));
		return val;
	}

private:
	int64_t freq;
} counter;

int64_t cycles_elapsed_write = 0;
int64_t cycles_elapsed_getsample = 0;

HRESULT WriteFrame(const SampleProvider & provider, IMFSinkWriter *pWriter, DWORD streamIndex, LONGLONG rtStart) {
	IMFSamplePtr pSample;
	HRESULT hr;

	{
		auto cnt_val = counter.value();
		hr = provider.GetSample(&pSample);
		cycles_elapsed_getsample += counter.value() - cnt_val;
	}

	// Set the time stamp and the duration.
	if (SUCCEEDED(hr))
		hr = pSample->SetSampleTime(rtStart);
	if (SUCCEEDED(hr))
		hr = pSample->SetSampleDuration(VIDEO_FRAME_DURATION);

	// Send the sample to the Sink Writer.
	if (SUCCEEDED(hr)) {
		auto cnt_val = counter.value();
		hr = pWriter->WriteSample(streamIndex, pSample);

		cycles_elapsed_write += counter.value() - cnt_val;

		if (FAILED(hr))
			cerr << "failed to write sample, hr = 0x" << hex << hr << endl;
	}

	return hr;
}

HRESULT InitializeSinkWriter(IMFSinkWriter **ppWriter, DWORD *pStreamIndex)
{
	*ppWriter = nullptr;
	*pStreamIndex = 0;

	IMFSinkWriterPtr pSinkWriter = nullptr;
	IMFMediaTypePtr  pMediaTypeOut = nullptr;
	IMFMediaTypePtr  pMediaTypeIn = nullptr;
	DWORD            streamIndex;

	HRESULT hr = MFCreateSinkWriterFromURL(L"output.mp4", nullptr, nullptr, &pSinkWriter);
	if (FAILED(hr))
		cout << "couldn't create SinkWriter";

	// Set the output media type.
	if (SUCCEEDED(hr))
		hr = MFCreateMediaType(&pMediaTypeOut);
	if (SUCCEEDED(hr))
		hr = pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	if (SUCCEEDED(hr))
		hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, VIDEO_ENCODING_FORMAT);
	if (SUCCEEDED(hr))
		hr = pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, VIDEO_BIT_RATE);
	if (SUCCEEDED(hr))
		hr = pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	if (SUCCEEDED(hr))
		hr = MFSetAttributeSize(pMediaTypeOut, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);
	if (SUCCEEDED(hr))
		hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_FRAME_RATE, VIDEO_FPS, 1);
	if (SUCCEEDED(hr))
		hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	if (SUCCEEDED(hr))
		hr = pSinkWriter->AddStream(pMediaTypeOut, &streamIndex);

	// Set the input media type.
	if (SUCCEEDED(hr))
		hr = MFCreateMediaType(&pMediaTypeIn);
	if (SUCCEEDED(hr))
		hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	if (SUCCEEDED(hr))
		hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, VIDEO_INPUT_FORMAT);
//	if (SUCCEEDED(hr))
//		hr = pMediaTypeIn->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE); // no throttling
	if (SUCCEEDED(hr))
		hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	if (SUCCEEDED(hr))
		hr = MFSetAttributeSize(pMediaTypeIn, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);
	if (SUCCEEDED(hr))
		hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_FRAME_RATE, VIDEO_FPS, 1);
	if (SUCCEEDED(hr))
		hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	if (SUCCEEDED(hr))
		hr = pSinkWriter->SetInputMediaType(streamIndex, pMediaTypeIn, nullptr);
	// Tell the sink writer to start accepting data.
	if (SUCCEEDED(hr))
		hr = pSinkWriter->BeginWriting();

	// Return the pointer to the caller.
	if (SUCCEEDED(hr)) {
		*ppWriter = pSinkWriter;
		(*ppWriter)->AddRef();
		*pStreamIndex = streamIndex;
	}

	return hr;
}

HRESULT Capture(const SampleProvider & provider, IMFSinkWriter * pWriter, DWORD streamIndex, unsigned count, unsigned fps) {
	HRESULT hr = S_OK;
	for (unsigned i = 0; i < count && SUCCEEDED(hr); ++i)
		hr = WriteFrame(provider, pWriter, streamIndex, VIDEO_FRAME_DURATION * i);
	return hr;
}


int main()
{
	HRESULT hr;
	COMWrapper com_wrapper;
	MFWrapper   mf_wrapper;



	ID3D11DevicePtr device;
	hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, nullptr);

	NvencEncoder nvenc_encoder(VIDEO_WIDTH, VIDEO_HEIGHT, device);
	int64_t total_cycles;
	DirectXSampleProvider dx_provider(nullptr);
	//DesktopDuplicationSampleProvider dd_provider;
	

	IDirect3DSurface9* surface;
	const D3DFORMAT D3DFMT_NV12 = (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2');
	IDirectXVideoProcessorService                       *m_pDXVA2VideoProcessServices;
	hr = DXVA2CreateVideoService(dx_provider.g_pd3dDevice, IID_PPV_ARGS(&m_pDXVA2VideoProcessServices));
	hr = m_pDXVA2VideoProcessServices->CreateSurface(VIDEO_WIDTH, VIDEO_HEIGHT, 0,
		D3DFMT_NV12, D3DPOOL_DEFAULT, 0, DXVA2_VideoProcessorRenderTarget, &surface, NULL);









	
	//hr = dx_provider.GetSample1(&surface);

	/*D3D11_TEXTURE2D_DESC desc{};
	desc.Width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	desc.Height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	desc.ArraySize = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
	desc.SampleDesc.Count = 1;

	ID3D11Texture2DPtr texture;
	hr = device->CreateTexture2D(&desc, nullptr, &texture);*/

    nvenc_encoder.write_frame(surface);

	cout << "Total cycles elapsed for getting samples: " << cycles_elapsed_getsample << endl;
	cout << "Total cycles elapsed for writing samples: " << cycles_elapsed_write << endl;
	//double total_time = double(total_cycles) / counter.get_freq();
	//cout << "Total cycles elapsed for capturing:       " << total_cycles << " (" << total_time << " s.)" << endl;
	//cout << "Average capturing FPS is " << VIDEO_FRAME_COUNT / total_time << endl;

	return 0;
}

