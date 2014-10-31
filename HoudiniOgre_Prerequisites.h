/*
-----------------------------------------------------------------------------
HoudiniOgre_Prerequisites.h

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
#ifndef __HoudiniOgre_Prerequisites_H__
#define __HoudiniOgre_Prerequisites_H__
// Things that all classes need
#include <UT/UT_Vector4.h>

#include "OgreColourValue.h"
#include "OgreString.h"

class Ogre::Vector3;
class Ogre::Matrix4;
class Ogre::Matrix3;
class UT_Vector3;
class UT_Vector4;
class UT_DMatrix4;
class UT_DMatrix3;
class OBJ_Bone;


/// Useful conversions
class HoudiniMappings
{
public:
	static Ogre::Vector3 toVec3(const UT_Vector4& vec);
	static Ogre::Vector3 toVec3(const UT_Vector3& vec);
	static Ogre::RGBA toRGBA(float*);
	static Ogre::Matrix4 toMat4(const UT_DMatrix4& inMat);
	static Ogre::Matrix3 toMat3(const UT_DMatrix3& inMat);
	static void explode(const UT_DMatrix4& inMat, 
		Ogre::Vector3& outScale, Ogre::Quaternion& outRot, Ogre::Vector3& outTrans);
};

/// List of bones
typedef std::vector<OBJ_Bone*> BoneList;


/** An entry for animation; allows the user to split the timeline into 
multiple separate animations. 
*/
struct AnimationEntry
{
	Ogre::String animationName;
	long startFrame; 
	long endFrame; 
};
/// List of animations
typedef std::list<AnimationEntry> AnimationList;


#endif


