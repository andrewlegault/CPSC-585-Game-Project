#include "Racer.h"

int Racer::xID = 0;
int Racer::yID = 1;
int Racer::zID = 2;

ConfigReader Racer::config = ConfigReader();
	
hkVector4 Racer::xAxis = hkVector4(1.0f, 0.0f, 0.0f);
hkVector4 Racer::yAxis = hkVector4(0.0f, 1.0f, 0.0f);
hkVector4 Racer::zAxis = hkVector4(0.0f, 0.0f, 1.0f);
hkVector4 Racer::attachFL = hkVector4(-0.8f, -0.6f, 1.5f);
hkVector4 Racer::attachFR = hkVector4(0.8f, -0.6f, 1.5f);
hkVector4 Racer::attachRL = hkVector4(-0.8f, -0.53f, -1.2f);
hkVector4 Racer::attachRR = hkVector4(0.8f, -0.53f, -1.2f);

/*hkReal Racer::chassisMass = 1500.0f;
float Racer::accelerationScale = 25.0f;
float Racer::frontSpringK = 75000.0f;
float Racer::rearSpringK = 75000.0f;
float Racer::frontDamperC = 8000.0f;
float Racer::rearDamperC = 8000.0f;

float Racer::frontExtents = 0.35f;
float Racer::rearExtents = 0.42f;
float Racer::springForceCap = 320000.0f;*/

hkReal Racer::chassisMass = config.chassisMass;
float Racer::accelerationScale = config.accelerationScale;
float Racer::frontSpringK = config.kFront;
float Racer::rearSpringK = config.kRear;
float Racer::frontDamperC = config.frontDamping;
float Racer::rearDamperC = config.rearDamping;

float Racer::frontExtents = config.frontExtents;
float Racer::rearExtents = config.rearExtents;
float Racer::springForceCap = config.springForceCap;


Racer::Racer(IDirect3DDevice9* device, Renderer* r, Physics* p, RacerType racerType)
{
	index = -1;

	currentSteering = 0.0f;


	switch (racerType)
	{
	case PLAYER:
		drawable = new Drawable(RACER, "racer1.dds", device);
		break;
	case AI1:
		drawable = new Drawable(RACER, "racer2.dds", device);
		break;
	default:
		drawable = new Drawable(RACER, "racer2.dds", device);
	}


	// Set up filter group (so the car doesn't collide with the wheels)
	int collisionGroupFilter = p->getFilter();
	
	hkpRigidBodyCinfo info;
	hkVector4 halfExtent(0.9f, 0.7f, 2.3f);		//Half extent for racer rigid body box
	info.m_shape = new hkpBoxShape(halfExtent);
	info.m_qualityType = HK_COLLIDABLE_QUALITY_CRITICAL;
	info.m_centerOfMass = hkVector4(0.0f, -0.4f, -1.2f);	// move CM a bit
	info.m_restitution = 0.1f;
	hkpMassProperties massProperties;
	hkpInertiaTensorComputer::computeBoxVolumeMassProperties(halfExtent, chassisMass, massProperties);
	info.setMassProperties(massProperties);
	info.m_collisionFilterInfo = hkpGroupFilter::calcFilterInfo(hkpGroupFilterSetup::LAYER_AI, collisionGroupFilter);
	body = new hkpRigidBody(info);		//Create rigid body
	body->setLinearVelocity(hkVector4(0, 0, 0));
	info.m_shape->removeReference();

	index = r->addDrawable(drawable);
	p->addRigidBody(body);


	// Create tires
	wheelFL = new FrontWheel(device, collisionGroupFilter);
	r->addDrawable(wheelFL->drawable);
	p->addRigidBody(wheelFL->body);
	
	WheelListener* listenFL = new WheelListener(&(wheelFL->touchingGround));
	wheelFL->body->addContactListener(listenFL);

	
	wheelFR = new FrontWheel(device, collisionGroupFilter);
	r->addDrawable(wheelFR->drawable);
	p->addRigidBody(wheelFR->body);

	WheelListener* listenFR = new WheelListener(&(wheelFR->touchingGround));
	wheelFL->body->addContactListener(listenFR);
	


	wheelRL = new RearWheel(device, collisionGroupFilter);
	r->addDrawable(wheelRL->drawable);
	p->addRigidBody(wheelRL->body);

	WheelListener* listenRL = new WheelListener(&(wheelRL->touchingGround));
	wheelFL->body->addContactListener(listenRL);


	wheelRR = new RearWheel(device, collisionGroupFilter);
	r->addDrawable(wheelRR->drawable);
	p->addRigidBody(wheelRR->body);
	
	WheelListener* listenRR = new WheelListener(&(wheelRR->touchingGround));
	wheelFL->body->addContactListener(listenRR);

	// Now constrain the tires
	hkpGenericConstraintData* constraint;
	hkpConstraintInstance* constraintInst;

	constraint = new hkpGenericConstraintData();
	buildConstraint(&attachFL, constraint, FRONT);
	constraintInst = new hkpConstraintInstance(wheelFL->body, body, constraint);
	p->world->addConstraint(constraintInst);
	constraint->removeReference();

	constraint = new hkpGenericConstraintData();
	buildConstraint(&attachFR, constraint, FRONT);
	constraintInst = new hkpConstraintInstance(wheelFR->body, body, constraint);
	p->world->addConstraint(constraintInst);
	constraint->removeReference();
	
	constraint = new hkpGenericConstraintData();
	buildConstraint(&attachRL, constraint, REAR);
	constraintInst = new hkpConstraintInstance(wheelRL->body, body, constraint);
	p->world->addConstraint(constraintInst);
	constraint->removeReference();

	constraint = new hkpGenericConstraintData();
	buildConstraint(&attachRR, constraint, REAR);
	constraintInst = new hkpConstraintInstance(wheelRR->body, body, constraint);
	p->world->addConstraint(constraintInst);
	constraint->removeReference();

	
	hkpConstraintStabilizationUtil::stabilizeRigidBodyInertia(body);

	reset();
}


