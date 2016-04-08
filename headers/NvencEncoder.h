#pragma once

#include "stdafx.h"
#include "nvEncodeAPI.h"
#include <DXGI1_2.h>


class NvencEncoder
{
public:
	NvencEncoder(uint32_t width, uint32_t height);
	~NvencEncoder();
	NVENCSTATUS write_frame(IDXGISurface &frame);
private:
	HMODULE nvenc_lib_ = nullptr;
	NV_ENCODE_API_FUNCTION_LIST nvenc_;
	NV_ENC_OUTPUT_PTR output_buffer_;
	void * encoder_;
	ID3D11DevicePtr device_;
};