#ifndef SND_INCLUDE
#define SND_INCLUDE

RWStructuredBuffer<int> BufferOut : register(u0);

cbuffer VS_CONSTANT_BUFFER : register(b0)
{
	uint NUM_SAMPLES;
	uint DURATION;
	uint T0;
	uint T1;
};

#define DURATION 60.0f
#define NUM_SAMPLES 28160.0f
#define TOTAL_SAMPLES (NUM_SAMPLES * DURATION)
#define NUM_THREADS_PER_GROUP 256

#define POSITION  ((groupID-1) * NUM_THREADS_PER_GROUP) + groupThreadID
#define TIME(_id) (float(_id) / TOTAL_SAMPLES) * DURATION

// glsl map
#define vec2 float2
#define vec3 float3
#define vec4 float4

#define fract frac
#define mix lerp
#define mod fmod

#define ivec4 int4
#define ivec2 int2

inline void WriteStereo(uint pos,float2 stereo)
{
	int l = (int)(32767.0f * stereo.x);
	int r = (int)(32767.0f * stereo.y);
	BufferOut[pos] =  (int)((l & 0xffff) << 16 | (r & 0xffff));
}

#endif