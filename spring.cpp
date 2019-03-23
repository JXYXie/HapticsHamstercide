#include "spring.h"
Spring::Spring(Sphere* a, Sphere* b, double length, double stiffness, double dampening) {
	p_a = a;
	p_b = b;
	spring_length = length;
	k = stiffness;
	c = dampening;
	line = new cShapeLine(p_a->getPosition(), p_b->getPosition());
	line->m_material->setYellow();
}

void Spring::calculateForces() {
	cVector3d a = p_a->getPosition();
	cVector3d b = p_b->getPosition();
	cVector3d force_damp = a - b;
	force_damp.normalize();
	double force_spring = (a - b).length() - spring_length;
	cVector3d force = -k * force_damp * force_spring;
	p_a->addForce(force - c * p_a->velocity);
	p_b->addForce(-force - c * p_b->velocity);
}

void Spring::updateSpring() {
	line->m_pointA = p_a->getPosition();
	line->m_pointB = p_b->getPosition();
}

