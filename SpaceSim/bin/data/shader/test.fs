
//#version 430 core

//#include "ShadewrDefinitions.h"


layout (binding = DESTRUCTION_TEX_BINDING)
uniform usamplerBuffer destructionBuffer;

layout (binding = COLOR_TEX_BINDING)
uniform usamplerBuffer colorBuffer;



struct ColorPalette
{
	vec3[8] colors;
};


layout (std140, binding = COLOR_PALETTE_UNI_BINDING)
uniform Palettes
{
	ColorPalette palettes[MAX_SHIPS];
};


in VertexOutput
{
	vec2 uv;
	flat int shipIdx;
	flat int colorIdx;
	flat int destructionIdx;
} input;


out vec4 fragColor;





bool isDestroyed(ivec2 pos)
{

	int idx = pos.y / 4;
	uint bit = (pos.y-idx*4) * 8 + pos.x;
	
	uint destruction = (texelFetch(destructionBuffer, input.destructionIdx * 2 + idx).r >> bit) & 1;
		
	return (destruction == 0);
}

vec3 getColor(ivec2 pos)
{
	int unpackedIdx = pos.y * 8 + pos.x;
	int totalShift = unpackedIdx * 3;
	int packedIdx = totalShift / 32;
	uint shift = totalShift % 32;

		
	uint paletteIdx = 0;
	paletteIdx |= (texelFetch(colorBuffer, input.colorIdx * 6 + packedIdx).r >> shift) & (1 << 3)-1;

	if (shift >= 30)
		paletteIdx |= (texelFetch(colorBuffer, input.colorIdx * 6 + packedIdx+1).r & (1 << (shift-32 + 3))-1) << (32 - shift);
			
	return palettes[input.shipIdx].colors[paletteIdx];
}


void main()
{
	
	ivec2 pos = ivec2(input.uv * 8);
	
	if (isDestroyed(pos))
	{
		discard;
		return;
	}
	
	fragColor = vec4(getColor(pos),1);
}