Racer::~Racer(void)
{
	if(body)
	{
		body->removeReference();
	}
}

void Racer::setPosAndRot(float posX, float posY, float posZ,
		float rotX, float rotY, float rotZ)	// In Radians
{
	drawable->setPosAndRot(posX, posY, posZ,
		rotX, rotY, rotZ);

	hkQuaternion quat;
	quat.setAxisAngle(hkVector4(1.0f, 0.0f, 0.0f), rotX);
	quat.mul(hkQuaternion(hkVector4(0.0f, 1.0f, 0.0f), rotY));
	quat.mul(hkQuaternion(hkVector4(0.0f, 0.0f, 1.0f), rotZ));

	hkVector4 pos = hkVector4(posX, posY, posZ);

	body->setPositionAndRotation(hkVector4(posX, posY, posZ), quat);

	wheelFL->setPosAndRot(attachFL(0) + pos(0), attachFL(1) + pos(1), attachFL(2) + pos(2), rotX, rotY, rotZ);
	wheelFR->setPosAndRot(attachFR(0) + pos(0), attachFR(1) + pos(1), attachFR(2) + pos(2), rotX, rotY, rotZ);
	wheelRL->setPosAndRot(attachRL(0) + pos(0), attachRL(1) + pos(1), attachRL(2) + pos(2), rotX, rotY, rotZ);
	wheelRR->setPosAndRot(attachRR(0) + pos(0), attachRR(1) + pos(1), attachRR(2) + pos(2), rotX, rotY, rotZ);

	update();
}


