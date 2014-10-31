/*
-----------------------------------------------------------------------------
HoudiniOgre_Mesh.cpp

Author: Steven Streeting 
Copyright © 2007 Torus Knot Software Ltd & EDM Studio Inc

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

#include "HoudiniOgre_Mesh.h"
#include "HoudiniOgre_Skeleton.h"

#include "OgreNoMemoryMacros.h"
#include <GU/GU_Detail.h>
#include <GU/GU_DetailHandle.h>
#include <GEO/GEO_Primitive.h>
#include <GEO/GEO_PrimPoly.h>
#include <GEO/GEO_CaptureData.h>
#include <OP/OP_Node.h>
#include <OP/OP_Operator.h>
#include <OP/OP_Director.h>
#include <OP/OP_Utils.h>
#include <OP/OP_Network.h>
#include <PRM/PRM_Parm.h>
#include <SOP/SOP_Node.h>
#include <SOP/SOP_CaptureData.h>
#include <GB/GB_Primitive.h>
#include <GB/GB_Group.h>
#include <GB/GB_ElementTree.h>
#include <GEO/GEO_AttributeHandle.h>
#include <UT/UT_DMatrix4.h>
#include <UT/UT_DMatrix3.h>
#include <OBJ/OBJ_Node.h>
#include <OBJ/OBJ_Bone.h>
#include "OgreMemoryMacros.h"

#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include "OgreMeshManager.h"
#include "OgreMeshSerializer.h"
#include "OgreHardwareBufferManager.h"
#include "OgreSubMesh.h"
#include "OgreMatrix4.h"
#include "OgreMatrix3.h"

#undef max
#undef min
//-----------------------------------------------------------------------
HoudiniOgre_Mesh::UniqueVertex::UniqueVertex()
: position(Ogre::Vector3::ZERO), normal(Ogre::Vector3::ZERO), colour(0), nextIndex(0)
{
	for (int i = 0; i < OGRE_MAX_TEXTURE_COORD_SETS; ++i)
		uv[i] = Ogre::Vector3::ZERO;
}
//-----------------------------------------------------------------------
bool HoudiniOgre_Mesh::UniqueVertex::operator==(const UniqueVertex& rhs) const
{
	bool ret = position == rhs.position && 
		normal == rhs.normal && 
		colour == rhs.colour;
	if (!ret) return ret;

	for (int i = 0; i < OGRE_MAX_TEXTURE_COORD_SETS && ret; ++i)
	{
		ret = ret && (uv[i] == rhs.uv[i]);
	}

	return ret;


}
//-----------------------------------------------------------------------
//---------------------------------------------------------------------
HoudiniOgre_Mesh::HoudiniOgre_Mesh()
{

}
//---------------------------------------------------------------------
HoudiniOgre_Mesh::~HoudiniOgre_Mesh()
{

}
//---------------------------------------------------------------------
void HoudiniOgre_Mesh::addGeometry(const OP_Node* objNode, bool snapshotting,
								   float frameTime, bool useObjectTransforms, 
								   int numFrames, int frameStart, 
								   float fps, float ikSampleRate)
{
	if (mpMesh.isNull())
	{
		mpMesh = Ogre::MeshManager::getSingleton().createManual("HoudiniExport", 
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	}


	// First derive the geometry we need to look at. 
	// Originally, I was taking tha geometry pre-deformation if we're animating, 
	// because I wanted the base mesh. However, Houdini only stores 'capture' transforms
	// on bones, whilst we need capture transforms on everything that could be 
	// affecting those bones too (like nulls acting as centres or mid-skeleton offsets)
	// This is because we need to recreate the animation, not just sample the
	// outcomes which is what Houdini expects.
	// When it comes down to it, skeletal animation is about deltas, so as long
	// as we are consistent about the 'bind' pose of both mesh and skeleton, we
	// can pick any pose at all to represent it. So instead, we'll pick a frame
	// and sample the final mesh and skeleton (with nulls and such) at that point,
	// and express all animation as a delta to that. 

	/* This code redundant because of the above 
	GU_Detail *guDetail;
	SOP_Node* captureSOP = 0;
	GU_DetailHandleAutoReadLock* gdl = 0;
	OP_Node *deformerNode = OPfindOpInput(objNode->castToOBJNode()->getDisplaySopPtr(), "deform");
	if (snapshotting || !deformerNode)
	{
		// we want the final geometry
		guDetail = const_cast<GU_Detail*>(
			objNode->castToOBJNode()->castToOBJGeometry()->getRenderGeometry(frameTime));
	}
	else
	{
		// we want pre-deformation
		captureSOP = CAST_SOPNODE(deformerNode->getInput(0));

		gdl = new GU_DetailHandleAutoReadLock(captureSOP->getCookedGeoHandle(OP_Context(0.0)));
		guDetail = const_cast<GU_Detail*>(gdl->getGdp());
	}*/
	//GU_Detail *guDetail = const_cast<GU_Detail*>(
	//	objNode->castToOBJNode()->castToOBJGeometry()->getRenderGeometry(frameTime));

	//GU_DetailHandleAutoReadLock gdl(objNode->castToOBJNode()->castToOBJGeometry()->getRenderGeometryHandle(frameTime));

	// Use Display flag (blue) rather than Render flag (purple)
	GU_DetailHandleAutoReadLock gdl(objNode->castToOBJNode()->getDisplayGeometryHandle(frameTime));

	GU_Detail *guDetail = const_cast<GU_Detail*> (gdl.getGdp());
	// const GU_Detail *guDetail = gdl.getGdp();  // SESI prefers this ... but it causes problems later on.

	if (!preprocessGeometry(guDetail, objNode, frameTime))
		return;

	Ogre::LogManager& lmgr = Ogre::LogManager::getSingleton();

	int primCount = guDetail->primitives().entries();
	int pointCount = guDetail->points().entries();

	lmgr.logMessage("Number of primitives: " + Ogre::StringConverter::toString(primCount));
	lmgr.logMessage("Number of points: " + Ogre::StringConverter::toString(pointCount));
	lmgr.logMessage("Has normals:" + Ogre::StringConverter::toString(mCurrentHasNormals) + 
		" on " + (mNormalsOnVertices ? "vertices" : "points"));
	Ogre::StringUtil::StrStreamType uvstr;
	uvstr << "Number of UV sets:" << mCurrentTextureCoordDimensions.size();
	for (int t = 0; t < mCurrentTextureCoordDimensions.size(); ++t)
	{
		uvstr << "(" << (mUVOnVertices[t] ? "vertices" : "points") << ") ";
	}
	lmgr.logMessage(uvstr.str()); 
	lmgr.logMessage("Has vertex colours:" + Ogre::StringConverter::toString(mCurrentHasVertexColours) + 
		" on " + (mDiffuseOnVertices ? "vertices" : "points"));


	if (mCurrentTextureCoordDimensions.size() > OGRE_MAX_TEXTURE_COORD_SETS)
	{
		// too many texture coordinates!
		Ogre::StringUtil::StrStreamType str;
		str << "Geometry object '" << objNode->getName()
			<< "' has too many texture coordinate sets (" 
			<< mCurrentTextureCoordDimensions.size()
			<< "); the limit is " << OGRE_MAX_TEXTURE_COORD_SETS;

		OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, str.str(), 
			"HoudiniOgre_Mesh::addGeometry");

	}

	// Bounds calculation
	Ogre::Real squaredRadius = 0.0f;
	Ogre::Vector3 min, max;
	bool first = true;
	// temp vertex storage
	UniqueVertex vertex;


	// OP_Context apparantly only uses the myTime param
	OP_Context context(frameTime);
	// sigh - more const-casting required due to const correctness issues in Houdini
	const UT_DMatrix4& hxform = const_cast<OP_Node*>(objNode)->getWorldTransform(context);
	Ogre::Matrix4 xform = HoudiniMappings::toMat4(hxform);
	UT_DMatrix3 hxrot;
	hxform.extractRotate(hxrot);
	Ogre::Matrix3 xrot = HoudiniMappings::toMat3(hxrot);


	// Iterate over all the primitives (polygons)
	for (int iprim = 0; iprim < primCount; ++iprim)
	{
		const GEO_Primitive* prim = guDetail->primitives()(iprim);
#if _DEBUG
		lmgr.logMessage("Primitive " + Ogre::StringConverter::toString(iprim));
#endif
		// only support polys for the moment
		if (prim->getPrimitiveId() == GEOPRIMPOLY)
		{

			const GEO_PrimPoly* primPoly = static_cast<const GEO_PrimPoly*>(prim);
			if (!primPoly->isDegenerate())
			{
				unsigned vcount = primPoly->getVertexCount();
#if _DEBUG
				lmgr.logMessage("Primitive is a non-degenerate polygon, vertex count=" + Ogre::StringConverter::toString(vcount));
#endif
				// Firstly, let's check which proto we're adding this to
				ProtoSubMesh* currentProto = mMainProtoMesh;
				PrimitiveToProtoSubMeshList::iterator polyi = 
					mPrimitiveToProtoSubMeshList.find(iprim);
				if (polyi != mPrimitiveToProtoSubMeshList.end())
				{
					currentProto = polyi->second;
				}
				// has this mesh been used in this proto before? if not set offset
				size_t positionIndexOffset;
				if (currentProto->lastMeshEntry == guDetail)
				{
					positionIndexOffset = currentProto->lastMeshIndexOffset;
				}
				else
				{
					// first time this has been used
					// since we assume we 100% process each polygon mesh before the next,
					// just use last pointer since faster in this section
					currentProto->lastMeshEntry = guDetail;
					positionIndexOffset = currentProto->indices.size();
					currentProto->lastMeshIndexOffset = positionIndexOffset;
					// Also have to store this for future reference
					currentProto->geometryOffsetMap[guDetail] = positionIndexOffset;
				}

				size_t firstIndex, prevIndex;
				for (unsigned vi = 0; vi < vcount; ++vi)
				{
					// Do the indexes in reverse order since Houdini seems to use
					// D3D-style vertex winding
					unsigned reverseIndex = vcount - vi - 1;

					// Vertex is the unique vertex with attributes used by this poly
					// but it's based on a point which may be shared between multiple polys
					const GEO_Vertex& hVertex = primPoly->getVertex(reverseIndex);
					// The unique (positional) point
					GB_Element* elem = hVertex.getBasePt();
					// elem->getNum indicates the unique point number on the geom

					int origPointIndex = elem->getNum(); // unique position index
					// adjust index per offset, this makes position indices unique
					// per polymesh in the same protosubmesh
					long adjustedPosIndex = origPointIndex + positionIndexOffset;

					// All normal, UV and colour attribs are on point
					// Vertex (a point on a face) only seems to have point number as an attrib
					const GEO_Point* point = guDetail->points()(origPointIndex);
					// Get position
					vertex.position = HoudiniMappings::toVec3(hVertex.getPos());

					// Apply instance transform
					if (useObjectTransforms)
					{
						vertex.position = xform * vertex.position;
					}

					// Get normal, if applicable
					if (mCurrentHasNormals)
					{
						UT_Vector3* norm;
						if (mNormalsOnVertices)
							norm = static_cast<UT_Vector3*>(hVertex.getAttribData(mNormalAttrib));
						else
							norm = static_cast<UT_Vector3*>(point->getAttribData(mNormalAttrib));
						vertex.normal = HoudiniMappings::toVec3(*norm);
						// Apply global rotation
						if (useObjectTransforms)
						{
							vertex.normal = xrot * vertex.normal;
						}
					}

					for (size_t i = 0; i < mCurrentTextureCoordDimensions.size(); ++i)
					{
						float* uv;
						if (mUVOnVertices[i]) 
							uv = static_cast<float*>(hVertex.getAttribData(mUVAttribs[i]));
						else
							uv = static_cast<float*>(point->getAttribData(mUVAttribs[i]));
						vertex.uv[i].x = uv[0];
						vertex.uv[i].y = 1.0 - uv[1];

					}

					if (mCurrentHasVertexColours)
					{
						float* col;
						if (mDiffuseOnVertices)
							col = static_cast<float*>(hVertex.getAttribData(mDiffseAttrib));
						else
							col = static_cast<float*>(point->getAttribData(mDiffseAttrib));
						vertex.colour = HoudiniMappings::toRGBA(col);
					}


					size_t index = createOrRetrieveUniqueVertex(
						currentProto, adjustedPosIndex, true, vertex);

					// Here we need to deal with the fact that Houdini's polygons 
					// are not triangles, necessarily
					if (vi >= 3)
					{
						// re-issue the first and previous vertices
						// this creates a fan, we might want to do something more
						// clever here eventually
						currentProto->indices.push_back(firstIndex);
						currentProto->indices.push_back(prevIndex);
					}
					currentProto->indices.push_back(index);
					prevIndex = index;
					if (vi == 0)
						firstIndex = index;

					// bounds
					if (first)
					{
						squaredRadius = vertex.position.squaredLength();
						min = max = vertex.position;
						first = false;
					}
					else
					{
						squaredRadius = 
							std::max(squaredRadius, vertex.position.squaredLength());
						min.makeFloor(vertex.position);
						max.makeCeil(vertex.position);
					}


#if _DEBUG
					Ogre::StringUtil::StrStreamType str;
					str << "Vertex " << vi << " has base point num " << elem->getNum()
						<< " and position " << vertex.position << "";
					lmgr.logMessage(str.str());
#endif


				}

			}

		}
	}

	// Merge bounds
	Ogre::AxisAlignedBox box;
	box.setExtents(min, max);
	box.merge(mpMesh->getBounds());
	mpMesh->_setBounds(box);
	mpMesh->_setBoundingSphereRadius(
		std::max(
		mpMesh->getBoundingSphereRadius(), 
		Ogre::Math::Sqrt(squaredRadius)));

	processBoneAssignments(objNode, guDetail);

	// pull out animation cycles IF we're exporting meshes once and not per-frame
	if (!snapshotting && numFrames > 1)
		processAnimationCycles(objNode, numFrames, frameStart, fps);

	// Post-process the mesh
	postprocessGeometry(guDetail, objNode);

}
//---------------------------------------------------------------------
bool HoudiniOgre_Mesh::preprocessGeometry(GU_Detail* guDetail, const OP_Node* objNode,
										  float frameTime)
{

	Ogre::LogManager& lmgr = Ogre::LogManager::getSingleton();
	// determine the geometry format

	// try vertices first
	// NB Houdini doesn't support per-vertex normals yet, but we might as well
	// cope with this since it's a popular RFE
	mNormalAttrib = guDetail->findVertexAttrib("N", sizeof(UT_Vector3), GB_ATTRIB_VECTOR);
	if (mNormalAttrib >= 0)
	{
		mCurrentHasNormals = true;
		mNormalsOnVertices = true;
	}
	else
	{
		mNormalAttrib = guDetail->findPointAttrib("N", sizeof(UT_Vector3), GB_ATTRIB_VECTOR);
		if (mNormalAttrib >= 0)
		{
			mCurrentHasNormals = true;
			mNormalsOnVertices = false;
		}
		else
		{
			mCurrentHasNormals = false;
		}
	}


	mCurrentTextureCoordDimensions.clear();
	mUVAttribs.clear();
	mUVOnVertices.clear();
	for (int uvset = 0; uvset < OGRE_MAX_TEXTURE_COORD_SETS; ++uvset)
	{
		Ogre::StringUtil::StrStreamType uvname;
		uvname << "uv";
		if (uvset > 0) // no uv1, just uv
		{
			uvname << (uvset + 1);
		}
		// Try to find on vertices first
		int uvAttrib = guDetail->findVertexAttrib(uvname.str().c_str(), 
			3*sizeof(float), GB_ATTRIB_FLOAT);
		bool uvsOnVertices = true;
		if (uvAttrib < 0)
		{
			uvsOnVertices = false;
			uvAttrib = guDetail->findPointAttrib(uvname.str().c_str(), 
				3*sizeof(float), GB_ATTRIB_FLOAT);
			// no good?
			if (uvAttrib < 0)
				break;
		}
		// TODO: determine actual size of UV sets?
		// Houdini seems to store 3D UVs all the time even when they're not useful
		mCurrentTextureCoordDimensions.push_back(2);
		mUVAttribs.push_back(uvAttrib);
		mUVOnVertices.push_back(uvsOnVertices);

	}

	mDiffseAttrib = guDetail->findVertexAttrib("Cd", 3*sizeof(float), GB_ATTRIB_FLOAT);
	if (mDiffseAttrib >= 0)
	{
		mCurrentHasVertexColours = true;
		mDiffuseOnVertices = true;
	}
	else
	{
		mDiffseAttrib = guDetail->findPointAttrib("Cd", 3*sizeof(float), GB_ATTRIB_FLOAT);
		if (mDiffseAttrib >= 0)
		{
			mCurrentHasVertexColours = true;
			mDiffuseOnVertices = false;
		}
		else
		{
			mCurrentHasVertexColours = false;
		}
	}

	// Create any ProtoSubMeshes which don't exist yet for the 
	// materials in question, and define the PolygonCluster map
	// Main material (will never exist if not merging submeshes)
	// Get global shader name
	// Houdini's not very good at const correctness, have to cast away const here
	PRM_Parm& shaderParm = const_cast<PRM_Parm&>(objNode->getParm("shop_surfacepath"));
	UT_String shaderName;
	shaderParm.getValue(frameTime, shaderName, 0, 0);

	Ogre::String materialName = Ogre::String(shaderName);
	lmgr.logMessage("Object surface shader: " + materialName);


	// TODO: potentially register this material if we want to export it later.

	mMainProtoMesh = createOrRetrieveProtoSubMesh(
		materialName, 
		Ogre::String(objNode->getName()),
		mCurrentHasNormals,
		mCurrentTextureCoordDimensions, 
		mCurrentHasVertexColours);

	// Now go looking for 'shop' SOPs which can change materials per poly

	for (int c = 0; c < objNode->getNchildren(); ++c)
	{
		OP_Node* child = objNode->getChild(c);
		OP_Operator* op = child->getOperator();
		// Hmm, all nodes appear to be of OpType "SOP", whilst I need to look for 
		// the detail type ie 'shop'
		lmgr.logMessage("Child " + Ogre::String(child->getName()) + " is of type " 
			+ Ogre::String(child->getOpType()) + " and opname " + Ogre::String(op->getName()));

		// Shader operator can change shaders per sub-object, we must respect that
		if (op->getName() == "shop")
		{
			// You can specify a new shader one of 2 ways, in the generic 'surface'
			// property (quicksurface) or in the 'Primary' tab 'Surface Shader'
			// property (surfpath1). To be honest I'm not sure which should 'win'
			// if both are specified, so I'm going to use the quicksurface if its there
			PRM_Parm& quicksurface = child->getParm("quicksurface");
			UT_String val;
			quicksurface.getValue(frameTime, val, 0, 0);
			if (val == "")
			{
				// try surfpath1
				PRM_Parm& surfpath1 = child->getParm("surfpath1");
				surfpath1.getValue(frameTime, val, 0, 0);
			}

			if (val != "")
			{
				Ogre::String subMaterial(val);
				// does it differ from main material?
				if (subMaterial != materialName)
				{
					lmgr.logMessage("specialised material: " + subMaterial);

					// TODO - register this material if we want to exoprt it later

					ProtoSubMesh* ps = createOrRetrieveProtoSubMesh(
						subMaterial,
						Ogre::String(child->getName()), // name submesh based on SOP
						mCurrentHasNormals,
						mCurrentTextureCoordDimensions, 
						mCurrentHasVertexColours);


					// Get a list of primitive groups that are using this shader
					SOP_Node* sopNode = child->castToSOPNode();
					GB_PrimitiveGroup* gbPrim = sopNode->parsePrimitiveGroupsCopy("*", guDetail);
					unsigned numEntries = gbPrim->entries();
					GB_ElementTree* tree =  gbPrim->ordered();
					Ogre::StringUtil::StrStreamType str;
					str << "Primitives referenced: ";
					for (const GB_Element* cur = tree->head(); cur != 0; cur = tree->next(cur))
					{
						// Create a mapping from this primitive to the proto
						mPrimitiveToProtoSubMeshList[cur->getNum()] = ps;
						str << cur->getNum() << " ";
					}
					lmgr.logMessage(str.str());

					// clean up 
					sopNode->destroyAdhocGroup(gbPrim);
				}
			}

		}

	}

	return true;

}
//-----------------------------------------------------------------------
void HoudiniOgre_Mesh::postprocessGeometry(GU_Detail* guDetail, const OP_Node* objNode)
{
	// clear all position index remaps, incase merged
	for (MaterialProtoSubMeshMap::iterator m = mMaterialProtoSubmeshMap.begin();
		m != mMaterialProtoSubmeshMap.end(); ++m)
	{

		for (ProtoSubMeshList::iterator p = m->second->begin();
			p != m->second->end(); ++p)
		{
			ProtoSubMesh* ps = *p;
			ps->posIndexRemap.clear();
		}
	}

	mPrimitiveToProtoSubMeshList.clear();
	mCurrentTextureCoordDimensions.clear();
	mUVAttribs.clear();
	mUVOnVertices.clear();

}
//-----------------------------------------------------------------------
void HoudiniOgre_Mesh::processBoneAssignments(const OP_Node* objNode, GU_Detail* origGuDetail)
{
	Ogre::LogManager& lmgr = Ogre::LogManager::getSingleton();
	lmgr.logMessage("Looking for bone assignments...");

	OP_Node *deformerNode = OPfindOpInput(objNode->castToOBJNode()->getDisplaySopPtr(), "deform");

	if (!deformerNode)
	{
		lmgr.logMessage("No deform SOP found, no bone assignments.");
		return;
	}

	// we want pre-deformation
	SOP_Node* captureSOP = CAST_SOPNODE(deformerNode->getInput(0));
	if (!captureSOP)
	{
		lmgr.logMessage("No inputs to deform node, no bone assignments.");
		return;
	}

	OP_Context tempCtx(0.0);
	GU_DetailHandleAutoReadLock gdl = 
		GU_DetailHandleAutoReadLock(captureSOP->getCookedGeoHandle(tempCtx));
	GU_Detail* guDetail = const_cast<GU_Detail*>(gdl.getGdp());

	// Now build a capture data helper to assist us getting nodes later.
	SOP_CaptureData capture_data;
	capture_data.initialize(captureSOP, guDetail);


	UT_String sop_path, root_path;
	guDetail->getCaptureRegionRootPath( root_path );
	root_path += "/";

	int numRegions = capture_data.getNumRegions();
	lmgr.logMessage("Number of capture regions: " + Ogre::StringConverter::toString(numRegions));
	// Probably should never happen, but lets be safe
	if (numRegions == 0)
	{
		lmgr.logMessage("No capture regions found, no bone assignments.");
		return;
	}

	// mapping from local region index to global bone index
	unsigned short* globalBoneIndexes = new unsigned short[numRegions];
	for (int b = 0; b < numRegions; ++b)
	{
		sop_path = root_path;
		sop_path += capture_data.getSOPPath(b);
		// Have to remove trailing digits since this path includes '0' on the end for cregion primitive
		// which is not part of the cregion object name
		sop_path.removeTrailingDigits();
		OP_Node* cregion = captureSOP->findNode(sop_path);
		OBJ_Bone* bone = cregion->getCreator()->castToOBJNode()->castToOBJBone();
		//UT_String s = bone->getName();
		Ogre::StringUtil::StrStreamType msg;
		msg << "Bone found: " << bone->getName();
		lmgr.logMessage(msg.str());

		// locate in list, or add
		unsigned short boneIndex;
		for (boneIndex = 0; boneIndex < mBoneList.size(); ++boneIndex)
		{
			if (mBoneList[boneIndex] == bone)
				break;
		}
		if (boneIndex == mBoneList.size())
		{
			// not found, insert
			mBoneList.push_back(bone);
		}

		// Record the mapping from local region index to global bone index
		globalBoneIndexes[b] = boneIndex;
	}

	// You can get the capture attribute like this:
	// int  blendInfoAttr = guDetail->findPointCaptureAttribute();
	// And a pointer to the data like this:
	// float* blendInfoPtr = (float*)guDetail->points()(17)->getAttribData(blendInfoAttr);
	// - even numbers in the array are region indexes, odds are weights
	// BUT - no way to find out how many there are? Seems like they are null-terminated
	// but I'm not entirely sure that's guaranteed, might just be buffer content fluke
	// Therefore use getCaptureWeights which gives definitive list size

	
	UT_IntArray regionArray;
	UT_FloatArray weightArray;

	unsigned int numPoints = guDetail->points().entries();
	for (unsigned int p = 0; p < numPoints; ++p)
	{
		const GEO_Point* point = guDetail->points()(p);
		guDetail->getCaptureWeights(point, regionArray, weightArray);
		int pointIndex = point->getNum();

		// Need to a) translate the region index back to a bone, and b) propagate
		// this bone assignment to all copies of vertex in proto

		for (int c = 0; c < regionArray.entries(); ++c)
		{
			Ogre::VertexBoneAssignment vba;
			vba.boneIndex = globalBoneIndexes[regionArray(c)];
			vba.weight = weightArray(c);

			// Apply this to each vertex derived from this point
			// Locate ProtoSubMeshes which use this mesh
			for (MaterialProtoSubMeshMap::iterator mi = mMaterialProtoSubmeshMap.begin();
				mi != mMaterialProtoSubmeshMap.end(); ++mi)
			{
				for (ProtoSubMeshList::iterator psi = mi->second->begin();
					psi != mi->second->end(); ++psi)
				{
					ProtoSubMesh* ps = *psi;
					ProtoSubMesh::GeometryOffsetMap::iterator poli = 
						ps->geometryOffsetMap.find(origGuDetail);
					if (poli != ps->geometryOffsetMap.end())
					{
						// adjust index based on merging
						size_t adjIndex = pointIndex + poli->second;
						// look up real index
						// If it doesn't exist, it's probably on a seam
						// between groups and we can safely skip it
						IndexRemap::iterator remi = ps->posIndexRemap.find(adjIndex);
						if (remi != ps->posIndexRemap.end())
						{
							size_t vertIndex = remi->second;
							bool moreVerts = true;
							// add UniqueVertex and clones
							while (moreVerts)
							{
								UniqueVertex& vertex = ps->uniqueVertices[vertIndex];
								vba.vertexIndex = vertIndex;
								ps->boneAssignments.insert(
									Ogre::Mesh::VertexBoneAssignmentList::value_type(vertIndex, vba));

#if _DEBUG
								Ogre::StringUtil::StrStreamType vbaMsg;
								vbaMsg << "Added bone assignment: point=" << pointIndex
									<< " vertex=" << vertIndex << " boneIndex=" << vba.boneIndex
									<< " bone=" << mBoneList[vba.boneIndex]->getName()
									<< " weight=" << vba.weight;
								lmgr.logMessage(vbaMsg.str());
#endif

								if (vertex.nextIndex == 0)
								{
									moreVerts = false;
								}
								else
								{
									vertIndex = vertex.nextIndex;
								}
							}
						}

					}
				}

			}

		}

	}
	
	delete [] globalBoneIndexes;

}
//---------------------------------------------------------------------
void HoudiniOgre_Mesh::processAnimationCycles(const OP_Node* objNode, 
											  int numFrames, int frameStart, 
											  float fps)
{
	Ogre::LogManager& lmgr = Ogre::LogManager::getSingleton();
	// Animations are stored on the objects themselves as a detail attribute
	// called 'animcycle'. We sample this throughout the timeline and its value
	// will change when a different attribute is found.
	// If no animcycle attribute is found, we just bake the entire timeline
	// as a single animation called 'default'

	AnimationEntry animEntry;
	bool inAnimation = false;
	
	Ogre::StringUtil::StrStreamType msg;
	for (int f = frameStart; f < frameStart + numFrames; ++f)
	{
		// Convert to frame time
		float fTime = (float)f / fps;

		// Since detail attributes change per frame, have to 'cook' the data at
		// each frame to extract it?

		SOP_Node* displaySop = objNode->castToOBJNode()->getDisplaySopPtr();

		OP_Context tempCtx(fTime);
		GU_DetailHandleAutoReadLock gdl(displaySop->getCookedGeoHandle(tempCtx));
		const GU_Detail *frameGeom = gdl.getGdp();


		GEO_AttributeHandle a = frameGeom->getDetailAttribute("animcycle");
		UT_String val;
		if (a.getString(val))
		{
			Ogre::String animName(val);
			if (inAnimation)
			{
				if (animName == animEntry.animationName)
				{
					// just extend
					animEntry.endFrame = f;
				}
				else
				{
					// changed, write out old animation
					animEntry.endFrame = f - 1;
					mAnimList.push_back(animEntry);
					inAnimation = false;

					msg.str(Ogre::StringUtil::BLANK);
					msg << "Animation detected: '" << animEntry.animationName 
						<< "' startFrame=" << animEntry.startFrame
						<< " endFrame=" << animEntry.endFrame;
					lmgr.logMessage(msg.str());
				}
			}

			// NB not else since inAnimation can be reset above
			if (!inAnimation)
			{
				// ignore animations called 'none' or blank
				if (!animName.empty() && animName != "none")
				{
					// start new animation
					animEntry.animationName = animName;
					animEntry.startFrame = f;
					inAnimation = true;
				}
			}

		}
	}
	// mop-up if we had animation right up to last frame
	if (inAnimation)
	{
		animEntry.endFrame = frameStart + numFrames - 1;
		mAnimList.push_back(animEntry);

		msg.str(Ogre::StringUtil::BLANK);
		msg << "Animation detected: '" << animEntry.animationName 
			<< "' startFrame=" << animEntry.startFrame
			<< " endFrame=" << animEntry.endFrame;
		lmgr.logMessage(msg.str());
	}

	// Did we find any animations?
	if (mAnimList.empty())
	{
		// if not, create a default one covering the whole period
		animEntry.animationName = "default";
		animEntry.startFrame = frameStart;
		animEntry.endFrame = frameStart + numFrames - 1;
		mAnimList.push_back(animEntry);
		lmgr.logMessage("No 'animcycle' attributes found, setting up a 'default' animation.");
	}


}
//-----------------------------------------------------------------------
HoudiniOgre_Mesh::ProtoSubMesh* HoudiniOgre_Mesh::createOrRetrieveProtoSubMesh(
	const Ogre::String& materialName, const Ogre::String& name, 
	bool hasNormals, TextureCoordDimensionList& texCoordDims, bool hasVertexColours)
{
	bool createNew = true;
	ProtoSubMesh* ret = 0;
	ProtoSubMeshList* protoList = 0;

	MaterialProtoSubMeshMap::iterator pi = mMaterialProtoSubmeshMap.find(materialName);
	if (pi == mMaterialProtoSubmeshMap.end())
	{
		protoList = new ProtoSubMeshList();
		mMaterialProtoSubmeshMap[materialName] = protoList;
	}
	else
	{
		// Iterate over the protos with the same material
		protoList = pi->second;

		for (ProtoSubMeshList::iterator psi = protoList->begin(); psi != protoList->end(); ++psi)
		{
			ProtoSubMesh* candidate = *psi;
			// Check format is compatible
			if (candidate->textureCoordDimensions.size() != texCoordDims.size())
			{
				continue;
			}
			if (candidate->hasVertexColours != hasVertexColours)
			{
				continue;
			}
			if (candidate->hasNormals != hasNormals)
			{
				continue;
			}

			bool compat = true;
			TextureCoordDimensionList::iterator t = texCoordDims.begin();
			TextureCoordDimensionList::iterator u = candidate->textureCoordDimensions.begin(); 
			for (;t != texCoordDims.end(); ++t,++u)
			{
				if (*t != *u)
				{
					compat = false;
					break;
				}
			}

			if (compat)
			{
				createNew = false;
				ret = candidate;
				break;
			}
		}
	}

	if (createNew)
	{
		ret = new ProtoSubMesh();
		protoList->push_back(ret);
		ret->materialName = materialName;
		ret->name = name;
		ret->textureCoordDimensions = texCoordDims;
		ret->hasVertexColours = hasVertexColours;
		ret->hasNormals = hasNormals;
	}

	return ret;

}
//-----------------------------------------------------------------------
size_t HoudiniOgre_Mesh::createOrRetrieveUniqueVertex(
	ProtoSubMesh* proto, size_t positionIndex, 
	bool positionIndexIsOriginal, const UniqueVertex& vertex)
{
	size_t lookupIndex;
	if (positionIndexIsOriginal)
	{
		// look up the original index
		IndexRemap::iterator remapi = 
			proto->posIndexRemap.find(positionIndex);
		if (remapi == proto->posIndexRemap.end())
		{
			// not found, add
			size_t realIndex = proto->uniqueVertices.size();
			// add remap entry so we can find this again
			proto->posIndexRemap[positionIndex] = realIndex;
			proto->uniqueVertices.push_back(vertex);
			return realIndex;
		}
		else
		{
			// Found existing mapping
			lookupIndex = remapi->second;
		}
	}
	else
	{
		// Not an original index, index is real
		lookupIndex = positionIndex;
	}

	// If we get here, either the position isn't an original index (ie
	// we've already found that it doesn't match, and have cascaded)
	// or there is an existing entry
	// Get existing
	UniqueVertex& orig = proto->uniqueVertices[lookupIndex];
	// Compare, do we have the same details?
	if (orig == vertex)
	{
		// ok, they match
		return lookupIndex;
	}
	else
	{
		// no match, go to next or create new
		if (orig.nextIndex)
		{
			// cascade to the next candidate (which is a real index, not an original)
			return createOrRetrieveUniqueVertex(
				proto, orig.nextIndex, false, vertex);
		}
		else
		{
			// No more cascades to check, must be a new one
			// get new index
			size_t realIndex = proto->uniqueVertices.size();
			orig.nextIndex = realIndex;
			// create new (NB invalidates 'orig' reference)
			proto->uniqueVertices.push_back(vertex);
			// note, don't add to remap, that's only for finding the
			// first entry, nextIndex is used to chain to the others

			return realIndex;
		}
	}
}
//---------------------------------------------------------------------
void HoudiniOgre_Mesh::Export(const Ogre::String& filename, bool edgeList, 
							  bool tangents, Ogre::VertexElementSemantic tangentsType, 
							  int numFrames, int frameStart, 
							  float fps, float ikSampleRate)
{
	Ogre::LogManager& lmgr = Ogre::LogManager::getSingleton();

	if (!mpMesh.isNull())
	{
		// Set up skeleton and link
		if (!mBoneList.empty())
		{
			// strip off 'mesh', replace with 'skeleton'
			Ogre::String skeletonFileName = filename.substr(0, filename.size() - 4) + "skeleton";

			HoudiniOgre_Skeleton skel(mBoneList);
			skel.Export(skeletonFileName, fps, ikSampleRate, mAnimList);

			// Now trim the skeleton file name down to just filename
			Ogre::String skeletonName;
			Ogre::String::size_type startPos = skeletonFileName.rfind('/');
			if (startPos == Ogre::String::npos)
			{
				skeletonName = skeletonFileName;
			}
			else
			{
				skeletonName = skeletonFileName.substr(startPos + 1);
			}

			mpMesh->setSkeletonName(skeletonName);

		}

		// Bake any protos that haven't been done yet
		bakeProtoSubMeshes();

		if (edgeList)
		{
			mpMesh->buildEdgeList();
		}

		if (tangents)
		{
			unsigned short inTex, outTex;
			if (mpMesh->suggestTangentVectorBuildParams(tangentsType, inTex, outTex))
			{
				mpMesh->buildTangentVectors(tangentsType, inTex, outTex);
			}
			else
			{
				std::cerr << "Warning: suggestTangentVectorBuildParams failed, aborting tangent generation." << std::endl;
			}
		}


		Ogre::MeshSerializer serializer;
		serializer.exportMesh(mpMesh.getPointer(), filename);

		Ogre::MeshManager::getSingleton().remove(mpMesh->getHandle());

		mpMesh.setNull();

		mBoneList.clear();
		mAnimList.clear();
	}

}
//---------------------------------------------------------------------
//-----------------------------------------------------------------------
void HoudiniOgre_Mesh::bakeProtoSubMeshes()
{
	// Take the list of ProtoSubMesh instances and bake a SubMesh per
	// instance, then clear the list

	for (MaterialProtoSubMeshMap::iterator mi = mMaterialProtoSubmeshMap.begin();
		mi != mMaterialProtoSubmeshMap.end(); ++mi)
	{
		for (ProtoSubMeshList::iterator psi = mi->second->begin();
			psi != mi->second->end(); ++psi)
		{
			// export each one
			bakeProtoSubMesh(*psi);

			// free it
			delete *psi;
		}
		// delete proto list
		delete mi->second;
	}
	mMaterialProtoSubmeshMap.clear();

}
//-----------------------------------------------------------------------
void HoudiniOgre_Mesh::bakeProtoSubMesh(ProtoSubMesh* proto)
{
	// Skip protos which have ended up empty
	if (proto->indices.empty())
		return;

	Ogre::SubMesh* sm = 0;
	if (proto->name.empty())
	{
		// anonymous submesh
		sm = mpMesh->createSubMesh();
	}
	else
	{
		// named submesh
		sm = mpMesh->createSubMesh(proto->name);
	}

	// Set material
	sm->setMaterialName(proto->materialName);
	// never use shared geometry
	sm->useSharedVertices = false;
	sm->vertexData = new Ogre::VertexData();
	// always do triangle list
	sm->indexData->indexCount = proto->indices.size();

	sm->vertexData->vertexCount = proto->uniqueVertices.size();
	// Determine index size
	bool use32BitIndexes = false;
	if (proto->uniqueVertices.size() > 65536)
	{
		use32BitIndexes = true;
	}

	sm->indexData->indexBuffer = 
		Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(
		use32BitIndexes ? Ogre::HardwareIndexBuffer::IT_32BIT : Ogre::HardwareIndexBuffer::IT_16BIT,
		sm->indexData->indexCount,
		Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
	if (use32BitIndexes)
	{
		uint32* pIdx = static_cast<uint32*>(
			sm->indexData->indexBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD));
		writeIndexes(pIdx, proto->indices);
		sm->indexData->indexBuffer->unlock();
	}
	else
	{
		uint16* pIdx = static_cast<uint16*>(
			sm->indexData->indexBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD));
		writeIndexes(pIdx, proto->indices);
		sm->indexData->indexBuffer->unlock();
	}


	// define vertex declaration
	unsigned buf = 0;
	size_t offset = 0;
	// always add position and normal
	sm->vertexData->vertexDeclaration->addElement(buf, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
	offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
	// Split vertex data after position if poses present
	if (!proto->poseList.empty())
	{
		buf++;
		offset = 0;
	}
	// Optional normal
	if(proto->hasNormals)
	{
		sm->vertexData->vertexDeclaration->addElement(buf, offset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
		offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
	}
	// split vertex data here if animated
	if (mpMesh->hasSkeleton())
	{
		buf++;
		offset = 0;
	}
	// Optional vertex colour
	if(proto->hasVertexColours)
	{
		sm->vertexData->vertexDeclaration->addElement(buf, offset, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
		offset += Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR);
	}
	// Define UVs
	for (unsigned short uvi = 0; uvi < proto->textureCoordDimensions.size(); ++uvi)
	{
		Ogre::VertexElementType uvType = 
			Ogre::VertexElement::multiplyTypeCount(
			Ogre::VET_FLOAT1, proto->textureCoordDimensions[uvi]);
		sm->vertexData->vertexDeclaration->addElement(
			buf, offset, uvType, Ogre::VES_TEXTURE_COORDINATES, uvi);
		offset += Ogre::VertexElement::getTypeSize(uvType);
	}

	// create & fill buffer(s)
	for (unsigned short b = 0; b <= sm->vertexData->vertexDeclaration->getMaxSource(); ++b)
	{
		createVertexBuffer(sm->vertexData, b, proto->uniqueVertices);
	}

	// deal with any bone assignments
	if (!proto->boneAssignments.empty())
	{
		// rationalise first (normalises and strips out any excessive bones)
		sm->parent->_rationaliseBoneAssignments(
			sm->vertexData->vertexCount, proto->boneAssignments);

		for (Ogre::Mesh::VertexBoneAssignmentList::iterator bi = proto->boneAssignments.begin();
			bi != proto->boneAssignments.end(); ++bi)
		{
			sm->addBoneAssignment(bi->second);
		}
	}
}
//---------------------------------------------------------------------
//-----------------------------------------------------------------------
template <typename T> 
void HoudiniOgre_Mesh::writeIndexes(T* buf, IndexList& indexes)
{
	IndexList::const_iterator i, iend;
	iend = indexes.end();
	for (i = indexes.begin(); i != iend; ++i)
	{
		*buf++ = static_cast<T>(*i);
	}
}
//-----------------------------------------------------------------------
void HoudiniOgre_Mesh::createVertexBuffer(Ogre::VertexData* vd, 
	unsigned short bufIdx, UniqueVertexList& uniqueVertexList)
{
	Ogre::HardwareVertexBufferSharedPtr vbuf = 
		Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
		vd->vertexDeclaration->getVertexSize(bufIdx),
		vd->vertexCount, 
		Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
	vd->vertexBufferBinding->setBinding(bufIdx, vbuf);
	size_t vertexSize = vd->vertexDeclaration->getVertexSize(bufIdx);

	char* pBase = static_cast<char*>(
		vbuf->lock(Ogre::HardwareBuffer::HBL_DISCARD));

	Ogre::VertexDeclaration::VertexElementList elems = 
		vd->vertexDeclaration->findElementsBySource(bufIdx);
	Ogre::VertexDeclaration::VertexElementList::iterator ei, eiend;
	eiend = elems.end();
	float* pFloat;
	Ogre::RGBA* pRGBA;

	UniqueVertexList::iterator srci = uniqueVertexList.begin();

	for (size_t v = 0; v < vd->vertexCount; ++v, ++srci)
	{
		for (ei = elems.begin(); ei != eiend; ++ei)
		{
			Ogre::VertexElement& elem = *ei;
			switch(elem.getSemantic())
			{
			case Ogre::VES_POSITION:
				elem.baseVertexPointerToElement(pBase, &pFloat);
				*pFloat++ = srci->position.x;
				*pFloat++ = srci->position.y;
				*pFloat++ = srci->position.z;
				break;
			case Ogre::VES_NORMAL:
				elem.baseVertexPointerToElement(pBase, &pFloat);
				*pFloat++ = srci->normal.x;
				*pFloat++ = srci->normal.y;
				*pFloat++ = srci->normal.z;
				break;
			case Ogre::VES_DIFFUSE:
				elem.baseVertexPointerToElement(pBase, &pRGBA);
				*pRGBA = srci->colour;
				break;
			case Ogre::VES_TEXTURE_COORDINATES:
				elem.baseVertexPointerToElement(pBase, &pFloat);
				for (int t = 0; t < Ogre::VertexElement::getTypeCount(elem.getType()); ++t)
				{
					float val = srci->uv[elem.getIndex()][t];
					*pFloat++ = val;
				}
				break;
			}
		}
		pBase += vertexSize;
	}
	vbuf->unlock();

}
//---------------------------------------------------------------------

