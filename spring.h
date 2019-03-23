#ifndef spring_h
#define spring_h

#include <stdio.h>
#include "chai3d.h"
#include "sphere.h"

class Spring {
	Sphere* p_a;
	Sphere* p_b;
	double spring_length;
	double k;
	double c;

public:

	Spring(Sphere*, Sphere*, double, double, double);
	cShapeLine* line;
	void calculateForces();
	void updateSpring();

};
#endif