void Racer::update()
{
	if (drawable && body)
	{
		D3DXMATRIX transMat;
		(body->getTransform()).get4x4ColumnMajor(transMat);
		drawable->setTransform(&transMat);
		
		// Now update wheels
		(wheelRL->body->getTransform()).get4x4ColumnMajor(transMat);
		wheelRL->drawable->setTransform(&transMat);
		
		(wheelRR->body->getTransform()).get4x4ColumnMajor(transMat);
		wheelRR->drawable->setTransform(&transMat);



		(wheelFL->body->getTransform()).get4x4ColumnMajor(transMat);

		D3DXMATRIX rot1, rot2, trans1;
		D3DXVECTOR3 scale, trans;
		D3DXQUATERNION rot;
		
		D3DXMatrixDecompose(&scale, &rot, &trans, &transMat);
		
		D3DXMatrixRotationQuaternion(&rot1, &rot);
		D3DXMatrixRotationAxis(&rot2, &(drawable->getYVector()), currentSteering * 1.11f);
		D3DXMatrixTranslation(&trans1, trans.x, trans.y, trans.z);
		
		D3DXMatrixMultiply(&transMat, &rot1, &rot2);
		D3DXMatrixMultiply(&transMat, &transMat, &trans1);
		wheelFL->drawable->setTransform(&transMat);

		(wheelFR->body->getTransform()).get4x4ColumnMajor(transMat);

		D3DXMatrixTranslation(&trans1, transMat._41, transMat._42, transMat._43);

		D3DXMatrixMultiply(&transMat, &rot1, &rot2);
		D3DXMatrixMultiply(&transMat, &transMat, &trans1);
		wheelFR->drawable->setTransform(&transMat);
	}
}

int Racer::getIndex()
{
	return index;
}


void Racer::buildConstraint(hkVector4* attachmentPt, hkpGenericConstraintData* constraint, WheelType type)
{
	hkpConstraintConstructionKit* kit = new hkpConstraintConstructionKit();
	kit->begin(constraint);
	kit->setLinearDofA(xAxis, xID);
	kit->setLinearDofB(xAxis, xID);
	kit->setLinearDofA(yAxis, yID);
	kit->setLinearDofB(yAxis, yID);
	kit->setLinearDofA(zAxis, zID);
	kit->setLinearDofB(zAxis, zID);

	kit->setPivotA(hkVector4(0,0,0));
	kit->setPivotB(*attachmentPt);
	kit->setAngularBasisABodyFrame();
	kit->setAngularBasisBBodyFrame();

	kit->constrainLinearDof(xID);
	kit->constrainLinearDof(zID);
	
	//kit->constrainToAngularDof(xID);
	kit->constrainAllAngularDof();

	if (type == FRONT)
	{
		kit->setLinearLimit(yID, -0.15f, 0.35f);
	}
	else
	{
		kit->setLinearLimit(yID, -0.15f, 0.42f);
	}

	kit->end();

	constraint->setSolvingMethod(hkpConstraintAtom::METHOD_STABILIZED);
}

// between -1.0 and 1.0 (backwards is negative)
void Racer::accelerate(float seconds, float value)
{
	if ((value == 0.0f) || ( !(wheelRL->touchingGround) && !(wheelRR->touchingGround)))
		return;

	hkVector4 forward = drawable->getZhkVector();
	hkVector4 vel = body->getMotion()->getLinearVelocity();
	float dot = vel.dot3(drawable->getZhkVector());

	if ((dot > 0.0f) && (value < 0.0f))
	{
		// Braking. Apply force to all wheels, opposite linear velocity
		if (wheelFL->touchingGround)
		{
			forward = wheelFL->body->getLinearVelocity();
			forward.normalize3IfNotZero();
			forward.mul(value * chassisMass * accelerationScale);
			body->applyForce(seconds, forward, wheelFL->body->getPosition());
		}

		if (wheelFR->touchingGround)
		{
			forward = wheelFR->body->getLinearVelocity();
			forward.normalize3IfNotZero();
			forward.mul(value * chassisMass * accelerationScale);
			body->applyForce(seconds, forward, wheelFR->body->getPosition());
		}

		if (wheelRL->touchingGround)
		{
			forward = wheelRL->body->getLinearVelocity();
			forward.normalize3IfNotZero();
			forward.mul(value * chassisMass * accelerationScale);
			body->applyForce(seconds, forward, wheelRL->body->getPosition());
		}

		if (wheelRR->touchingGround)
		{
			forward = wheelRR->body->getLinearVelocity();
			forward.normalize3IfNotZero();
			forward.mul(value * chassisMass * accelerationScale);
			body->applyForce(seconds, forward, wheelRR->body->getPosition());
		}

	}
	else
	{
		forward.mul(value * chassisMass * accelerationScale);

		if (wheelRL->touchingGround)
			body->applyForce(seconds, forward,  wheelRL->body->getPosition());

		if (wheelRR->touchingGround)
			body->applyForce(seconds, forward, wheelRR->body->getPosition());
	}
}


