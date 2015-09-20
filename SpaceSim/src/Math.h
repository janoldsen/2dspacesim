#ifndef MATH_H
#define MATH_H




typedef struct
{
	union 
	{
		struct
		{
			float x;
			float y;
		};
		float a[3];
	};
} Point, Vector, PointVec;


typedef struct
{
	Vector a[3];
} Matrix;


Point newPoint(float x, float y);
Vector newVector(float x, float y);

Matrix newIdentiy();
Matrix newRotation(float angle);


PointVec mul(Matrix* m, PointVec* v);



#endif
