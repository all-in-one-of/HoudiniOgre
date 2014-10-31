/*
-----------------------------------------------------------------------------
HoudiniOgre_Mesh.h

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

#ifndef __HoudiniOgre_Skeleton__
#define __HoudiniOgre_Skeleton__

#include "HoudiniOgre_Prerequisites.h"

class OP_Node;
class OBJ_Node;
class OBJ_Bone;
class Ogre::Skeleton;
class Ogre::Bone;
class Ogre::NodeAnimationTrack;

#include "OgreNoMemoryMacros.h"
#include <OBJ/OBJ_Bone.h>
#include <UT/UT_DMatrix4.h>
#include "OgreMemoryMacros.h"
/** Class to perform the export of a .skeleton file.
*/
class HoudiniOgre_Skeleton
{
public:
	/** Constructor, takes a list of bone objects the mesh export has found to be
		of interest.
	*/
	HoudiniOgre_Skeleton(const BoneList& bones);

	virtual ~HoudiniOgre_Skeleton();

	void Export(const Ogre::String& filename, float framesPerSecond, 
		float ikSampleRate, const AnimationList& animList);

protected:
	/** An actual bone to be exported - may not be an OBJ_Bone.
	*/
	struct BoneEntry
	{
		unsigned short boneID;
		OBJ_Node* parent;
		unsigned short parentID;
		std::set<unsigned short> childIDs;
		OBJ_Node* node;
		Ogre::Bone* ogreBone;
		bool isObjBone;
		UT_DMatrix4 invBindXform;

		BoneEntry(unsigned short ID) : boneID(ID), parent(0), node(0), 
			ogreBone(0)
		{
		}
		BoneEntry(unsigned short ID, OBJ_Bone* b, Ogre::Bone* ob) 
			: boneID(ID), parent(0), node(b), ogreBone(ob), isObjBone(true)
		{
		}
		BoneEntry(unsigned short ID, OBJ_Node* n, Ogre::Bone* ob) 
			: boneID(ID), parent(0), node(n), ogreBone(ob), isObjBone(false)
		{
		}
	};    

	const BoneList& mOrigBoneList;

	typedef std::map<OP_Node*, BoneEntry> BoneEntryMap;
	BoneEntryMap mBoneEntryMap;

	void ascendBoneHierarchy(Ogre::Skeleton* skel, OBJ_Node* bone);
	void buildBoneStructure(Ogre::Skeleton* skel);
	void establishInitialTransforms();
	void sampleAnimations(Ogre::Skeleton* skel, const AnimationList& animList, 
		float fps, float sampleRate);
	void sampleTrack(Ogre::NodeAnimationTrack* track, const BoneEntry& boneEntry, 
		float startTime, float endTime, float sampleFreq);
	void sampleKeyframe(Ogre::NodeAnimationTrack* track, const BoneEntry& boneEntry, 
		float time);

};

#endif

