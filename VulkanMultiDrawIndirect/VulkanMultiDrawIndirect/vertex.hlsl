struct VS_IN {
	float3 pos : POSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
};

struct VS_OUT
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
};

cbuffer PerFrameBuffer : register(b0)
{
	float4x4 gViewProj;

}

cbuffer ObjectBuffer : register(b1)
{
	float4x4 gWorld;
	float4x4 gWorldViewInvTrp;
}

VS_OUT main( VS_IN input )
{
	VS_OUT output;
	output.pos = mul(float4(input.pos,1.0f), gWorld);
	output.nor = mul(float4(input.nor,0.0f), gWorldViewInvTrp);
	output.tex = input.tex;
	return output;
}