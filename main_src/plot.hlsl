
struct VertexShaderOutput {

	float4 Color : COLOR;
	float4 Position : SV_Position;

};

struct VertexShaderInput {

	float Position_x : POSITION0;
	float Position_y : POSITION1;

};

struct PixelShaderInput {

	float4 Color : COLOR;

};

struct ScalingDesc {

	float scaleX;
	float scaleY;
	float centreX;
	float centreY;

};

struct MouseWheelDesc {
	float mult;
	float range;
};

ConstantBuffer<ScalingDesc> desc : register(b0, space0);
ConstantBuffer<MouseWheelDesc> mwheel : register(b1, space0);

VertexShaderOutput vertex_main(VertexShaderInput IN)
{
	VertexShaderOutput OUT;

	OUT.Position = float4(IN.Position_x / desc.scaleX, IN.Position_y / desc.scaleY, 0.999, 1.0);
	OUT.Position.x += desc.centreX;
	OUT.Position.y += desc.centreY;

	OUT.Position.x *= mwheel.mult;
	OUT.Position.x -= -mwheel.mult - mwheel.range;
	OUT.Color = float4(1.0, 0.0, 0.0, 1.0);

	return OUT;
}

float4 pixel_main(PixelShaderInput IN) : SV_Target
{
	return IN.Color;
}