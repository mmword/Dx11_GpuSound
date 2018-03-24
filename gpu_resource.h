#pragma once

#define _WIN32_WINNT 0x600
#include <stdio.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <memory>

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")

class GPUDevice;
class GPUProgram;
class GPUBuffer;
template<typename T>
class GPUConstantBuffer;

class GPUDevice
{
	ID3D11DeviceContext *context = nullptr;
	ID3D11Device* device = nullptr;
	BOOL _success = FALSE;
public:

	GPUDevice();

	~GPUDevice();

	inline BOOL IsSuccess() const;

	inline ID3D11DeviceContext *GetContext() const;

	inline ID3D11Device *GetDevice() const;

	void BindProgram(const GPUProgram *program);

	void SetBuffer(const GPUBuffer *buffer);

	void ResetRersources();

	template<typename T>
	void SetConstantBuffer(const GPUConstantBuffer<T> *buffer);

	void Dispatch(int blocks);

	void Release();
};

class GPUProgram
{
	const GPUDevice *_device = nullptr;
	ID3D11ComputeShader* shader = nullptr;
	BOOL _success = FALSE;
	WCHAR _file[MAX_PATH] = { 0 };
	CHAR _entryPoint[128] = { 0 };

	HRESULT CompileComputeShader(_In_ LPCWSTR srcFile, _In_ LPCSTR entryPoint,
		_In_ ID3D11Device* device, _Outptr_ ID3DBlob** blob)
	{
		if (!srcFile || !entryPoint || !device || !blob)
			return E_INVALIDARG;

		*blob = nullptr;

		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
		flags |= D3DCOMPILE_DEBUG;
#endif

		// We generally prefer to use the higher CS shader profile when possible as CS 5.0 is better performance on 11-class hardware
		LPCSTR profile = (device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "cs_5_0" : "cs_4_0";

		const D3D_SHADER_MACRO defines[] =
		{
			"EXAMPLE_DEFINE", "1",
			NULL, NULL
		};

		ID3DBlob* shaderBlob = nullptr;
		ID3DBlob* errorBlob = nullptr;
		HRESULT hr = D3DCompileFromFile(srcFile, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entryPoint, profile,
			flags, 0, &shaderBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob)
			{
				//OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				printf((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}

			if (shaderBlob)
				shaderBlob->Release();

			return hr;
		}

		*blob = shaderBlob;

		return hr;
	}

public:

	GPUProgram(const GPUDevice &device, LPCWSTR file, LPSTR entryPoint,BOOL compile);

	~GPUProgram();

	inline ID3D11ComputeShader *GetProgram() const;

	inline BOOL IsSuccess() const;

    BOOL ReCompile();

	void Release();
};

class GPUBuffer
{
	const GPUDevice *_device = nullptr;
	ID3D11UnorderedAccessView *uav = nullptr;
	ID3D11Buffer *gpu_buffer = nullptr;
	BOOL _success = FALSE;
	UINT BufferSizeInBytes,Stride;

public:

	GPUBuffer(const GPUDevice &device, UINT SizeInBytes, UINT ByteStride);

	~GPUBuffer();

	inline ID3D11UnorderedAccessView *GetUAV() const;

	inline ID3D11Buffer *GetBuffer() const;

	inline BOOL IsSuccess() const;

	BOOL CopyToHost(BYTE *dst);

	void Release();
};

template<typename T>
class GPUConstantBuffer
{
	const GPUDevice *_device = nullptr;
	ID3D11UnorderedAccessView *uav = nullptr;
	ID3D11Buffer *gpu_buffer = nullptr;
	BOOL _success = FALSE;
	UINT Stride = 0;

public:

	GPUConstantBuffer(const GPUDevice &device, const T &&initData);

	BOOL Set(const T &data);

	inline ID3D11Buffer* GetBuffer() const;

	inline BOOL IsSuccess() const;
};

// GPUDEvice --------------------------------------------------------------------------------------------------

GPUDevice::GPUDevice()
{
	const D3D_FEATURE_LEVEL lvl[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, lvl, _countof(lvl),
		D3D11_SDK_VERSION, &device, nullptr, &context);
	if (hr == E_INVALIDARG)
	{
		// DirectX 11.0 Runtime doesn't recognize D3D_FEATURE_LEVEL_11_1 as a valid value
		hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &lvl[1], _countof(lvl) - 1,
			D3D11_SDK_VERSION, &device, nullptr, &context);
	}

