uniform float4x4 modelViewProj;
uniform float3 lightDir;
uniform float3 lightDiffuse;
uniform float3 lightAmbient;

uniform float3 cameraPos;

uniform float3 camFwd;

uniform float2 fogBand;
uniform float fogStart;
uniform float fogEnd;

uniform float4x4 shadowMat;
uniform float4x4 staticMat;

uniform float4 Uterr;

uniform float4 Uuv;

uniform float4 clipPlane;

void main(
	float3 position,
	float2 texcoord,
	float4 weights0123,
	float weight4,
	float3 normal,
	float4 color,
	float2 out vTexcoord : TEXCOORD0,
	float4 out vWeights0123 : TEXCOORD1,
	float2 out vWeight4Fog : TEXCOORD2,
	float3 out vLight : TEXCOORD3,
#if !defined(TERR_BASE) || defined(TERR_STATIC)
	float2 out vStatic : TEXCOORD4,
#endif
#if !defined(TERR_BASE) || defined(TERR_SHADOW)
	float4 out vShadow : TEXCOORD5,
#endif
	float3 out vWorldPos : TEXCOORD6,
#ifndef TERR_BASE
	float3 out vNormal : TEXCOORD7,
#endif
	float out vClip : CLP0,
	float4 out gl_Position : POSITION
) {
	gl_Position = mul(float4(position, 1.0f), modelViewProj);

	float2 layerUV = texcoord;
	if (Uuv.w > -0.5f)
	{
		if (Uuv.w < 0.5f) layerUV = position.xz * Uuv.x + Uuv.yz;
		else if (Uuv.w < 1.5f) layerUV = float2(position.x, position.y) * Uuv.x + Uuv.yz;
		else layerUV = float2(position.y, position.z) * Uuv.x + Uuv.yz;
	}
	vTexcoord = layerUV;
	vWeights0123 = weights0123;
	vWeight4Fog.x = weight4;

	vLight = color.rgb * Uterr.x + lightDiffuse * max(0.0f, dot(normalize(normal), lightDir));
	vWorldPos = position;
#ifndef TERR_BASE
	vNormal = normal;
#endif
	vClip = dot(float4(position, 1.0f), clipPlane);

	float camDist = (dot(camFwd, camFwd) > 0.5f) ? dot(position - cameraPos, camFwd) : length(position - cameraPos);
	vWeight4Fog.y = clamp((fogEnd - camDist) / (fogEnd - fogStart), 0.0f, 1.0f);
	if (fogBand.y > 0.0f) vWeight4Fog.y = 1.0f - (1.0f - vWeight4Fog.y) * saturate((camDist - fogBand.x) * fogBand.y);
#if !defined(TERR_BASE) || defined(TERR_STATIC)
	vStatic = mul(float4(position, 1.0f), staticMat).xy;
#endif
#if !defined(TERR_BASE) || defined(TERR_SHADOW)
	vShadow = mul(float4(position, 1.0f), shadowMat);
#endif
}
