#include "snd_include.cs"

#define D(u,v)   b+=float(u);if(t>b){x=b;n=float(v);}

[numthreads(NUM_THREADS_PER_GROUP, 1, 1)]
void CSMain( uint dispatchThreadID : SV_DispatchThreadID)
{
	float time = TIME(dispatchThreadID);
	//time = fmod(time, 40.0);
	float2 tot=0.0;
	for (int i = 0; i < 3; i++)
	{
		float h = float(i) / (3.0f - 1.0f);
		// compute note	
		float t = (time - 0.57f*h) / 0.18f;
		float n = 0.0f, b = 0.0f, x = 0.0f;
		
		D(10, 71)D(2, 76)D(3, 79)D(1, 78)D(2, 76)D(4, 83)D(2, 81)D(6, 78)D(6, 76)D(3, 79)
		D(1, 78)D(2, 74)D(4, 77)D(2, 71)D(10, 71)D(2, 76)D(3, 79)D(1, 78)D(2, 76)D(4, 83)
		D(2, 86)D(4, 85)D(2, 84)D(4, 80)D(2, 84)D(3, 83)D(1, 82)D(2, 71)D(4, 79)D(2, 76)
		D(10, 79)D(2, 83)D(4, 79)D(2, 83)D(4, 79)D(2, 84)D(4, 83)D(2, 82)D(4, 78)D(2, 79)
		D(3, 83)D(1, 82)D(2, 70)D(4, 71)D(2, 83)D(10, 79)D(2, 83)D(4, 79)D(2, 83)D(4, 79)
		D(2, 86)D(4, 85)D(2, 84)D(4, 80)D(2, 84)D(3, 83)D(1, 82)D(2, 71)D(4, 79)D(2, 76)
		

		// calc frequency and time for note	  
		float noteFreq = 440.0f*pow(2.0f, (n - 69.0f) / 12.0f);
		float noteTime = 0.18f*(t - x);

		// compute instrument	
		float y = 0.5f*sin(6.2831f*1.00f*noteFreq*noteTime)*exp(-0.0015f*1.0f*noteFreq*noteTime);
		y += 0.3f*sin(6.2831f*2.01f*noteFreq*noteTime)*exp(-0.0015f*2.0f*noteFreq*noteTime);
		y += 0.2f*sin(6.2831f*4.01f*noteFreq*noteTime)*exp(-0.0015f*4.0f*noteFreq*noteTime);
		y += 0.1f*y*y*y;
		y *= 0.9f + 0.1f*cos(40.0f*noteTime);
		y *= smoothstep(0.0f, 0.01f, noteTime);

		tot += y * float2(0.5f + 0.2f*h,0.5f - 0.2f*h) * (1.0f - sqrt(h)*0.85f);
	}
	WriteStereo(dispatchThreadID,tot);
}