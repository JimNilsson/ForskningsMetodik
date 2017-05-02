struct VS_OUT
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
};


Texture2D DiffuseMap : register(t0);
SamplerState Sam : register(s0);

float4 main(VS_OUT input) : SV_TARGET
{
	return float4(input.nor,1.0f);// DiffuseMap.Sample(Sam, input.tex);
}