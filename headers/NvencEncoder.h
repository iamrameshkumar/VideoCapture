#pragma once

#include "stdafx.h"
#include "nvEncodeAPI.h"
#include <DXGI1_2.h>
#include <D3D11.h>
_COM_SMARTPTR_TYPEDEF(ID3D11Texture2D, __uuidof(ID3D11Texture2D));


class NvencEncoder
{
public:
	NvencEncoder(uint32_t width, uint32_t height, ID3D11DevicePtr device);
	~NvencEncoder();
	NVENCSTATUS write_frame(IDirect3DSurface9* texture, std::ostream &out = std::cout);
private:
	ID3D11DevicePtr device_;
	HMODULE nvenc_lib_ = nullptr;
	NV_ENCODE_API_FUNCTION_LIST nvenc_;
	NV_ENC_OUTPUT_PTR output_buffer_;
	void * encoder_;
};