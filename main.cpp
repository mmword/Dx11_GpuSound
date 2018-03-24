#define _CRT_SECURE_NO_WARNINGS

#include "gpu_resource.h"
#include "wave.h"
#include <conio.h>
#include <string>

#define DURATION 60
#define NUM_SAMPLES 28160
#define TOTAL_SAMPLES (NUM_SAMPLES * DURATION)
#define TOTAL_SAMPLESI (UINT)(TOTAL_SAMPLES)
#define NUM_THREADS_PER_GROUP 256

// d3d shader compiler path (with including library)
//C:\Program Files (x86)\Windows Kits\8.1\bin\x64


struct ConstantData
{
	UINT _NUM_SAMPLES, _DURATION,T0,T1;
};

BOOL compile_and_play(GPUDevice &device,GPUProgram &generator,GPUBuffer &gpu_buffer,GPUConstantBuffer<ConstantData> &csBuffer,WaveSound &cpu_buffer_sound)
{
	printf("compiling...\n");
	BOOL res = generator.ReCompile();
	if (res)
	{
		device.BindProgram(&generator);
		device.SetBuffer(&gpu_buffer);
		device.SetConstantBuffer<ConstantData>(&csBuffer);
		device.Dispatch(cpu_buffer_sound.GetTotalSamples() / NUM_THREADS_PER_GROUP);
		device.ResetRersources();
		if ((res = gpu_buffer.CopyToHost(cpu_buffer_sound.GetBuffer())))
			res = cpu_buffer_sound.Play(false);
	}
	return res;
}


void main(int argc, char* argv[])
{
	if (argc < 2)
		return;

	int sz = strlen(argv[0]);
	int fileSz = strlen(argv[1]);

	std::wstring _file_path(sz,L'#'),_file(fileSz,L'#');
	mbstowcs(&_file_path[0], argv[0], sz);
	mbstowcs(&_file[0], argv[1], fileSz);

	if (_file[1] == L':')
		_file = _file.substr(_file_path.find_last_of(L'\\') + 1);
	
	_file_path = _file_path.substr(0, _file_path.find_last_of(L'\\') + 1).append(_file);

	wprintf(L"%s\n",_file_path.c_str());

	GPUDevice device;
	if (device.IsSuccess())
	{
		GPUProgram generator(device, _file_path.c_str(), "CSMain",FALSE);
		WaveSound cpu_buffer_sound(DURATION, NUM_SAMPLES);
		GPUBuffer gpu_buffer(device, cpu_buffer_sound.GetBufferSize(), cpu_buffer_sound.GetSampleSize());
		if (gpu_buffer.IsSuccess())
		{
			GPUConstantBuffer<ConstantData> csBuffer(device, { NUM_SAMPLES , DURATION });
			printf("press enter to recompile...\n");

			do
			{
				if (!compile_and_play(device, generator, gpu_buffer, csBuffer, cpu_buffer_sound))
					printf("error...\n");
			} while (_getch() == 13);

			generator.Release();
			gpu_buffer.Release();
			device.Release();
		}
		else
		{
			printf("cant create gbu buffer...\n");
			_getch();
		}
	}
	else
	{
		printf("cant initialize device...\n");
		_getch();
	}
}