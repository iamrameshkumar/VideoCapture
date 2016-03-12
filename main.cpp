#include <iostream>

#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfapi.h>

#include "ScreenSampleProvider.h"
#include "GDISampleProvider.h"

using namespace std;

#pragma comment (lib, "evr")
#pragma comment (lib, "mfreadwrite")
#pragma comment (lib, "mfplat")
#pragma comment (lib, "mfuuid")
#pragma comment (lib, "d3d9")
#pragma comment (lib, "d3dx9")
#pragma comment (lib, "dxguid")


// Format constants
static const int VIDEO_WIDTH = GetSystemMetrics(SM_CXVIRTUALSCREEN);
static const int VIDEO_HEIGHT = GetSystemMetrics(SM_CYVIRTUALSCREEN);
const UINT32 VIDEO_FPS = 30;
const UINT64 VIDEO_FRAME_DURATION = 10'000'000 / VIDEO_FPS;
const UINT32 VIDEO_BIT_RATE = 8'000'000;
const GUID   VIDEO_ENCODING_FORMAT = MFVideoFormat_H264;
const GUID   VIDEO_INPUT_FORMAT = MFVideoFormat_RGB32;
const UINT32 VIDEO_FRAME_COUNT = 20 * VIDEO_FPS;

ScreenSampleProvider provider(nullptr);
GDISampleProvider gprovider(nullptr);


struct perf_counter
{
	perf_counter() {
		QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&freq));
	}
	int64_t get_freq() const {
		return freq;
	}
	int64_t value() {
		int64_t val;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&val));
		return val;
	}

private:
	int64_t freq;
} counter;

int64_t cycles_elapsed_write = 0;
int64_t cycles_elapsed_getsample = 0;

HRESULT WriteFrame(
	IMFSinkWriter *pWriter,
	DWORD streamIndex,
	LONGLONG rtStart        // Time stamp.
	)
{
	CComPtr<IMFSample> pSample;
	HRESULT hr;
	{
		auto cnt_val = counter.value();
		hr = gprovider.GetSample(&pSample);
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
	}

	return hr;
}

HRESULT InitializeSinkWriter(IMFSinkWriter **ppWriter, DWORD *pStreamIndex)
{
	*ppWriter = nullptr;
	*pStreamIndex = 0;

	CComPtr<IMFSinkWriter> pSinkWriter = nullptr;
	CComPtr<IMFMediaType>  pMediaTypeOut = nullptr;
	CComPtr<IMFMediaType>  pMediaTypeIn = nullptr;
	DWORD                  streamIndex;

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


int main()
{
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
		throw runtime_error("couldn't CoInitializeEx");

	hr = MFStartup(MF_VERSION);
	if (FAILED(hr)) 
		throw runtime_error("couldn't MFStartup");
	int64_t total_cycles;

	{ // scope for SinkWriter lifetime
		CComPtr<IMFSinkWriter> pSinkWriter;
		DWORD streamIndex;
		hr = InitializeSinkWriter(&pSinkWriter, &streamIndex);
		if (FAILED(hr)) {
			cerr << "couldn't initialize SinkWriter" << endl;
			return 0;
		}
		total_cycles = counter.value();
		for (size_t i = 0; i < VIDEO_FRAME_COUNT; ++i) {
//			cerr << "i=" << i << '\n';
			hr = WriteFrame(pSinkWriter, streamIndex, VIDEO_FRAME_DURATION * i);
			if (FAILED(hr)) {
				cerr << "couldn't write sample" << endl;
				break;
			}
		}
		total_cycles = counter.value() - total_cycles;
		cerr << "Finalizing the SinkWriter" << endl;
		pSinkWriter->Finalize();
	}
	MFShutdown();
	CoUninitialize();
	cout << "Total cycles elapsed for getting samples: " << cycles_elapsed_getsample << endl;
	cout << "Total cycles elapsed for writing samples: " << cycles_elapsed_write << endl;
	double total_time = (double)total_cycles / counter.get_freq();
	cout << "Total cycles elapsed for capturing:       " << total_cycles << " (" << total_time << " s.)" << endl;
	cout << "Average capturing FPS is " << VIDEO_FRAME_COUNT / total_time << endl;
}

