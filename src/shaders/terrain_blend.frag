uniform float4 UtLight;
uniform float3 UtLightC;

uniform sampler2D layer0 : TEXUNIT0;
uniform sampler2D layer1 : TEXUNIT1;
uniform sampler2D layer2 : TEXUNIT2;
uniform sampler2D layer3 : TEXUNIT3;
uniform sampler2D layer4 : TEXUNIT4;

uniform float3 fogColor;

uniform float3 lightAmbient;

uniform sampler2D shadowMap : TEXUNIT5;
uniform float4 shadowP;

uniform sampler2D staticShadow : TEXUNIT6;
uniform float4 staticP;
uniform float4 dynPos[2];
uniform float4 dynCol[2];

uniform float Ugate;

uniform float4 UdetailP;

uniform float4 UuvM;
uniform float4 Ulay;
uniform float Ulay4;

uniform float Udg;

uniform float4 Uterr;

uniform sampler2D UskyTex : TEXUNIT7;
uniform float4 UskyFar;

uniform sampler2D canopyTex : TEXUNIT8;
uniform float4 UcanopyP;
uniform float4 UcanopyA;
uniform float3 UcanopyL;
float CanopyShadow(float3 wp)
{
	if (UcanopyP.x < 0.5f) return 0.0f;

	float2 shear = UcanopyL.xy * wp.y;
	float2 uv = (wp.xz + shear) * UcanopyP.y;
	uv += UcanopyA.xy + UcanopyA.zw;
	float c = tex2D(canopyTex, uv).r;
	return c * UcanopyP.w;
}
float3 SkyFadeColor(float4 coords)
{
	return (UskyFar.x > 0.5f) ? (float3)tex2D(UskyTex, coords.xy * UskyFar.yz).rgb : fogColor;
}

