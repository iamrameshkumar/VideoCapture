#include "stdafx.h"

#include "SampleProvider.h"
#include "GDISampleProvider.h"
#include "DesktopDuplicationSampleProvider.h"
#include "DirectXSampleProvider.h"

using namespace std;

#pragma comment (lib, "evr")
#pragma comment (lib, "mfreadwrite")
#pragma comment (lib, "mfplat")
#pragma comment (lib, "mfuuid")
#pragma comment (lib, "d3d9")
#pragma comment (lib, "d3dx9")
#pragma comment (lib, "dxguid")
#pragma comment (lib, "d3d11")

_COM_SMARTPTR_TYPEDEF(IMFSample,     __uuidof(IMFSample));
_COM_SMARTPTR_TYPEDEF(IMFMediaType,  __uuidof(IMFMediaType));
_COM_SMARTPTR_TYPEDEF(IMFSinkWriter, __uuidof(IMFSinkWriter));


// Format constants
const int VIDEO_WIDTH = GetSystemMetrics(SM_CXVIRTUALSCREEN);
const int VIDEO_HEIGHT = GetSystemMetrics(SM_CYVIRTUALSCREEN);
const UINT32 VIDEO_FPS = 30;
const UINT64 VIDEO_FRAME_DURATION = 10'000'000 / VIDEO_FPS;
const UINT32 VIDEO_BIT_RATE = 8'000'000;
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

	int64_t total_cycles;
	DirectXSampleProvider dx_provider(nullptr);
	GDISampleProvider gprovider(nullptr);
	DesktopDuplicationSampleProvider dd_provider;

#if false
	IMFSample *s1, *s2;
	IMFMediaBuffer *b1, *b2;
	provider.GetSample(&s1);
	provider.GetSample1(&s2);
	DWORD n1, n2;
	s1->GetBufferCount(&n1);
	s2->GetBufferCount(&n2);
	cout << "1st num = " << n1 << ", 2nd num = " << n2 << endl;
	s1->GetBufferByIndex(0, &b1);
	s2->GetBufferByIndex(0, &b2);
	DWORD l1, l2;
	b1->GetCurrentLength(&l1);
	b2->GetCurrentLength(&l2);
	cout << "1st len = " << l1 << ", 2nd len = " << l2 << endl;
	BYTE *data1, *data2;
	b1->Lock(&data1, nullptr, nullptr);
	b2->Lock(&data2, nullptr, nullptr);
	int i = data1[99999];
	int j = data2[99999];
	cout << "1st=" << i << ", 2nd=" << j << endl;
	return 0;
#endif
	

	{ // scope for SinkWriter lifetime
		IMFSinkWriterPtr pSinkWriter;
		DWORD streamIndex;
		hr = InitializeSinkWriter(&pSinkWriter, &streamIndex);
		if (FAILED(hr)) {
			cerr << "couldn't initialize SinkWriter" << endl;
			return 0;
		}

		total_cycles = counter.value();

		hr = Capture(dd_provider, pSinkWriter, streamIndex, VIDEO_FRAME_COUNT, VIDEO_FPS);
		if (SUCCEEDED(hr)) {
			total_cycles = counter.value() - total_cycles;
			cerr << "Finalizing the SinkWriter" << endl;
			hr = pSinkWriter->Finalize();
			if (FAILED(hr)) cerr << "Failed to finalize the SinkWriter\n";
		} else {
			cerr << "Capture() failed" << endl;
		}
	}

	MFShutdown();
	CoUninitialize();

	cout << "Total cycles elapsed for getting samples: " << cycles_elapsed_getsample << endl;
	cout << "Total cycles elapsed for writing samples: " << cycles_elapsed_write << endl;
	double total_time = double(total_cycles) / counter.get_freq();
	cout << "Total cycles elapsed for capturing:       " << total_cycles << " (" << total_time << " s.)" << endl;
	cout << "Average capturing FPS is " << VIDEO_FRAME_COUNT / total_time << endl;

	return 0;
}

