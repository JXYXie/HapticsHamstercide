#ifndef sphere_h
#define sphere_h

#include <stdio.h>
#include "chai3d.h"
#include <vector>

using namespace chai3d;
using namespace std;

class Sphere {
	// Attributes of sphere
	bool fixed;
	double radius = 0.01;
	double k = 1000.0;
	// Vector of forces to be applied
	vector<cVector3d> global_forces;

public:

	cShapeSphere* point;
	double mass = 0.005;
	cVector3d velocity = cVector3d(0, 0, 0);

	Sphere(cVector3d, double, bool stationary = false);

	// Goes through the list of calculated forces and applies them to the sphere
	void updateSphere(double);
	// Given the cursor position will calculate the forces to be applied to the sphere
	// Will return force applied back to the cursor
	cVector3d calculateForces(cVector3d c_p, double c_r);

	void addForce(cVector3d);

	cVector3d getPosition();

};

#endif
