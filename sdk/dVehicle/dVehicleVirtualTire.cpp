/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#include "dStdafxVehicle.h"
#include "dVehicleChassis.h"
#include "dVehicleSingleBody.h"
#include "dVehicleVirtualTire.h"

dVehicleVirtualTire::dVehicleVirtualTire(dVehicleNode* const parent, const dMatrix& locationInGlobalSpace, const dTireInfo& info, const dMatrix& localFrame)
	:dVehicleTireInterface(parent, info)
	,m_joint()
	,m_dynamicContactBodyNode(NULL, true)
	,m_omega(0.0f)
	,m_speed(0.0f)
	,m_position(0.0f)
	,m_tireLoad(0.0f)
	,m_tireAngle(0.0f)
	,m_brakeTorque(0.0f)
	,m_steeringAngle(0.0f)
	,m_invSuspensionLength(m_info.m_suspensionLength > 0.0f ? 1.0f/m_info.m_suspensionLength : 0.0f)
{
	SetWorld(parent->GetWorld());
	m_dynamicContactBodyNode.SetWorld(m_world);

	dVehicleSingleBody* const chassisNode = (dVehicleSingleBody*) m_parent;
	//dVehicleChassis* const chassis = chassisNode->GetChassis();
	NewtonBody* const newtonBody = chassisNode->m_newtonBody;
	//NewtonWorld* const world = NewtonBodyGetWorld(newtonBody);

	m_tireShape = NewtonCreateChamferCylinder(m_world, 0.5f, 1.0f, 0, NULL);
	NewtonCollisionSetScale(m_tireShape, m_info.m_width, m_info.m_radio, m_info.m_radio);

	dMatrix chassisMatrix;
	NewtonBodyGetMatrix(newtonBody, &chassisMatrix[0][0]);

	dMatrix alignMatrix(dGetIdentityMatrix());
	alignMatrix.m_front = dVector (0.0f, 0.0f, 1.0f, 0.0f);
	alignMatrix.m_up = dVector (0.0f, 1.0f, 0.0f, 0.0f);
	alignMatrix.m_right = alignMatrix.m_front.CrossProduct(alignMatrix.m_up);

	m_matrix = alignMatrix * localFrame;
	m_matrix.m_posit = localFrame.UntransformVector(chassisMatrix.UntransformVector(locationInGlobalSpace.m_posit));

	m_bindingRotation = locationInGlobalSpace * (m_matrix * chassisMatrix).Inverse();
	m_bindingRotation.m_posit = dVector (0.0f, 0.0f, 0.0f, 1.0f);

	dVector com;
	dVector inertia(0.0f);
	NewtonConvexCollisionCalculateInertialMatrix(m_tireShape, &inertia[0], &com[0]);
	// simplify calculation by making wheel inertia spherical
	inertia = dVector(m_info.m_mass * dMax(dMax(inertia.m_x, inertia.m_y), inertia.m_z));

	dComplementaritySolver::dBodyState* const chassisBody = GetBody();
	chassisBody->SetMass(m_info.m_mass);
	chassisBody->SetInertia(inertia.m_x, inertia.m_y, inertia.m_z);
	chassisBody->UpdateInertia();

	// set the tire joint
	m_joint.Init (&m_body, m_parent->GetBody());
	m_joint.m_tire = this;

	for (int i = 0 ; i < sizeof (m_contactsJoints) / sizeof (m_contactsJoints[0]) - 1; i ++) {
		m_contactsJoints[i].SetOwners (this, &chassisNode->m_groundNode);
	}
	m_contactsJoints[sizeof (m_contactsJoints) / sizeof (m_contactsJoints[0]) - 1].SetOwners(this, &m_dynamicContactBodyNode);

m_brakeTorque = 100.0f;
}

dVehicleVirtualTire::~dVehicleVirtualTire()
{
	NewtonDestroyCollision(m_tireShape);
}

NewtonCollision* dVehicleVirtualTire::GetCollisionShape() const
{
	return m_tireShape;
}

void dVehicleVirtualTire::CalculateNodeAABB(const dMatrix& matrix, dVector& minP, dVector& maxP) const
{
	CalculateAABB(GetCollisionShape(), matrix, minP, maxP);
}

dComplementaritySolver::dBilateralJoint* dVehicleVirtualTire::GetJoint()
{
	return &m_joint;
}

