#include "FrontWheel.h"


FrontWheel::FrontWheel(IDirect3DDevice9* device, int filter)
{
	drawable = new Drawable(FRONTWHEEL, "tire.dds", device);

	hkVector4 startAxis = hkVector4(-0.25f, 0, 0);
	hkVector4 endAxis = hkVector4(0.25f, 0, 0);
	hkReal radius = 0.4f;

	hkpRigidBodyCinfo info;
	hkVector4 halfExtent(0.15f, 0.3f, 0.3f);		//Half extent for wheel rigid body box
	info.m_shape = new hkpCylinderShape(startAxis, endAxis, radius);
	info.m_restitution = 0.2f;
	hkReal wheelMass = 25.0f;
	info.m_qualityType = HK_COLLIDABLE_QUALITY_MOVING;
	hkpMassProperties massProperties;
	hkpInertiaTensorComputer::computeCylinderVolumeMassProperties(startAxis, endAxis, radius, wheelMass, massProperties);
	info.setMassProperties(massProperties);
	info.m_collisionFilterInfo = hkpGroupFilter::calcFilterInfo(hkpGroupFilterSetup::LAYER_DYNAMIC, filter);
	body = new hkpRigidBody(info);		//Create rigid body
	info.m_friction = 3.0f;
	info.m_shape->removeReference();


	
}


FrontWheel::~FrontWheel(void)
{
	if (body)
	{
		body->removeReference();
	}
}
