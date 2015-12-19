#include "Physics.h"
#include "Math.h"

// COLLISION
// render objects in RG-int-texture
// id is tile id in upper bits and pixel id in lower bits
// write +id in R and -id in G and blend with MIN operations


typedef struct PhysicsShipData
{
	float invMass;
	float invInertia;
} PhysicsShipData;

void stepPhysics(float dt)
{

	//broadphase

	//narrowphase 

	//solver
}
