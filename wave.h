#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <memory>

#pragma comment(lib,"winmm.lib")

struct WaveHeader
{
	UINT RIFF = 0x46464952;
	UINT fileSize;
	UINT WAVE = 0x45564157;
	UINT FMT = 0x20746D66;
	UINT formatChunkSize = 16;
	USHORT formatType = 1;
	USHORT tracks = 2;
	UINT samplesPerSecond = 28160;
	UINT bytesPerSecond;
	USHORT frameSize;
	USHORT bitsPerSample = 16;
	UINT DATA = 0x61746164;
	UINT dataChunkSize;
	void Complete(UINT Duration, UINT waveSize = 4, UINT headerSize = 8)
	{
		float numSamples = static_cast<float>(samplesPerSecond);
		frameSize = (short)(tracks * ((bitsPerSample + 7) / 8));
		bytesPerSecond = samplesPerSecond * frameSize;
		int samples = (int)(numSamples * Duration);
		dataChunkSize = samples * frameSize;
		fileSize = waveSize + headerSize + formatChunkSize + headerSize + dataChunkSize;
	}
};


class WaveSound
{
	std::unique_ptr<int> output;
	UINT size;
public:

	WaveSound(UINT msDuration, UINT _samplesPerSecond)
	{
		WaveHeader _header;
		_header.samplesPerSecond = _samplesPerSecond;
		_header.Complete(msDuration);
		size = (_samplesPerSecond * msDuration);
		output.reset(new int[size + sizeof(WaveHeader)]);
		memcpy(output.get(), &_header, sizeof(WaveHeader));
	}

	inline BYTE *GetBuffer() const
	{
		return ((BYTE*)output.get()) + sizeof(WaveHeader);
	}

	inline UINT GetBufferSize() const
	{
		return size * 4;
	}

	inline UINT GetTotalSamples() const
	{
		return size;
	}

    inline UINT GetSampleSize() const
	{
		return sizeof(int);
	}

	BOOL Play(bool sync)
	{
		DWORD _flags = SND_MEMORY;
		if (!sync)
			_flags |= SND_ASYNC;
		return PlaySoundA((LPCSTR)output.get(), GetModuleHandle(0), _flags);
	}

	BOOL WriteToFile(LPCSTR file)
	{
		FILE *f = fopen(file,"wb");
		if (f != nullptr)
		{
			fwrite(output.get(), sizeof(WaveHeader) + GetBufferSize(), 1, f);
			fclose(f);
			return TRUE;
		}
		return FALSE;
	}
};