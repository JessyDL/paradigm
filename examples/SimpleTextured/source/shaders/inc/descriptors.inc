struct FrameData
{
	mat4 clipMatrix;
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 WVP;
	mat4 VP;
	vec4 ScreenParams;
	vec4 GameTimer;
	vec4 Parameters;
	vec4 FogColor;
	vec4 viewPos;
	vec4 viewDir;
};

struct material
{
	vec4 color;
};

struct Light 
{
	vec4 color;
	vec4 component1;
	vec4 component2;
};