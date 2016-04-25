#include "stdafx.h"
#include "NvencEncoder.h"
#include <vector>

#if defined(_WIN64)
static const LPCTSTR encodeLibName = TEXT("nvEncodeAPI64.dll");
#elif defined(_WIN32)
static const LPCTSTR encodeLibName = TEXT("nvEncodeAPI.dll");
#endif

typedef NVENCSTATUS(NVENCAPI * 	PNVENCODEAPICREATEINSTANCE)(NV_ENCODE_API_FUNCTION_LIST *functionList);

NvencEncoder::NvencEncoder(uint32_t width, uint32_t height)
	: nvenc_lib_{ LoadLibrary(encodeLibName) }
{
	D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device_, nullptr, nullptr);
	if (!nvenc_lib_)
		throw std::runtime_error((std::string("cannot load ") + (char*)encodeLibName));

	PNVENCODEAPICREATEINSTANCE NvEncodeAPICreateInstance =
		(PNVENCODEAPICREATEINSTANCE)GetProcAddress(nvenc_lib_, "NvEncodeAPICreateInstance" );
	if (!NvEncodeAPICreateInstance)
		throw std::runtime_error("cannot load \"NvEncodeAPICreateInstance\" function");
    nvenc_.version = NV_ENCODE_API_FUNCTION_LIST_VER;
	NVENCSTATUS status = NvEncodeAPICreateInstance(&nvenc_);

	if (status == NV_ENC_SUCCESS) {
		NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params {};
		params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
		params.device = device_;
		params.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;

		status = nvenc_.nvEncOpenEncodeSessionEx(&params, &encoder_);
	}

	uint32_t count = 0;
	if (status == NV_ENC_SUCCESS)
		status = nvenc_.nvEncGetEncodeGUIDCount(encoder_, &count);
	std::vector<GUID> guids(count);
	if (status == NV_ENC_SUCCESS) {
		status = nvenc_.nvEncGetEncodeGUIDs(encoder_, guids.data(), count, &count);
	} // TODO test capabilities before using them
    if (std::find(guids.begin(), guids.end(), NV_ENC_CODEC_H264_GUID) == guids.end())
        throw std::runtime_error("H264 codec not supported by the GPU");

	{
		NV_ENC_CONFIG_H264 h264_config{};
        h264_config.idrPeriod = 2;
		NV_ENC_CONFIG config{};
        config.version = NV_ENC_CONFIG_VER;
		config.gopLength = 10;
		config.frameIntervalP = 1;
		config.encodeCodecConfig.h264Config = h264_config;
		NV_ENC_INITIALIZE_PARAMS params{};
        params.version = NV_ENC_INITIALIZE_PARAMS_VER;
		params.encodeGUID = NV_ENC_CODEC_H264_GUID;
		//params.encodeConfig = &config;
		params.encodeWidth = width;
		params.encodeHeight = height;
        params.presetGUID = NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID;
		params.enablePTD = 1; // Picture Type Decision
		status = nvenc_.nvEncInitializeEncoder(encoder_, &params);
	}

	{
		NV_ENC_CREATE_BITSTREAM_BUFFER params{};
		params.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
		params.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;
		params.size = width * height * 4;
		status = nvenc_.nvEncCreateBitstreamBuffer(encoder_, &params);
        output_buffer_ = params.bitstreamBuffer;
	}

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    desc.Height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    desc.ArraySize = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;

    HRESULT hr = device_->CreateTexture2D(&desc, nullptr, &texture_);
}

NvencEncoder::~NvencEncoder()
{
    NVENCSTATUS status;
	if (nvenc_lib_) {
		status = nvenc_.nvEncEncodePicture(encoder_, nullptr);
		status = nvenc_.nvEncDestroyBitstreamBuffer(encoder_, output_buffer_);
		status = nvenc_.nvEncDestroyEncoder(encoder_);
		FreeLibrary(nvenc_lib_);
	}
}

NVENCSTATUS NvencEncoder::write_frame(std::ostream &out)
{
	NV_ENC_REGISTER_RESOURCE res_params{};
	res_params.version = NV_ENC_REGISTER_RESOURCE_VER;

	NVENCSTATUS status = nvenc_.nvEncRegisterResource(encoder_, &res_params);

	NV_ENC_MAP_INPUT_RESOURCE map_params{};
	map_params.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
	map_params.inputResource = (void*)texture_;
	status = nvenc_.nvEncMapInputResource(encoder_, &map_params);

	D3D11_TEXTURE2D_DESC desc{};
	texture_->GetDesc(&desc);
	// encode it
	NV_ENC_PIC_PARAMS params{};
	params.version = NV_ENC_PIC_PARAMS_VER;
	params.inputWidth = desc.Width;
	params.inputHeight = desc.Height;
	params.inputPitch = params.inputWidth;
	params.inputBuffer = nullptr;
	params.outputBitstream = output_buffer_;
	nvenc_.nvEncEncodePicture(encoder_, &params);
	
	nvenc_.nvEncUnmapInputResource(encoder_, map_params.mappedResource);
	nvenc_.nvEncUnregisterResource(encoder_, res_params.registeredResource);

	NV_ENC_LOCK_BITSTREAM lock_params{};
	lock_params.version = NV_ENC_LOCK_BITSTREAM_VER;
	lock_params.outputBitstream = output_buffer_;

	nvenc_.nvEncLockBitstream(encoder_, &lock_params);
    out.write((char*)lock_params.bitstreamBufferPtr, lock_params.bitstreamSizeInBytes);
	
	nvenc_.nvEncUnlockBitstream(encoder_, &output_buffer_);


	return NV_ENC_SUCCESS;
}
