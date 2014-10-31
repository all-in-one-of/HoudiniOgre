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
#include "HoudiniOgre_Skeleton.h"

#include "OgreNoMemoryMacros.h"
#include <UT/UT_DMatrix4.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_Quaternion.h>
#include <UT/UT_Matrix3.h>
#include "OgreMemoryMacros.h"

#include "OgreLogManager.h"
#include "OgreSkeletonSerializer.h"
#include "OgreSkeleton.h"
#include "OgreBone.h"
#include "OgreSkeletonManager.h"
#include "OgreAnimation.h"
#include "OgreAnimationTrack.h"
#include "OgreKeyFrame.h"
//---------------------------------------------------------------------
HoudiniOgre_Skeleton::HoudiniOgre_Skeleton(const BoneList& bones)
: mOrigBoneList(bones)
{

}
//---------------------------------------------------------------------
HoudiniOgre_Skeleton::~HoudiniOgre_Skeleton()
{

}
//---------------------------------------------------------------------
void HoudiniOgre_Skeleton::Export(const Ogre::String& filename, float framesPerSecond, 
							 float ikSampleRate, const AnimationList& animList)
{
	Ogre::SkeletonSerializer ser;


	Ogre::SkeletonPtr skeleton = Ogre::SkeletonManager::getSingleton().create(
		"export", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);


	buildBoneStructure(skeleton.get());

	establishInitialTransforms();

	sampleAnimations(skeleton.get(), animList, framesPerSecond, ikSampleRate);

	skeleton->optimiseAllAnimations();

	// export
	ser.exportSkeleton(skeleton.get(), filename);


	Ogre::SkeletonManager::getSingleton().remove(skeleton->getHandle());


}
//---------------------------------------------------------------------
void HoudiniOgre_Skeleton::buildBoneStructure(Ogre::Skeleton* skel)
{
	Ogre::LogManager::getSingleton().logMessage("Building the skeleton structure...");
	/* Apart from the bones themselves, other objects may form part of a 
		skeleton hierarchy and they may be animated. Therefore we have to build a 
		final list of nodes which will become the final skeleton hierarchy.

	*/

	// First copy all the bones into our local map & create basic bones
	unsigned short boneID = 0;
	for (BoneList::const_iterator i = mOrigBoneList.begin(); i != mOrigBoneList.end(); ++i)
	{
		Ogre::Bone* ogreBone = 
			skel->createBone(Ogre::String((*i)->getName()), boneID);

		mBoneEntryMap.insert(BoneEntryMap::value_type(*i, 
			BoneEntry(boneID++, *i, ogreBone)));
	}

	// Now iterate over the same list again (not our new list since we'll add to that)
	// scan up the hierarchy for each one, linking them up or creating new entries
	for (BoneList::const_iterator i = mOrigBoneList.begin(); i != mOrigBoneList.end(); ++i)
	{
		OBJ_Bone* bone = *i;
		ascendBoneHierarchy(skel, bone);
	}

	// Log and link together
	Ogre::LogManager& lmgr = Ogre::LogManager::getSingleton();
	lmgr.logMessage("Bone hierarchy dump:");
	for (BoneEntryMap::iterator i = mBoneEntryMap.begin(); i != mBoneEntryMap.end(); ++i)
	{
		Ogre::StringUtil::StrStreamType msg;
		BoneEntry& be = i->second;
		msg << "Bone " << be.node->getName();
		
		msg << "'s parent is "
			<< (be.parent ? be.parent->getName() : "nothing");

		lmgr.logMessage(msg.str());
	}
	
	Ogre::LogManager::getSingleton().logMessage("Skeleton structure done.");

}
//---------------------------------------------------------------------
void HoudiniOgre_Skeleton::ascendBoneHierarchy(Ogre::Skeleton* skel, OBJ_Node* bone)
{
	BoneEntryMap::iterator bi = mBoneEntryMap.find(bone);
	assert (bi != mBoneEntryMap.end());
	BoneEntry& thisBoneEntry = bi->second;

	OP_Node* parentNode = bone->getInput(0);

	if (parentNode)
	{
		OBJ_Node* parentObjNode = parentNode->castToOBJNode();

		if (parentObjNode)
		{

			// does it already exist?
			BoneEntryMap::iterator pi = mBoneEntryMap.find(parentObjNode);
			
			if (pi == mBoneEntryMap.end())
			{
				// create new
				unsigned short newBoneID = (unsigned short)mBoneEntryMap.size();
				// is it a bone?
				OBJ_Bone* parentBone = parentObjNode->castToOBJBone();

				// Create parent bone and link
				Ogre::Bone* ogreParentBone = skel->createBone(
					Ogre::String(parentObjNode->getName()), newBoneID);
				ogreParentBone->addChild(thisBoneEntry.ogreBone);

				if (parentBone)
				{
					mBoneEntryMap.insert(BoneEntryMap::value_type(parentNode, 
						BoneEntry(newBoneID, parentBone, ogreParentBone)));
				}
				else
				{
					// some other kind of node
					mBoneEntryMap.insert(BoneEntryMap::value_type(parentNode, 
						BoneEntry(newBoneID, parentObjNode, ogreParentBone)));
				}

				thisBoneEntry.parent = parentObjNode;
				thisBoneEntry.parentID = newBoneID;

				Ogre::StringUtil::StrStreamType msg;
				msg << "Added node " << parentObjNode->getName() << " since skeleton is dependent on it.";
				Ogre::LogManager::getSingleton().logMessage(msg.str());
			}
			else
			{
				// link if not linked
				if (!thisBoneEntry.parent)
				{
					thisBoneEntry.parent = parentObjNode;
					thisBoneEntry.parentID = pi->second.boneID;
					pi->second.ogreBone->addChild(thisBoneEntry.ogreBone);

				}
			}



			// Keep going up
			ascendBoneHierarchy(skel, parentObjNode);
		}
	}


}
//-----------------------------------------------------------------------------
void HoudiniOgre_Skeleton::establishInitialTransforms()
{
	// GJ - make temporary so that Linux build is happy
	OP_Context tempCtx(0.0);

	for (BoneEntryMap::iterator i = mBoneEntryMap.begin(); i != mBoneEntryMap.end(); ++i)
	{
		BoneEntry& be = i->second;


		// Get transforms at frame 0, which is the point at which the mesh will
		// be exported in the case of animation
		if (be.parent)
		{
			be.node->getRelativeTransform(*be.parent, be.invBindXform, tempCtx);
		}
		else
		{
			be.invBindXform = be.node->getTransform(tempCtx);
		}

		// Invert
		be.invBindXform.invert();

		Ogre::Vector3 scl, trans;
		Ogre::Quaternion rot;
		HoudiniMappings::explode(be.invBindXform, scl, rot, trans);

		be.ogreBone->setScale(scl);
		be.ogreBone->setOrientation(rot);
		be.ogreBone->setPosition(trans);
		be.ogreBone->setBindingPose();
		

	}


}
//---------------------------------------------------------------------
void HoudiniOgre_Skeleton::sampleAnimations(Ogre::Skeleton* skel, const AnimationList& animList, 
											float fps, float sampleRate)
{
	for (AnimationList::const_iterator i = animList.begin(); i != animList.end(); ++i)
	{
		const AnimationEntry& animEntry =  *i;
		float len = static_cast<float>(animEntry.endFrame - animEntry.startFrame + 1)
			/ fps;
		Ogre::Animation* anim = skel->createAnimation(animEntry.animationName, len);

		float startTime = ((float)animEntry.startFrame) / fps;
		float endTime = startTime + len;
		float sampleFreq = sampleRate / fps;

		for (BoneEntryMap::iterator bi = mBoneEntryMap.begin(); bi != mBoneEntryMap.end(); ++bi)
		{
			const BoneEntry& bEntry = bi->second;
			Ogre::NodeAnimationTrack* track = anim->createNodeTrack(bEntry.boneID, bEntry.ogreBone);

			sampleTrack(track, bEntry, startTime, endTime, sampleFreq);
		}


	}


}
//---------------------------------------------------------------------
void HoudiniOgre_Skeleton::sampleTrack(Ogre::NodeAnimationTrack* track, 
	const BoneEntry& be, float startTime, float endTime, float sampleFreq)
{
	for (float t = startTime; t < endTime; t += sampleFreq)
	{
		// Sample bones at this point
		float keyTime = t - startTime;

		sampleKeyframe(track, be, keyTime);

	}

	// Sample final frame always
	sampleKeyframe(track, be, endTime);

}
//---------------------------------------------------------------------
void HoudiniOgre_Skeleton::sampleKeyframe(Ogre::NodeAnimationTrack* track, 
	const BoneEntry& be, float keyTime)
{
	OP_Context ctx(keyTime);
	UT_DMatrix4 xform;

	// Get transforms at frame 0, which is the point at which the mesh will
	// be exported in the case of animation
	if (be.parent)
	{
		be.node->getRelativeTransform(*be.parent, xform, ctx);
	}
	else
	{
		xform = be.node->getTransform(ctx);
	}

	// Make relative to bind transform
	// Remember Houdini uses transposed matrix layout to Ogre
	UT_DMatrix4 relativeToBindXform = xform * be.invBindXform;

	Ogre::Vector3 scl, trans;
	Ogre::Quaternion rot;
	HoudiniMappings::explode(relativeToBindXform, scl, rot, trans);

	Ogre::TransformKeyFrame* kf = track->createNodeKeyFrame(keyTime);

	kf->setScale(scl);
	kf->setRotation(rot);
	kf->setTranslate(trans);

}


