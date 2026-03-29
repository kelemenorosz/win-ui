
struct VertexShaderOutput {

	float4 Color : COLOR;
	float4 Position : SV_Position;

};

struct VertexShaderInput {

	float2 Position_xy : POSITION0;

};

struct PixelShaderInput {

	float4 Color : COLOR;

};

struct ColorDesc {
	float red;
	float green;
	float blue;
	float alpha;
	float z;
};

ConstantBuffer<ColorDesc> color : register(b0, space0);

VertexShaderOutput vertex_main(VertexShaderInput IN)
{
	VertexShaderOutput OUT;

	OUT.Position = float4(IN.Position_xy.x, IN.Position_xy.y, color.z, 1.0);
	OUT.Color = float4(color.red, color.green, color.blue, color.alpha);

	return OUT;
}

float4 pixel_main(PixelShaderInput IN) : SV_Target
{
	return IN.Color;
}