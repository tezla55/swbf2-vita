uniform sampler2D layer0 : TEXUNIT0;

uniform float3 fogColor;

float4 main(
	float2 vTexcoord : TEXCOORD0,
	float3 vLight : TEXCOORD1,
	float vFogFactor : TEXCOORD2
) {
	float4 c = tex2D(layer0, vTexcoord);
	c.rgb = c.rgb * vLight;
	c.rgb = lerp(fogColor, c.rgb, vFogFactor);
	return c;
}