// between -1.0 and 1.0 (left is negative)
void Racer::steer(float seconds, float value)
{
	currentSteering = value;
	/*
	D3DXMATRIX transMat;
	(wheelFL->body->getTransform()).get4x4ColumnMajor(transMat);

		D3DXMATRIX rot1, rot2, trans1;
		D3DXVECTOR3 scale, trans;
		D3DXQUATERNION rot;
		
		D3DXMatrixDecompose(&scale, &rot, &trans, &transMat);
		
		D3DXMatrixRotationQuaternion(&rot1, &rot);
		D3DXMatrixRotationAxis(&rot2, &(drawable->getYVector()), currentSteering * 1.11f);
		D3DXMatrixTranslation(&trans1, trans.x, trans.y, trans.z);
		
		D3DXMatrixMultiply(&transMat, &rot1, &rot2);
		D3DXMatrixMultiply(&transMat, &transMat, &trans1);

		hkTransform transform;
		transform.set4x4ColumnMajor(transMat);
		wheelFL->body->setTransform(transform);
		*/

	
	if (!(wheelFL->touchingGround) && !(wheelFR->touchingGround))
		return;

	hkVector4 vel = body->getMotion()->getLinearVelocity();
	float dot = vel.dot3(drawable->getZhkVector());

	bool negative = false;

	if (dot < 0.0f)
	{
		negative = true;
		dot *= -1;
	}
	
	float torqueScale = 0.0f;
	float centripScale = 0.0f;

	// Now adjust forces
	if (dot < 1.0f)
	{
		torqueScale = 80.0f;
		centripScale = 50.0f;
	}
	else
	{
		torqueScale = 70.0f;
		centripScale = 80.0f;
	}

	if (negative)
	{
		torqueScale *= -1;
		centripScale *= -1;
	}


	hkVector4 force = drawable->getXhkVector();
	force.mul(value * chassisMass * centripScale);
	body->applyForce(seconds, force, body->getPosition());

	force = drawable->getYhkVector();
	force.mul(value * chassisMass * torqueScale);
	body->applyTorque(seconds, force);
}


void Racer::reset()
{
	hkVector4 reset = hkVector4(0.0f, 0.0f, 0.0f);
	setPosAndRot(0.0f, 10.0f, 0.0f, 0, 0, 0);
	body->setLinearVelocity(reset);
	wheelFL->body->setLinearVelocity(reset);
	wheelFR->body->setLinearVelocity(reset);
	wheelRL->body->setLinearVelocity(reset);
	wheelRR->body->setLinearVelocity(reset);
}


void Racer::applySprings(float seconds)
{
	// Applies springs and dampers using the helper function getForce()
	hkVector4 force, point;
	D3DXMATRIX transMat;
	(body->getTransform()).get4x4ColumnMajor(transMat);
	hkVector4 upVector = hkVector4(transMat._21, transMat._22, transMat._23);

	if (wheelFL->touchingGround)
	{
		force = getForce(&upVector,  wheelFL->body, &attachFL, FRONT);
		point.setTransformedPos(body->getTransform(), attachFL);
		body->applyForce(seconds, force, point);
	}

	if (wheelFR->touchingGround)
	{
		force = getForce(&upVector,  wheelFR->body, &attachFR, FRONT);
		point.setTransformedPos(body->getTransform(), attachFR);
		body->applyForce(seconds, force, point);
	}

	if (wheelRL->touchingGround)
	{
		force = getForce(&upVector,  wheelRL->body, &attachRL, REAR);
		point.setTransformedPos(body->getTransform(), attachRL);
		body->applyForce(seconds, force, point);
	}

	if (wheelRR->touchingGround)
	{
		force = getForce(&upVector,  wheelRR->body, &attachRR, REAR);
		point.setTransformedPos(body->getTransform(), attachRR);
		body->applyForce(seconds, force, point);
	}
}


