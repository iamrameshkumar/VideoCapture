#pragma once
//#include <wincon.h>
#include <mfobjects.h>
#include "SampleProvider.h"

class GDISampleProvider : SampleProvider
{
public:
	GDISampleProvider(HWND hWnd);
	~GDISampleProvider();
	HRESULT GetSample(IMFSample ** ppSample) override;
private:
	HDC hdc;
	HDC hDest;
	int width;
	int height;
	DWORD length;
	HBITMAP hbDesktop;
};
