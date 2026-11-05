uniform float4x4 modelViewProj;
uniform float3 lightDir;
uniform float3 lightDiffuse;
uniform float3 lightAmbient;

uniform float3 cameraPos;
uniform float fogStart;
uniform float fogEnd;

void main(
	float3 position,
	float2 texcoord,
	float3 normal,
	float2 out vTexcoord : TEXCOORD0,
	float3 out vLight : TEXCOORD1,
	float out vFogFactor : TEXCOORD2,
	float4 out gl_Position : POSITION
) {
	gl_Position = mul(float4(position, 1.0f), modelViewProj);
	vTexcoord = texcoord;

	float ndotl = max(dot(normalize(normal), lightDir), 0.0f);
	vLight = lightAmbient + lightDiffuse * ndotl;

	float camDist = length(position - cameraPos);
	vFogFactor = clamp((fogEnd - camDist) / (fogEnd - fogStart), 0.0f, 1.0f);
}
