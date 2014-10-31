/*
-----------------------------------------------------------------------------
HoudiniOgre_ROP.h

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

#ifndef __HoudiniOgre_ROP__
#define __HoudiniOgre_ROP__

#include "HoudiniOgre_Prerequisites.h"

#include "OgreLogManager.h"
#include "OgreDefaultHardwareBufferManager.h"
#include "OgreMeshManager.h"
#include "OgreMaterialManager.h"
#include "OgreSkeletonManager.h"
// Houdini includes
#include "OgreNoMemoryMacros.h"
#include <ROP/ROP_Node.h>
#include "OgreMemoryMacros.h"

// Forward decls
class OP_Network;
class OP_OperatorTable;
class OP_Operator;
class PRM_TemplatePair;
class IFD_RenderDefinition;



/** ROP specialisation.
@remarks
The purpose of this class is to allow the creation of custom output driver
objects pertaining to the OGRE Export, which in turn allows custom parameters
to be set for rendering. 
*/
class HoudiniOgre_ROP : public ROP_Node
{
public:


	/// Overridden from ROP_Node
	unsigned disableParms();

	/** Static factory method called by Houdini when creating this object.
	*/
	static OP_Node* myConstructor(OP_Network *net, const char*name,	
		OP_Operator *op);

	/** Static method for constructing a new operator to be plugged into Houdini. */
	static void	installOperator(OP_OperatorTable *table);

	/*** Static method to build and return parameters as a template pair */
	static OP_TemplatePair * getCustomTemplatePair();

	int startRender(int, float, float);
	ROP_RENDER_CODE renderFrame(float, UT_Interrupt*);
	ROP_RENDER_CODE endRender();

protected:
	// no public construction
	HoudiniOgre_ROP(OP_Network *net, const char *name, OP_Operator *entry);
	virtual ~HoudiniOgre_ROP();


	int extractParams();
	int exportGeometries(float t, bool snapshotting, int numFrames = 1, int frameStart = 0);
	void createSingletons();
	void cleanUpSingletons();

	
	Ogre::LogManager* mLogMgr;	
	Ogre::ResourceGroupManager* mResMgr;	
	Ogre::MeshManager* mMeshMgr;	
	Ogre::SkeletonManager* mSkelMgr;	
	Ogre::MaterialManager* mMatMgr;	
	Ogre::DefaultHardwareBufferManager* mBufMgr;

	UT_String mOutputPath;
	bool mExportMeshPerObject;
	bool mSnapshotPerFrame;
	bool mObjectTransforms;
	bool mGenerateTangents;
	Ogre::VertexElementSemantic mTangentsSemantic;
	bool mGenerateEdgeLists;
	float mFps;
	float mIkSampleRate;


};

#endif
