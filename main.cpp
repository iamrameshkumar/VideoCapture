#include <iostream>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfapi.h>
#include "ScreenSampleProvider.h"
#include <common.h>
#include "dirtyhacks.h"
using namespace std;

#pragma comment (lib, "evr")
#pragma comment (lib, "mfreadwrite")
#pragma comment (lib, "mfplat")
#pragma comment (lib, "mfuuid")
#pragma comment (lib, "d3d9")
#pragma comment (lib, "d3dx9")
#pragma comment (lib, "dxguid")


// Format constants
const UINT32 VIDEO_WIDTH = 1600;
const UINT32 VIDEO_HEIGHT = 900;
const UINT32 VIDEO_FPS = 30;
const UINT64 VIDEO_FRAME_DURATION = 10 * 1000 * 1000 / VIDEO_FPS;
const UINT32 VIDEO_BIT_RATE = 8'000'000;
const GUID   VIDEO_ENCODING_FORMAT = MFVideoFormat_H264;
const GUID   VIDEO_INPUT_FORMAT = MFVideoFormat_RGB32;
const UINT32 VIDEO_PELS = VIDEO_WIDTH * VIDEO_HEIGHT;
const UINT32 VIDEO_FRAME_COUNT = 20 * VIDEO_FPS;

ScreenSampleProvider provider(nullptr);

DWORD videoFrameBuffer[VIDEO_PELS];
HRESULT WriteFrame(
	IMFSinkWriter *pWriter,
	DWORD streamIndex,
	const LONGLONG& rtStart        // Time stamp.
	)
{
	IMFSample *pSample = NULL;
	IMFMediaBuffer *pBuffer = NULL;

	const LONG cbWidth = 4 * VIDEO_WIDTH;
	const DWORD cbBuffer = cbWidth * VIDEO_HEIGHT;

	BYTE *pData = NULL;

	// Create a new memory buffer.

	HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);
	provider.getSample(nullptr);
	IMFMediaBuffer * dbuf;
//	hr = pMFCreateDXSurfaceBuffer(IID_IDirect3DSurface9, provider.g_pSurface, false, &dbuf); // kill me please
	D3DLOCKED_RECT rect;
	provider.g_pSurface->LockRect(&rect, nullptr, D3DLOCK_READONLY);

	// Lock the buffer and copy the video frame to the buffer.
	if (SUCCEEDED(hr))
	{
		hr = pBuffer->Lock(&pData, NULL, NULL);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFCopyImage(
			pData,                      // Destination buffer.
			cbWidth,                    // Destination stride.
			(BYTE*)rect.pBits,    // First row in source image.
			cbWidth,                    // Source stride.
			cbWidth,                    // Image width in bytes.
			VIDEO_HEIGHT                // Image height in pixels.
			);
	}
	if (pBuffer)
	{
		pBuffer->Unlock();
	}
	provider.g_pSurface->UnlockRect();

	// Set the data length of the buffer.
	if (SUCCEEDED(hr))
	{
		hr = pBuffer->SetCurrentLength(cbBuffer);
	}

	// Create a media sample and add the buffer to the sample.
	if (SUCCEEDED(hr))
	{
//		hr = provider.getSample(&pSample);
		hr = MFCreateSample(&pSample);
	}
	if (FAILED(hr))
	{
		cout << "getSample failed" << endl;
	}
	if (SUCCEEDED(hr))
	{
//		IMFMediaBuffer * buf;
//		DWORD cnt;
//		hr = pSample->GetBufferByIndex(0, &buf);
//		hr = buf->GetCurrentLength(&cnt);
//		cout << "buf len = " << cnt << endl;
//		cout << "cbBuffer = " << cbBuffer << endl;
		hr = pSample->AddBuffer(pBuffer);
	}

	// Set the time stamp and the duration.
	if (SUCCEEDED(hr))
	{
		hr = pSample->SetSampleTime(rtStart);
	}
	if (SUCCEEDED(hr))
	{
		hr = pSample->SetSampleDuration(VIDEO_FRAME_DURATION);
	}

	// Send the sample to the Sink Writer.
	if (SUCCEEDED(hr))
	{
		hr = pWriter->WriteSample(streamIndex, pSample);
	}

	SAFE_RELEASE(pSample);
	SAFE_RELEASE(pBuffer);
	return hr;
}

HRESULT InitializeSinkWriter(IMFSinkWriter **ppWriter, DWORD *pStreamIndex)
{
	*ppWriter = nullptr;
	*pStreamIndex = 0;

	IMFSinkWriter   *pSinkWriter = nullptr;
	IMFMediaType    *pMediaTypeOut = nullptr;
	IMFMediaType    *pMediaTypeIn = nullptr;
	DWORD           streamIndex;

	HRESULT hr = MFCreateSinkWriterFromURL(L"output.mp4", nullptr, nullptr, &pSinkWriter);
	if (FAILED(hr))
	{
		cout << "couldn't create SinkWriter";
	}

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
	if (SUCCEEDED(hr))
		hr = pMediaTypeIn->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE); // no throttling
	if (SUCCEEDED(hr))
		hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	if (SUCCEEDED(hr))
		hr = MFSetAttributeSize(pMediaTypeIn, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);
	if (SUCCEEDED(hr))
		hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_FRAME_RATE, VIDEO_FPS, 1);
	if (SUCCEEDED(hr))
		hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	if (SUCCEEDED(hr))
		hr = pSinkWriter->SetInputMediaType(streamIndex, pMediaTypeIn, NULL);
	// Tell the sink writer to start accepting data.
	if (SUCCEEDED(hr))
	{
		hr = pSinkWriter->BeginWriting();
	}

	// Return the pointer to the caller.
	if (SUCCEEDED(hr))
	{
		*ppWriter = pSinkWriter;
		(*ppWriter)->AddRef();
		*pStreamIndex = streamIndex;
	}

	SAFE_RELEASE(pSinkWriter);
	SAFE_RELEASE(pMediaTypeOut);
	SAFE_RELEASE(pMediaTypeIn);
	return hr;
}


int main()
{
	for (int i = 0; i < VIDEO_HEIGHT; ++i)
	{
		for (int j = 0; j < VIDEO_WIDTH; ++j) {
			videoFrameBuffer[i * VIDEO_WIDTH + j] = i*i - j*j;
		}
	}

	IMFSinkWriter * pSinkWriter;
	DWORD streamIndex;
	HRESULT hr;
	hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		cerr << "couldn't CoInitialize" << endl;
		return 0;
	}
	hr = MFStartup(MF_VERSION);
	if (FAILED(hr)) {
		cerr << "couldn't MFStartup" << endl;
		return 0;
	}
	hr = InitializeSinkWriter(&pSinkWriter, &streamIndex);
	if (FAILED(hr)) {
		cerr << "couldn't initialize SinkWriter" << endl;
		return 0;
	}

	for (int i = 0; i < VIDEO_FRAME_COUNT; ++i) {
		cout << "i=" << i << endl;
//		IMFSample * pSample;
//		HRESULT hr = provider.getSample(&pSample);
		if (FAILED(hr))	{
			cout << "Couldn't get sample";
			break;
		}
		hr = WriteFrame(pSinkWriter, streamIndex, VIDEO_FRAME_DURATION * i);
		if (FAILED(hr))	{
			cout << "couldn't write sample" << endl;
			break;
		}
	}
	pSinkWriter->Finalize();
	SAFE_RELEASE(pSinkWriter);
	MFShutdown();
	CoUninitialize();

	cout << "Hello, world!" << endl;
}
