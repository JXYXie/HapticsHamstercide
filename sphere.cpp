#include "sphere.h"

Sphere::Sphere(cVector3d p, double r, bool stationary) {
	fixed = stationary;
	radius = r;
	point = new cShapeSphere(radius);
	point->setLocalPos(p);
}

void Sphere::updateSphere(double time) {
	cVector3d force(0, 0, 0);

	for (cVector3d f : global_forces) {
		force += f;
	}
	// Reset vector of global forces
	global_forces.clear();

	double cair = 1;
	cVector3d force_damp = -cair * velocity;
	cVector3d acceleration = (force + force_damp) / mass;
	velocity = velocity + time * acceleration;
	cVector3d position = point->getLocalPos() + time * velocity;
	point->setLocalPos(position);
}

cVector3d Sphere::calculateForces(cVector3d c_p, double c_r) {
	cVector3d force(0, 0, 0);
	cVector3d pos = getPosition();

	cVector3d dist = c_p - pos;
	double d = - dist.length() + (radius + c_r);

	if (d > 0) {
		dist.normalize();
		force = dist * d * -k;
		addForce(force);
	}
	return -force;
}

void Sphere::addForce(cVector3d force) {
	if (!fixed) {
		global_forces.push_back(force);
	}
}

cVector3d Sphere::getPosition() {
	return point->getLocalPos();
}