	// Verify compute shader is supported
	if (device->GetFeatureLevel() < D3D_FEATURE_LEVEL_11_0)
	{
		Release();
		printf("not supported by this device\n");
		return;
	}
	_success = SUCCEEDED(hr);
}

GPUDevice::~GPUDevice()
{
	Release();
}

inline BOOL GPUDevice::IsSuccess() const
{
	return _success;
}

inline ID3D11DeviceContext *GPUDevice::GetContext() const
{
	return context;
}

inline ID3D11Device *GPUDevice::GetDevice() const
{
	return device;
}

void GPUDevice::ResetRersources()
{
	ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
	context->CSSetUnorderedAccessViews(0, 1, ppUAViewNULL, NULL);

	ID3D11Buffer *_buffer[1] = { NULL };
	context->CSSetConstantBuffers(0, 1, _buffer);

	context->CSSetShader(NULL, NULL, 0);
}

void GPUDevice::BindProgram(const GPUProgram *program)
{
	if (program != nullptr)
		context->CSSetShader(program->GetProgram(), NULL, 0);
	else
		context->CSSetShader(NULL, NULL, 0);
}

void GPUDevice::SetBuffer(const GPUBuffer *buffer)
{
	if (buffer != nullptr)
	{
		ID3D11UnorderedAccessView* ppUAView[1] = { buffer->GetUAV() };
		context->CSSetUnorderedAccessViews(0, 1, ppUAView, NULL);
	}
	else
	{
		ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
		context->CSSetUnorderedAccessViews(0, 1, ppUAViewNULL, NULL);
	}
}

template<typename T>
void GPUDevice::SetConstantBuffer(const GPUConstantBuffer<T> *buffer)
{
	if (buffer != nullptr)
	{
		ID3D11Buffer *_buffer[1] = { buffer->GetBuffer() };
		context->CSSetConstantBuffers(0, 1, _buffer);
	}
	else
	{
		ID3D11Buffer *_buffer[1] = { NULL };
		context->CSSetConstantBuffers(0, 1, _buffer);
	}
}

void GPUDevice::Dispatch(int blocks)
{
	context->Dispatch(blocks, 1, 1);
}

void GPUDevice::Release()
{
	if (context != nullptr)
	{
		context->Release();
		context = nullptr;
	}
	if (device != nullptr)
	{
		device->Release();
		device = nullptr;
	}
	_success = FALSE;
}

// GPUProgram -----------------------------------------------------------------------------

GPUProgram::GPUProgram(const GPUDevice &device, LPCWSTR file, LPSTR entryPoint,BOOL compile)
{
	if (device.IsSuccess() && file && entryPoint)
	{
		_device = &device;
		wcscpy_s(_file, file);
		strcpy_s(_entryPoint, entryPoint);
		if(compile)
			ReCompile();
	}
}

GPUProgram::~GPUProgram()
{
	Release();
}

inline ID3D11ComputeShader *GPUProgram::GetProgram() const
{
	return shader;
}

inline BOOL GPUProgram::IsSuccess() const
{
	return _success;
}

BOOL GPUProgram::ReCompile()
{
	// Release oldest resource
	Release();
	if (_device && _device->IsSuccess())
	{
		// Compile shader
		ID3DBlob *csBlob = nullptr;
		HRESULT hr = CompileComputeShader(_file, _entryPoint, _device->GetDevice(), &csBlob);
		if (FAILED(hr))
			printf("Failed compiling shader %08X\n", hr);
		else
			_success = SUCCEEDED(_device->GetDevice()->CreateComputeShader(csBlob->GetBufferPointer(), csBlob->GetBufferSize(), nullptr, &shader));
		if (csBlob != nullptr)
			csBlob->Release();
	}
	return _success;
}

void GPUProgram::Release()
{
	if (shader != nullptr)
	{
		shader->Release();
		shader = nullptr;
		_success = FALSE;
	}
}

// GPUBuffer --------------------------------------------------------------------------------

