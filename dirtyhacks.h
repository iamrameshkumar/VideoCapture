#pragma once
#include <libloaderapi.h>

HMODULE evrModule = LoadLibraryA("evr.dll");

/* MFCreateDXSurfaceBuffer prototype */
typedef HRESULT(STDAPICALLTYPE *MFCDXSB)(_In_ REFIID iid, _In_ IUnknown *unkSurface, _In_ BOOL bottomUpWhenLinera, _Out_ IMFMediaBuffer **mediaBuffer);

MFCDXSB pMFCreateDXSurfaceBuffer = (MFCDXSB)GetProcAddress(evrModule, "MFCreateDXSurfaceBuffer");