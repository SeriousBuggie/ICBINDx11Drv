#define DO_STANDARD_BUFFER 0
#include "ShaderGlobals.h"

// Metallicafan212:	Constant buffer, but with the added tile rotation values
cbuffer CommonBuffer : register (START_CONST_NUM)
{
	COMMON_VARS
	
	// Metallicafan212:	The info we use for this specific shader
	float4		XAxis		: packoffset(c4);
	float4		YAxis		: packoffset(c5);
	float4		ZAxis		: packoffset(c6);
	int			bDoRot		: packoffset(c7.x);
	int			bDoUVHack	: packoffset(c7.y);
	float2		Pad3		: packoffset(c7.z);
};

// Metallicafan212:	HACK!!!! This includes this twice to define the final color function, as HLSL cannot do out of order compiling
//					The buffer variables have to be defined before they can be used
#define DO_FINAL_COLOR
#include "ShaderGlobals.h"

// Metallicafan212:	Standard texture sampler
Texture2D 	Diffuse 		: register(t0);

SamplerState DiffState;

struct VSInput 
{ 
	float4 pos 		: POSITION0;
	float4 uv		: TEXCOORD0;
	float4 color	: COLOR0;
	float4 fog		: COLOR1;
};

struct PSInput 
{
	float4 pos 						: SV_POSITION0; 
	float2 uv						: TEXCOORD0;
	
	// Metallicafan212:	The default proton/wine HLSL compiler doesn't support the centroid modifier for some reason..... 
#if !WINE
	centroid linear float2 cuv		: TEXCOORD1;
#else
	float2 cuv						: TEXCOORD1;
#endif

	float4 color					: COLOR0; 
	float4 fog						: COLOR1;
	float  distFog					: COLOR2;
	int   bRejectBW					: COLOR3;
};

PSInput VertShader(VSInput input)
{	
	PSInput output = (PSInput)0;
	
	// Metallicafan212:	Rotate this vert if we're doing rotated drawing
	if(bDoRot)
	{
		input.pos.xyz = float3(dot(input.pos, XAxis), dot(input.pos, YAxis), dot(input.pos, ZAxis));//mul(input.pos, Coords);
	}
	
	// Metallicafan212:	Set the W to 1 so matrix math works
	input.pos.w 		= 1.0f;
	
	// Metallicafan212:	Transform it out
	output.pos 			= mul(input.pos, Proj);
	output.color		= input.color;
	output.uv.xy		= input.uv.xy;
	output.cuv.xy		= input.uv.xy;
	
	// Metallicafan212:	Do the final fog value
	output.distFog		= DoDistanceFog(output.pos);
	
	// Metallicafan212:	TODO! If this is a UI tile, don't do the HP2 specific black and white effect
	//					Should make this configurable....
	output.bRejectBW	= input.pos.z <= 0.5f;
	
	return output;
}

float4 PxShader(PSInput input) : SV_TARGET
{
	// Metallicafan212:	Leaving this in here for future reference, if I ever need it again
	//					NVidia, with point filtering on will not round UV coordinates coming in, like AMD and Intel
	//					Rather, it just seems to floor them
	/*
	float4 DiffColor;
	// Metallicafan212:	NVIDIA SPECIFIC HACK!!!!
	//					TODO! Figure out how to get around this when uploading the verts to the GPU, so we don't have to have vendor special code....
	if(bNVTileHack)
	{
		float USize, VSize, Levels;
		
		// Metallicafan212:	Get the dimensions, since .Load uses the actual pixel locations, not 0.0-1.0 UV coords
		Diffuse.GetDimensions(0, USize, VSize, Levels);
		
		// Metallicafan212:	Round the coordinates to get around the NVidia UV issues
		DiffColor = Diffuse.Load(float3(round(input.uv.x * USize), round(input.uv.y * VSize), 0.0f), 0) * input.color;
	}
	else
	{
		DiffColor = Diffuse.SampleBias(DiffState, input.uv, 0.0f) * input.color;
	}
	*/
	
	float2 UseUV = input.uv;
	
	float4 DiffColor;
	
	if(bDoUVHack)
	{
		UseUV 		= input.cuv;
		
		/*
		if(UseUV.x >= 1.0f)
		{
			UseUV.x -= floor(UseUV.x);
		}
		
		if(UseUV.y >= 1.0f)
		{
			UseUV.y -= floor(UseUV.y);
		}
		
		float USize, VSize, Levels;
		
		// Metallicafan212:	Calculate the mip level
		float LOD = 0.0f;//Diffuse.CalculateLevelOfDetail(DiffState, UseUV);
		
		// Metallicafan212:	Get the dimensions, since .Load uses the actual pixel locations, not 0.0-1.0 UV coords
		Diffuse.GetDimensions(LOD, USize, VSize, Levels);
		
		UseUV		= UseUV * (float2(USize, VSize));
		
		// Metallicafan212:	Round the coordinates to get around the NVidia UV issues
		UseUV 		= round(UseUV);
		
		
		DiffColor = Diffuse.Load(float3(UseUV, LOD)) * input.color;
		*/
		
		DiffColor = DoGammaCorrection(Diffuse.SampleBias(DiffState, UseUV, 0.0f)) * input.color;
	}
	else
	{
		DiffColor = DoGammaCorrection(Diffuse.SampleBias(DiffState, UseUV, 0.0f)) * input.color;
	}
	
	
	// Metallicafan212:	Do alpha rejecting
	//					TODO! This also sets the global selection color for the editor!
	//					Reevaluate how we do this!!!!
	CLIP_PIXEL(DiffColor);
	
	DiffColor = DoPixelFog(input.distFog, DiffColor);
	
	//if(bDoSelection || !input.bRejectBW)
	//{
	DiffColor = DoFinalColor(DiffColor);
	//}

	return DiffColor;
}