GPUBuffer::GPUBuffer(const GPUDevice &device, UINT SizeInBytes, UINT ByteStride)
{
	if (device.IsSuccess())
	{
		_device = &device;
		BufferSizeInBytes = SizeInBytes;
		Stride = ByteStride;
		// The compute shader will need to output to some buffer so here 
		// we create a GPU buffer for that.
		D3D11_BUFFER_DESC descGPUBuffer;
		ZeroMemory(&descGPUBuffer, sizeof(descGPUBuffer));
		descGPUBuffer.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		descGPUBuffer.ByteWidth = SizeInBytes;
		descGPUBuffer.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		descGPUBuffer.StructureByteStride = ByteStride;


		if (SUCCEEDED(device.GetDevice()->CreateBuffer(&descGPUBuffer, NULL, &gpu_buffer)))
		{
			// Now we create a view on the resource. DX11 requires you to send the data
			// to shaders using a "shader view"
			D3D11_BUFFER_DESC descBuf;
			ZeroMemory(&descBuf, sizeof(descBuf));
			gpu_buffer->GetDesc(&descBuf);

			D3D11_UNORDERED_ACCESS_VIEW_DESC descView;
			ZeroMemory(&descView, sizeof(descView));
			descView.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			descView.Buffer.FirstElement = 0;
			// Format must be must be DXGI_FORMAT_UNKNOWN, when creating 
			// a View of a Structured Buffer
			descView.Format = DXGI_FORMAT_UNKNOWN;
			descView.Buffer.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;

			if (FAILED(device.GetDevice()->CreateUnorderedAccessView(gpu_buffer,
				&descView, &uav)))
			{
				gpu_buffer->Release();
				return;
			}
			_success = TRUE;
		}
	}
}

GPUBuffer::~GPUBuffer()
{
	Release();
}

inline ID3D11UnorderedAccessView *GPUBuffer::GetUAV() const
{
	return uav;
}

inline ID3D11Buffer *GPUBuffer::GetBuffer() const
{
	return gpu_buffer;
}

BOOL GPUBuffer::CopyToHost(BYTE *dst)
{
	BOOL Res = FALSE;
	if (dst != nullptr && _device != nullptr && _device->IsSuccess())
	{
		ID3D11Buffer *tempBuffer;
		D3D11_BUFFER_DESC outputDesc;
		outputDesc.Usage = D3D11_USAGE_STAGING;
		outputDesc.ByteWidth = BufferSizeInBytes;
		outputDesc.BindFlags = 0;
		outputDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		outputDesc.StructureByteStride = Stride;
		outputDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

		if (SUCCEEDED(_device->GetDevice()->CreateBuffer(&outputDesc, 0, &tempBuffer)))
		{
			_device->GetContext()->CopyResource(tempBuffer, gpu_buffer);
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
			if (SUCCEEDED(_device->GetContext()->Map(tempBuffer, 0, D3D11_MAP_READ, 0, &mappedResource)))
			{
				memcpy(dst, mappedResource.pData, BufferSizeInBytes);
				_device->GetContext()->Unmap(tempBuffer, 0);
				Res = TRUE;
			}
			tempBuffer->Release();
		}
	}
	return Res;
}

inline BOOL GPUBuffer::IsSuccess() const
{
	return _success;
}

void GPUBuffer::Release()
{
	if (uav != nullptr)
	{
		uav->Release();
		uav = nullptr;
	}
	if (gpu_buffer != nullptr)
	{
		gpu_buffer->Release();
		gpu_buffer = nullptr;
	}
	_success = FALSE;
}

// GPUConstantBuffer----------------------------------------------------------------------------------

template<typename T>
GPUConstantBuffer<T>::GPUConstantBuffer(const GPUDevice &device, const T &&initData)
{
	if (device.IsSuccess())
	{
		_device = &device;
		D3D11_BUFFER_DESC cbDesc;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.ByteWidth = sizeof(T);
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		// Fill in the subresource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = &initData;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		HRESULT hr = _device->GetDevice()->CreateBuffer(&cbDesc, &InitData, &gpu_buffer);
		_success = SUCCEEDED(hr);
	}
}

template<typename T>
BOOL GPUConstantBuffer<T>::Set(const T &data)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	if (SUCCEEDED(_device->GetContext()->Map(gpu_buffer, 0, D3D11_MAP_WRITE, 0, &mappedResource)))
	{
		memcpy(mappedResource.pData, &data, sizeof(T));
		_device->GetContext()->Unmap(gpu_buffer, 0);
		return TRUE;
	}
	return FALSE;
}

template<typename T>
inline BOOL GPUConstantBuffer<T>::IsSuccess() const
{
	return _success;
}

template<typename T>
inline ID3D11Buffer *GPUConstantBuffer<T>::GetBuffer() const
{
	return gpu_buffer;
}