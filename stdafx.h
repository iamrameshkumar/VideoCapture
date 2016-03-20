#pragma once
#include <iostream>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <comip.h>
#include <comdef.h>

#include <d3d9.h>
#include <d3dx9tex.h>
#include <evr.h>


_COM_SMARTPTR_TYPEDEF(IMFMediaBuffer, __uuidof(IMFMediaBuffer));
_COM_SMARTPTR_TYPEDEF(IMF2DBuffer, __uuidof(IMF2DBuffer));


struct MFWrapper {
	MFWrapper() {
		if (FAILED(MFStartup(MF_VERSION)))
			throw runtime_error("Cannot MFStartup");
	}
	~MFWrapper() { MFShutdown(); }
};

struct COMWrapper {
	COMWrapper() {
		if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
			throw runtime_error("Cannot CoInitializeEx");
	}
	~COMWrapper() { CoUninitialize(); }
};