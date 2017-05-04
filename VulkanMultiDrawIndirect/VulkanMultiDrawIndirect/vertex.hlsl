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

VS_OUT main(VS_IN input, uint id : SV_VertexID)
{
	VS_OUT output;
	output.pos = mul(float4(input.pos,1.0f), mul(gWorld, gViewProj));
	output.nor = mul(float4(input.nor,0.0f), gWorldViewInvTrp);
	output.tex = input.tex;
	//if(id % 3 == 0)
	//	output.pos = float4(0,0,0,1.0f);
	//if(id % 3 == 1)
	//	output.pos = float4(0,1,0,1.0f);
	//if(id % 3 == 2)
	//	output.pos = float4(1,0,0,1.0f);
		
	return output;
}