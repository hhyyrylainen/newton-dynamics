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

dVehicleSingleBody::dVehicleSingleBody(dVehicleChassis* const chassis)
	:dVehicleInterface(chassis)
{
}

dVehicleSingleBody::~dVehicleSingleBody()
{
}

dVehicleTireInterface* dVehicleSingleBody::AddTire (const dVector& locationInGlobalSpace, const dVehicleTireInterface::dTireInfo& tireInfo)
{
	return new dVehicleVirtualTire(this, locationInGlobalSpace, tireInfo);
}

dMatrix dVehicleSingleBody::GetMatrix () const
{
	dMatrix matrix;
	NewtonBody* const chassisBody = m_chassis->GetBody();
	NewtonBodyGetMatrix(chassisBody, &matrix[0][0]);
	return matrix;
}

void dVehicleSingleBody::InitRigiBody(dFloat timestep)
{



	dVehicleInterface::InitRigiBody(timestep);
}
