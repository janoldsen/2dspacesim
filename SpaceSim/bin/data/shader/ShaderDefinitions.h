
//shared header between shaders and application



//application dependent defines
#ifdef APPLICATION
typedef unsigned int uint;
typedef struct { int x; int y; } ivec2;
typedef struct Plate GfxPlate;
#endif //APPLICATION

struct Plate
{
	ivec2 pos;
	uint shipIdx;
	uint colorIdx;
	uint destructionIdx;
};



#define SCREEN_SIZE_UNI_LOC 0
#define BLOCK_SIZE_UNI_LOC 1

#define POS_ATTRIB_LOC 0
#define SHIP_IDX_ATTRIB_LOC 1
#define COLOR_IDX_ATTRIB_LOC 2
#define DESTRUCTION_IDX_ATTRIB_LOC 3

#define DESTRUCTION_TEX_BINDING 0
#define COLOR_TEX_BINDING 1
#define TRANSFORM_UNI_BINDING 0
#define COLOR_PALETTE_UNI_BINDING 1


#define MAX_SHIPS 256