#ifdef TERR_BASE
float4 main(
	float2 vTexcoord : TEXCOORD0,
	float4 vWeights0123 : TEXCOORD1,
	float2 vWeight4Fog : TEXCOORD2,
	float3 vLight : TEXCOORD3,
	float3 vWorldPos : TEXCOORD6
#ifdef TERR_STATIC
	, float2 vStatic : TEXCOORD4
#endif
#ifdef TERR_SHADOW
	, float4 vShadow : TEXCOORD5
#endif
	, float4 coords : WPOS
) {
#ifdef TERR_ADD
	{
		float zp = UtLight.y + 5.0f - vWorldPos.y;
		float2 h = float2(UtLight.z - vWorldPos.z, vWorldPos.x - UtLight.x);
		float rho = (zp > 0.0f) ? length(h) / zp : 2.0f;
		float c = cos(1.5707963f * min(rho / 0.98f, 1.0f));
		return float4(UtLightC * (c * c), 0.0f);
	}
#endif

	if (Udg > 5.5f)
	{
#ifdef TERR_SINGLE
		return float4(0.0f, 1.0f, 0.0f, 1.0f);
#else
		return float4(1.0f, 0.0f, 0.0f, 1.0f);
#endif
	}
	if (Udg > 4.5f) return float4(frac(vTexcoord * 4.0f), 0.0f, 1.0f);
#ifdef TERR_SINGLE
	float4 result = tex2D(layer0, vTexcoord);
	if (UdetailP.x > 0.5f)
	{
		float3 d = 2.0f * tex2D(layer1, vWorldPos.xz * UdetailP.y).rgb;
		result.rgb *= 1.0f + UdetailP.z * (d - 1.0f);
	}
#else
	float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float2 wp = vWorldPos.xz + UuvM.xy;
	float2 t0 = Ulay.x > 0.0f ? wp * Ulay.x : vTexcoord;
	float2 t1 = Ulay.y > 0.0f ? wp * Ulay.y : vTexcoord;
	float2 t2 = Ulay.z > 0.0f ? wp * Ulay.z : vTexcoord;
	float2 t3 = Ulay.w > 0.0f ? wp * Ulay.w : vTexcoord;
	float2 t4 = Ulay4 > 0.0f ? wp * Ulay4 : vTexcoord;
	if (vWeights0123.x != 0.0f) result += tex2D(layer0, t0) * vWeights0123.x;
	if (vWeights0123.y != 0.0f) result += tex2D(layer1, t1) * vWeights0123.y;
	if (vWeights0123.z != 0.0f) result += tex2D(layer2, t2) * vWeights0123.z;
	if (vWeights0123.w != 0.0f) result += tex2D(layer3, t3) * vWeights0123.w;
	if (vWeight4Fog.x != 0.0f) result += tex2D(layer4, t4) * vWeight4Fog.x;
#endif
	float3 albedo = result.rgb;
	float3 light = min(vLight, Uterr.yyy);
	result.rgb = albedo * light;
	float shadowFill = 0.0f;
#if defined(TERR_STATIC) || defined(TERR_SHADOW)
	float shadowed = 0.0f;
#ifdef TERR_STATIC
	if (vStatic.x > 0.0f && vStatic.x < 1.0f && vStatic.y > 0.0f && vStatic.y < 1.0f)
		shadowed = 1.0f - tex2D(staticShadow, vStatic.xy).r;
#endif
#ifdef TERR_SHADOW
	{
		float3 sc = vShadow.xyz;
		float edge = saturate(min(min(sc.x, 1.0f - sc.x), min(sc.y, 1.0f - sc.y)) * 20.0f);
		if (edge > 0.0f && sc.z < 1.0f)
		{
			float3 e = tex2D(shadowMap, sc.xy).rgb;
			if (sc.z <= e.b) shadowed = max(shadowed, (1.0f - e.r) * edge);
		}
	}
#endif
	result.rgb *= lerp(1.0f, shadowP.y, shadowed);
	shadowFill = shadowed;
#endif
	const float canopy = CanopyShadow(vWorldPos);
	result.rgb *= lerp(1.0f, shadowP.y, canopy);
	shadowFill = max(shadowFill, canopy);

	shadowFill *= shadowFill;
	result.rgb += albedo * lightAmbient * shadowFill;

	if (vWeight4Fog.y < 1.0f) result.rgb = lerp(SkyFadeColor(coords), result.rgb, vWeight4Fog.y);
	result.a *= Uterr.z;
	return result;
}
#else
float4 main(
	float2 vTexcoord : TEXCOORD0,
	float4 vWeights0123 : TEXCOORD1,
	float2 vWeight4Fog : TEXCOORD2,
	float3 vLight : TEXCOORD3,
	float2 vStatic : TEXCOORD4,
	float4 vShadow : TEXCOORD5,
	float3 vWorldPos : TEXCOORD6,
	float3 vNormal : TEXCOORD7,
	float4 coords : WPOS
) {

	float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
	if (Ugate > 1.5f)
	{
		result = tex2D(layer0, vTexcoord);
		if (UdetailP.x > 0.5f)
		{
			float3 d = 2.0f * tex2D(layer1, vWorldPos.xz * UdetailP.y).rgb;
			result.rgb *= 1.0f + UdetailP.z * (d - 1.0f);
		}
	}
	else
	{
		float2 wp = vWorldPos.xz + UuvM.xy;
		float2 t0 = Ulay.x > 0.0f ? wp * Ulay.x : vTexcoord;
		float2 t1 = Ulay.y > 0.0f ? wp * Ulay.y : vTexcoord;
		float2 t2 = Ulay.z > 0.0f ? wp * Ulay.z : vTexcoord;
		float2 t3 = Ulay.w > 0.0f ? wp * Ulay.w : vTexcoord;
		float2 t4 = Ulay4 > 0.0f ? wp * Ulay4 : vTexcoord;
		if (Ugate < 0.5f || vWeights0123.x != 0.0f) result += tex2D(layer0, t0) * vWeights0123.x;
		if (Ugate < 0.5f || vWeights0123.y != 0.0f) result += tex2D(layer1, t1) * vWeights0123.y;
		if (Ugate < 0.5f || vWeights0123.z != 0.0f) result += tex2D(layer2, t2) * vWeights0123.z;
		if (Ugate < 0.5f || vWeights0123.w != 0.0f) result += tex2D(layer3, t3) * vWeights0123.w;
		if (Ugate < 0.5f || vWeight4Fog.x != 0.0f) result += tex2D(layer4, t4) * vWeight4Fog.x;
	}

	if (Udg > 5.5f) return (Ugate > 1.5f) ? float4(0.0f, 1.0f, 0.0f, 1.0f) : float4(1.0f, 0.0f, 0.0f, 1.0f);
	if (Udg > 4.5f) return float4(frac(vTexcoord * 4.0f), 0.0f, 1.0f);
	if (Udg > 3.5f) return float4(0.5f, 0.5f, 0.5f, 1.0f);
	if (Udg > 2.5f) return float4(result.rgb, 1.0f);
	float3 albedo = result.rgb;
	float3 light = min(vLight, Uterr.yyy);

	if (Udg < 1.5f && (dynPos[0].w > 0.0f || dynPos[1].w > 0.0f))
	{
		float3 n = normalize(vNormal);
		for (int i = 0; i < 2; i++)
		{
			if (dynPos[i].w > 0.0f)
			{
				float3 L = dynPos[i].xyz - vWorldPos;
				float d = length(L);
				float att = saturate(1.0f - d / max(dynPos[i].w, 0.001f));
				light += dynCol[i].rgb * (att * max(0.0f, dot(n, L / max(d, 0.001f))));
			}
		}
	}
	result.rgb = albedo * light;
	float shadowFill = 0.0f;
	float shadowed = 0.0f;

	const bool dgShadows = !(Udg > 0.5f && Udg < 1.5f);
	if (dgShadows && staticP.x > 0.5f)
	{
		float2 ss = vStatic;
		if (ss.x > 0.0f && ss.x < 1.0f && ss.y > 0.0f && ss.y < 1.0f)
			shadowed = 1.0f - tex2D(staticShadow, ss.xy).r;
	}
	if (dgShadows && shadowP.x > 0.5f)
	{
		float3 sc = vShadow.xyz;
		float edge = saturate(min(min(sc.x, 1.0f - sc.x), min(sc.y, 1.0f - sc.y)) * 20.0f);

		if (edge > 0.0f && sc.z < 1.0f)
		{
			float3 e = tex2D(shadowMap, sc.xy).rgb;
			if (sc.z <= e.b) shadowed = max(shadowed, (1.0f - e.r) * edge);
		}
	}
	result.rgb *= lerp(1.0f, shadowP.y, shadowed);
	shadowFill = shadowed;
	const float canopy = CanopyShadow(vWorldPos);
	result.rgb *= lerp(1.0f, shadowP.y, canopy);
	shadowFill = max(shadowFill, canopy);

	shadowFill *= shadowFill;
	result.rgb += albedo * lightAmbient * shadowFill;
	if (vWeight4Fog.y < 1.0f) result.rgb = lerp(SkyFadeColor(coords), result.rgb, vWeight4Fog.y);
	result.a *= Uterr.z;
	return result;
}
#endif
