/*
-----------------------------------------------------------------------------
HoudiniOgre_Prerequisites.cpp

Author: Steven Streeting 
Copyright © 2007 Torus Knot Software Ltd & EDM Studio, Inc

This file is part of HoudiniOgre

    HoudiniOgre is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    HoudiniOgre is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

-----------------------------------------------------------------------------
*/
#include "HoudiniOgre_Prerequisites.h"

#include <UT/UT_Vector3.h>
#include <UT/UT_Vector4.h>
#include <UT/UT_DMatrix4.h>
#include <UT/UT_DMatrix3.h>
#include <UT/UT_Quaternion.h>
#include <UT/UT_XformOrder.h>

#include "OgreVector3.h"
#include "OgreMatrix4.h"
#include "OgreMatrix3.h"
#include "OgreHardwareVertexBuffer.h"

//---------------------------------------------------------------------
Ogre::Vector3 HoudiniMappings::toVec3(const UT_Vector4& vec)
{
	return Ogre::Vector3(vec.x(), vec.y(), vec.z());
}
//---------------------------------------------------------------------
Ogre::Vector3 HoudiniMappings::toVec3(const UT_Vector3& vec)
{
	return Ogre::Vector3(vec.x(), vec.y(), vec.z());
}
//---------------------------------------------------------------------
Ogre::RGBA HoudiniMappings::toRGBA(float* inp)
{
	Ogre::ColourValue col(inp[0], inp[1], inp[2], inp[3]); 
	return Ogre::VertexElement::convertColourValue(col, 
		Ogre::VertexElement::getBestColourVertexElementType());

}
//---------------------------------------------------------------------
Ogre::Matrix4 HoudiniMappings::toMat4(const UT_DMatrix4& inMat)
{
	// Houdini's matrices are transposed compared to Ogres
	return Ogre::Matrix4(
		inMat(0,0), inMat(1,0), inMat(2,0), inMat(3,0),
		inMat(0,1), inMat(1,1), inMat(2,1), inMat(3,1),
		inMat(0,2), inMat(1,2), inMat(2,2), inMat(3,2),
		inMat(0,3), inMat(1,3), inMat(2,3), inMat(3,3)
		);

}
//---------------------------------------------------------------------
Ogre::Matrix3 HoudiniMappings::toMat3(const UT_DMatrix3& inMat)
{
	// Houdini's matrices are transposed compared to Ogres
	return Ogre::Matrix3(
		inMat(0,0), inMat(1,0), inMat(2,0), 
		inMat(0,1), inMat(1,1), inMat(2,1), 
		inMat(0,2), inMat(1,2), inMat(2,2)
		);
}
//---------------------------------------------------------------------
void HoudiniMappings::explode(const UT_DMatrix4& inMat, 
	Ogre::Vector3& outScale, Ogre::Quaternion& outRot, Ogre::Vector3& outTrans)
{
	UT_XformOrder xformOrder(UT_XformOrder::SRT, UT_XformOrder::XYZ);

	UT_Vector3 rot, scl, trans;
	inMat.explode(xformOrder, rot, scl, trans);


	Ogre::Quaternion xrot, yrot, zrot;
	xrot.FromAngleAxis(Ogre::Radian(rot.x()), Ogre::Vector3::UNIT_X);
	yrot.FromAngleAxis(Ogre::Radian(rot.y()), Ogre::Vector3::UNIT_Y);
	zrot.FromAngleAxis(Ogre::Radian(rot.z()), Ogre::Vector3::UNIT_Z);

	outScale = HoudiniMappings::toVec3(scl);
	outRot = xrot * yrot * zrot;
	outTrans = HoudiniMappings::toVec3(trans);


}