void dVehicleVirtualTire::RenderDebugTire(void* userData, int vertexCount, const dFloat* const faceVertec, int id)
{
	dCustomJoint::dDebugDisplay* const debugContext = (dCustomJoint::dDebugDisplay*) userData;

	int index = vertexCount - 1;
	dVector p0(faceVertec[index * 3 + 0], faceVertec[index * 3 + 1], faceVertec[index * 3 + 2]);
	for (int i = 0; i < vertexCount; i++) {
		dVector p1(faceVertec[i * 3 + 0], faceVertec[i * 3 + 1], faceVertec[i * 3 + 2]);
		debugContext->DrawLine(p0, p1);
		p0 = p1;
	}
}

dMatrix dVehicleVirtualTire::GetHardpointMatrix (dFloat param) const
{
	dMatrix matrix(dYawMatrix(m_steeringAngle) * m_matrix);
	matrix.m_posit += m_matrix.m_up.Scale(param * m_info.m_suspensionLength - m_info.m_pivotOffset);
	return matrix;
}

dMatrix dVehicleVirtualTire::GetLocalMatrix () const
{
	return m_bindingRotation * dPitchMatrix(m_tireAngle) * GetHardpointMatrix(m_position * m_invSuspensionLength);
}

dMatrix dVehicleVirtualTire::GetGlobalMatrix () const
{
	dMatrix newtonBodyMatrix;

	dVehicleSingleBody* const chassisNode = (dVehicleSingleBody*)m_parent->GetAsVehicle();
	dAssert (chassisNode);
	return GetLocalMatrix() * chassisNode->GetBody()->GetMatrix();
}

void dVehicleVirtualTire::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
	dVehicleTireInterface::Debug(debugContext);

	dMatrix trieMatrix (GetGlobalMatrix ());
	NewtonCollisionForEachPolygonDo(m_tireShape, &trieMatrix[0][0], RenderDebugTire, debugContext);
}

void dVehicleVirtualTire::SetSteeringAngle(dFloat steeringAngle)
{
	m_steeringAngle = steeringAngle;
}

dFloat dVehicleVirtualTire::GetSteeringAngle() const
{
	return m_steeringAngle;
}

dFloat dVehicleVirtualTire::GetBrakeTorque() const
{
	return m_brakeTorque;
}

void dVehicleVirtualTire::SetBrakeTorque(dFloat brakeTorque)
{
	m_brakeTorque = dAbs (brakeTorque);
}

void dVehicleVirtualTire::ApplyExternalForce()
{
	dVehicleSingleBody* const chassisNode = (dVehicleSingleBody*)m_parent;
	dComplementaritySolver::dBodyState* const tireBody = GetBody();
	dComplementaritySolver::dBodyState* const chassisBody = chassisNode->GetBody();

	dMatrix tireMatrix (GetHardpointMatrix (m_position * m_invSuspensionLength) * chassisBody->GetMatrix());
	tireBody->SetMatrix(tireMatrix);

	tireBody->SetOmega(chassisBody->GetOmega() + tireMatrix.m_front.Scale(m_omega));
	tireBody->SetVeloc(chassisBody->CalculatePointVelocity (tireMatrix.m_posit) + tireMatrix.m_up.Scale(m_speed));

	tireBody->SetTorque(dVector (0.0f));
	tireBody->SetForce(chassisNode->m_gravity.Scale (tireBody->GetMass()));

	dVehicleTireInterface::ApplyExternalForce();
}

int dVehicleVirtualTire::GetKinematicLoops(dKinematicLoopJoint** const jointArray)
{
	int count = 0;
	for (int i = 0; i < sizeof (m_contactsJoints) / sizeof (m_contactsJoints[0]); i ++) {
		dKinematicLoopJoint* const loop = &m_contactsJoints[i];
		if (loop->IsActive ()) {
			jointArray[count] = loop;
			dAssert (!loop->GetOwner0()->IsLoopNode());
			if (loop->GetOwner1()->IsLoopNode()) {
				loop->GetOwner1()->SetIndex(-1);
			}
			count ++;
		}
	}
	return dVehicleTireInterface::GetKinematicLoops(&jointArray[count]) + count;
}