hkVector4 Racer::getForce(hkVector4* up, hkpRigidBody* wheel, hkVector4* attach, WheelType type)
{
	hkVector4 actualPos, restPos, force, damperForce, pointVel;
	float displacement, speedOfDisplacement;

	float k, c;

	if (type == FRONT)
	{
		k = frontSpringK;
		c = frontDamperC;
	}
	else
	{
		k = rearSpringK;
		c = rearDamperC;
	}


	actualPos = wheel->getPosition();
	restPos.setTransformedPos(body->getTransform(), *attach);
	actualPos.sub3clobberW(restPos);

	displacement = actualPos.dot3(*up);
	

	if (type == FRONT)
	{
		if (displacement < -frontExtents)
		{
			displacement = -frontExtents;
		}
		else if (displacement > frontExtents)
		{
			displacement = frontExtents;
		}
	}
	else
	{
		if (displacement < -rearExtents)
		{
			displacement = -rearExtents;
		}
		else if (displacement > rearExtents)
		{
			displacement = rearExtents;
		}
	}

	force = hkVector4(*up);
	force.mul(k * displacement);

	body->getPointVelocity(restPos, pointVel);

	speedOfDisplacement = pointVel.dot3(*up);

	damperForce = hkVector4(*up);
	damperForce.mul(c * -speedOfDisplacement);

	force.add3clobberW(damperForce);

	if (force.dot3(force) > springForceCap*springForceCap)
	{
		float forceMultiplier = springForceCap / ((float)force.length3());
		force.mul(forceMultiplier);
	}
	

	return force;
}



void Racer::applyForces(float seconds)
{
	applySprings(seconds);
	applyFriction(seconds);
}



void Racer::applyFriction(float seconds)
{
	float xFrictionForce = 3.0f * 20 * -chassisMass / 4.0f;
	float zFrictionForce = 1.5f * 20 * -chassisMass / 4.0f;

	hkVector4 point;
	D3DXMATRIX transMat;
	(body->getTransform()).get4x4ColumnMajor(transMat);
	
	hkVector4 xVector = hkVector4(transMat._11, transMat._12, transMat._13);
	hkVector4 zVector = hkVector4(transMat._31, transMat._32, transMat._33);

	if (wheelFL->touchingGround)
	{
		applyFrictionToTire(&attachFL, wheelFL->body, &xVector, &zVector, xFrictionForce, zFrictionForce, seconds);
	}

	if (wheelFR->touchingGround)
	{
		applyFrictionToTire(&attachFR, wheelFR->body, &xVector, &zVector, xFrictionForce, zFrictionForce, seconds);
	}

	if (wheelRL->touchingGround)
	{
		applyFrictionToTire(&attachRL, wheelRL->body, &xVector, &zVector, xFrictionForce, zFrictionForce, seconds);
	}

	if (wheelRR->touchingGround)
	{
		applyFrictionToTire(&attachRR, wheelRR->body, &xVector, &zVector, xFrictionForce, zFrictionForce, seconds);
	}
}


void Racer::applyFrictionToTire(hkVector4* attachPoint, hkpRigidBody* wheelBody,
	hkVector4* xVector, hkVector4* zVector, float xFrictionForce, float zFrictionForce, float seconds)
{
	hkVector4 point, velocity, xForce, zForce;
	point.setTransformedPos(body->getTransform(), *attachPoint);
	velocity = wheelBody->getLinearVelocity();

	xForce = hkVector4(*xVector);
	float xThreshold = 0.3f;
	float zThreshold = 0.3f;

	float dot = velocity.dot3(xForce);

	if (dot < -xThreshold)
	{
		xForce.mul(-xFrictionForce);
		
		body->applyForce(seconds, xForce, point);
	}
	else if (dot > xThreshold)
	{
		xForce.mul(xFrictionForce);
		body->applyForce(seconds, xForce, point);
	}

	zForce = hkVector4(*zVector);
		
	dot = velocity.dot3(zForce);

	if (dot < -zThreshold)
	{
		zForce.mul(-zFrictionForce);
		body->applyForce(seconds, zForce, point);
	}
	else if (dot > zThreshold)
	{
		zForce.mul(zFrictionForce);
		body->applyForce(seconds, zForce, point);
	}
}







WheelListener::WheelListener(bool* touchingGround)
{
	touching = touchingGround;
}


// Wheel listener callback setup
void WheelListener::collisionAddedCallback(const hkpCollisionEvent& ev)
{
	*touching = true;
}

void WheelListener::collisionRemovedCallback(const hkpCollisionEvent& ev)
{
	*touching = false;
}
