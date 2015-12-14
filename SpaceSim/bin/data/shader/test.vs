//#version 430 core

//#include "ShaderDefinitions.h"



uniform layout(location = SCREEN_SIZE_UNI_LOC)
vec2 screenSize = vec2(800,600);

uniform layout(location = BLOCK_SIZE_UNI_LOC)
int blockSize = 50;

	
layout (std140, binding = TRANSFORM_UNI_BINDING )
uniform Transforms
{
	mat3x3 shipTransforms[MAX_SHIPS];
};

in layout(location = POS_ATTRIB_LOC)
ivec2 platePos;
in layout(location = SHIP_IDX_ATTRIB_LOC)
int shipIdx;
in layout(location = COLOR_IDX_ATTRIB_LOC)
int colorIdx;
in layout(location = DESTRUCTION_IDX_ATTRIB_LOC)
int destructionIdx;
	

out VertexOutput
{
	vec2 uv;
	flat int shipIdx;
	flat int colorIdx;
	flat int destructionIdx;
} vsOutput;






void main()
{
	const vec3 pos[] = vec3[]
	(
		vec3(-0.5,  0.5, 1),
		vec3( 0.5,  0.5, 1),
		vec3(-0.5, -0.5, 1),
		vec3( 0.5, -0.5, 1)
	);
	
	const vec2 uvs[] = vec2[]
	(
		vec2(0,  0),
		vec2(1,  0),
		vec2(0,  1),
		vec2(1,  1)
	);
	
	
	mat4x4 proj = mat4x4(
		2/screenSize.x, 0, 	0, -1,
		0, 2/screenSize.y,	0, -1,
		0, 0, 			1, 0,
		0, 0, 			0, 1
	);
	

	
	mat3x3 view = mat3(
		0.5, 0, screenSize.x/2,
		0, 0.5, screenSize.y/2,
		0, 0, 1
	);
	
	mat3x3 blockMat = mat3x3(
		blockSize, 0, blockSize * platePos.x,
		0, blockSize, blockSize * platePos.y,
		0,   0, 1 
	);	
	
	gl_Position =  vec4((pos[gl_VertexID] * blockMat * shipTransforms[shipIdx] * view),1) * proj;
	vsOutput.uv = uvs[gl_VertexID];
	vsOutput.shipIdx = shipIdx;
	vsOutput.colorIdx = colorIdx;
	vsOutput.destructionIdx = destructionIdx;
	
}