void dVehicleVirtualTire::Integrate(dFloat timestep)
{
	dVehicleTireInterface::Integrate(timestep);

	dVehicleSingleBody* const chassis = (dVehicleSingleBody*)m_parent;
	dComplementaritySolver::dBodyState* const chassisBody = chassis->GetBody();
	
	const dMatrix chassisMatrix (chassis->GetBody()->GetMatrix());
	const dMatrix tireMatrix(GetHardpointMatrix(0.0f) * chassisBody->GetMatrix());

	dVector tireOmega(m_body.GetOmega());
	dVector chassisOmega(chassisBody->GetOmega());
	dVector localOmega(tireOmega - chassisOmega);
	m_omega = tireMatrix.m_front.DotProduct3(localOmega);
	// check if the tire is going to rest
	if (dAbs(m_omega) < 0.25f) {
		dFloat alpha = tireMatrix.m_front.DotProduct3(m_body.GetTorque()) * m_body.GetInvInertia()[0][0];
		if (alpha < 0.2f) {
			m_omega = 0.0f;
		}
	}

static int xxx;
xxx++;
if (xxx > 2000) {
if ((m_solverIndex == 2) || (m_solverIndex == 3))
m_omega = -20.0f;
}


	m_tireAngle += m_omega * timestep;
	while (m_tireAngle < 0.0f)
	{
		m_tireAngle += 2.0f * dPi;
	}
	m_tireAngle = dMod(m_tireAngle, dFloat(2.0f * dPi));

	dVector tireVeloc(m_body.GetVelocity());
	dVector chassisVeloc(chassisBody->GetVelocity());
	dVector chassinPointVeloc (chassisVeloc + chassisOmega.CrossProduct(tireMatrix.m_posit - chassisMatrix.m_posit));
	dVector localVeloc (tireVeloc - chassinPointVeloc);

	m_speed = tireMatrix.m_up.DotProduct3(localVeloc);
	m_position = dClamp (m_position + m_speed * timestep, dFloat (0.0f), m_info.m_suspensionLength);
}

void dVehicleVirtualTire::CalculateContacts(const dVehicleChassis::dCollectCollidingBodies& bodyArray, dFloat timestep)
{
	for (int i = 0; i < sizeof(m_contactsJoints) / sizeof(m_contactsJoints[0]); i++) {
		m_contactsJoints[i].ResetContact();
	}

	if (bodyArray.m_count) {
		dVehicleSingleBody* const chassisNode = (dVehicleSingleBody*)m_parent;
		dComplementaritySolver::dBodyState* const chassisBody = chassisNode->GetBody();

		const dMatrix& chassisMatrix = chassisBody->GetMatrix();
		dMatrix tireMatrix (GetHardpointMatrix (1.0f) * chassisMatrix);
		dVector veloc0 (tireMatrix.m_up.Scale (-m_info.m_suspensionLength));
		dVector tmp (0.0f);
		
		dVector contact(0.0f);
		dVector normal(0.0f);
		dFloat penetration(0.0f);

		dFloat param = 1.0f - m_position * m_invSuspensionLength;

		int contactCount = 0;
		for (int i = 0; i < bodyArray.m_count; i ++) {
			dMatrix matrixB;
			dLong attributeA;
			dLong attributeB;
			dFloat impactParam;

			NewtonBodyGetMatrix(bodyArray.m_array[i], &matrixB[0][0]);
			NewtonCollision* const otherShape = NewtonBodyGetCollision (bodyArray.m_array[i]);

			int count = NewtonCollisionCollideContinue(m_world, 1, 1.0f,
				m_tireShape, &tireMatrix[0][0], &veloc0[0], &tmp[0],
				otherShape, &matrixB[0][0], &tmp[0], &tmp[0], 
				&impactParam, &contact[0], &normal[0], &penetration,
				&attributeA, &attributeB, 0);

			if (count) {
				// calculate tire penetration
				dFloat dist = (param - impactParam) * m_info.m_suspensionLength;
				if (dist > -D_TIRE_MAX_ELASTIC_DEFORMATION) {

					normal.m_w = 0.0f;
					penetration = normal.DotProduct3(tireMatrix.m_up.Scale(dist));

					dVector lateralDir (normal.CrossProduct(tireMatrix.m_right));
					if (lateralDir.DotProduct3(lateralDir) < 0.1f) {
						lateralDir = normal.CrossProduct(tireMatrix.m_front.CrossProduct(normal)); 
					}
					lateralDir = lateralDir.Normalize();
					dAssert (lateralDir.DotProduct3(lateralDir) > 0.0f);

					contact -= tireMatrix.m_up.Scale (dist);
					contact.m_w = 1.0f;
					m_contactsJoints[contactCount].SetContact(contact, normal, lateralDir, penetration, 1.0f, 0.8f);
					contactCount ++;
				}
			}
		}
	}
}
