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

#ifndef __HoudiniOgre_Mesh__
#define __HoudiniOgre_Mesh__

#include "HoudiniOgre_Prerequisites.h"
#include "OgreCommon.h"
#include "OgreVector3.h"
#include "OgreMesh.h"

// forward decls
class UT_String;
class GU_Detail;
class OP_Node;
class OBJ_Bone;
class SOP_Node;

#include "OgreNoMemoryMacros.h"
#include <GB/GB_AttributeHandle.h>
#include "OgreMemoryMacros.h"


/** Deals with exporting Houdini geometry objects into a Mesh.
@remarks
	A single OGRE mesh may be made up of several Houdini geometry objects. Merging
	geometry into a single mesh is important for efficiency, and where possible
	we want to make sure that the smallest number of render ops are used. Therefore
	if two Houdini geometry objects use the same shader and we're exporting a 
	unified mesh, we actually want to combine the geometry from those two objects
	into one set of vertex / index data and one render operation.
@par
	Everything using one instance of this object will combine geometry into one
	Ogre::Mesh - including separate SubMesh instances for differing materials.
	If you actually want to export more than one mesh, then instantiate this class
	more than once corresponding to where you want the divisions to be.
*/
class HoudiniOgre_Mesh
{
public:
	/** Standard constructor.
	*/
	HoudiniOgre_Mesh();
	virtual ~HoudiniOgre_Mesh();

	/** Add a set of geometry to this mesh.
	*/
	void addGeometry(const OP_Node* objNode, bool snapshotting, float frameTime,
		bool useObjectTransforms, int numFrames, int frameStart, 
		float fps, float ikSampleRate);

	/** Export the built mesh contents to a file. */
	void Export(const Ogre::String& filename, bool edgeList, bool tangents, 
		Ogre::VertexElementSemantic tangentsType, int numFrames, int frameStart, 
		float fps, float ikSampleRate);

protected:

	Ogre::MeshPtr mpMesh;

	/** This struct represents a unique vertex, identified from a unique 
	combination of components.
	*/
	class UniqueVertex
	{
	public:
		Ogre::Vector3 position;
		Ogre::Vector3 normal;
		Ogre::Vector3 uv[OGRE_MAX_TEXTURE_COORD_SETS];
		Ogre::RGBA colour;
		// The index of the next component with the same base details
		// but with some variation
		size_t nextIndex;

		UniqueVertex();
		bool operator==(const UniqueVertex& rhs) const;

	};
	typedef std::vector<UniqueVertex> UniqueVertexList;
	// dynamic index list; 32-bit until we know the max vertex index
	typedef std::vector<Ogre::uint32> IndexList;

	typedef std::map<size_t, size_t> IndexRemap;
	/** Working area which will become a submesh once we've finished figuring
	out what goes in there.
	*/
	struct ProtoSubMesh
	{
		// Name of the submesh (may be blank if we're merging)
		Ogre::String name;
		// Material name
		Ogre::String materialName;
		// unique vertex list
		UniqueVertexList uniqueVertices;
		// Defines number of texture coord sets and their dimensions
		std::vector<unsigned short> textureCoordDimensions;
		// Vertex colours?
		bool hasVertexColours;
		// Normals?
		bool hasNormals;
		// Last geometry object added to this proto (re-use base index)
		GU_Detail* lastMeshEntry;
		// Index offset for last geometry object entry
		size_t lastMeshIndexOffset;
		// index list
		IndexList indices;
		// map of polygon mesh -> position index offset (only > 0 when submeshes merged)
		typedef std::map<GU_Detail*, size_t> GeometryOffsetMap;
		GeometryOffsetMap geometryOffsetMap;
		// map original position index (+any PM offset) -> first real instance in this one
		IndexRemap posIndexRemap;
		Ogre::Mesh::VertexBoneAssignmentList boneAssignments;
		/// By-value pose list, build up ready for transfer later
		std::list<Ogre::Pose> poseList;

		ProtoSubMesh() : lastMeshEntry(0), lastMeshIndexOffset(0) {}


	};

	/// List of ProtoSubMeshes that use the same material but are not geometrically compatible
	typedef std::list<ProtoSubMesh*> ProtoSubMeshList;

	/// List of proto submeshes by material
	typedef std::map<Ogre::String, ProtoSubMeshList*> MaterialProtoSubMeshMap;
	/// List of proto submeshes by material
	MaterialProtoSubMeshMap mMaterialProtoSubmeshMap;
	/// List of deviant proto submeshes by primitive index 
	typedef std::map<int, ProtoSubMesh*> PrimitiveToProtoSubMeshList;
	/// List of deviant proto submeshes by primitive index
	PrimitiveToProtoSubMeshList mPrimitiveToProtoSubMeshList;
	/// Primary ProtoSubMesh (the one used by the geometry object by default)
	ProtoSubMesh* mMainProtoMesh;
	// Current PolygonMesh texture coord information
	typedef std::vector<unsigned short> TextureCoordDimensionList;
	TextureCoordDimensionList mCurrentTextureCoordDimensions;
	// Current geometry has Vertex colours?
	bool mCurrentHasVertexColours;
	// Current geometry has normals?
	bool mCurrentHasNormals;
	// Attribute linking
	/// Normal attribute index
	int mNormalAttrib;
	/// Are normals on the points or per-vertex (= a point for a specific face)
	bool mNormalsOnVertices;
	typedef std::vector<int> AttributeList;
	AttributeList mUVAttribs;
	typedef std::vector<bool> BoolList;
	/// Is each uv on the points or per-vertex (= a point for a specific face)
	BoolList mUVOnVertices;
	int mDiffseAttrib;
	/// Is diffuse on the points or per-vertex (= a point for a specific face)
	bool mDiffuseOnVertices;



	/** Try to look up an existing vertex with the same information, or
        create a new one.
    @remarks
		Note that we buid up the list of unique position indexes that are
		actually used by each ProtoSubMesh as we go. When new positions
		are found, they are added and a remap entry created to take account
		of the fact that there may be extra vertices created in between, or
		there may be gaps due to clusters meaning not every position is
		used by every ProtoSubMesh. When an existing entry is found, we
		compare the vertex data, and if it differs, create a new vertex and
		'chain' it to the previous instances of this position index through
		nextIndex. This means that every position vertex has a single 
		remapped starting point in the per-ProtoSubMesh vertex list, and a 
		unidirectional linked list of variants of that vertex where other
		components differ.
    @par
        Note that this re-uses as many vertices as possible, and also places
        every unique vertex in it's final index in one pass, so the return 
		value from this method can be used as an adjusted vertex index.
    @returns The index of the unique vertex
    */	
	size_t createOrRetrieveUniqueVertex(ProtoSubMesh* proto, 
		size_t positionIndex, bool positionIndexIsOriginal,
		const UniqueVertex& vertex);

	/// Perform initial preprocessing on geometry object (returns false if aborted)
	bool preprocessGeometry(GU_Detail* guDetail, const OP_Node* objNode, float frameTime);
	/// Perform final postprocessing on geometry object
	void postprocessGeometry(GU_Detail* guDetail, const OP_Node* objNode);
	/// Build a list of bone assignments and construct a list of bones of interest
	void processBoneAssignments(const OP_Node* objNode, GU_Detail* origGuDetail);
	/// Look for animation cycle attributes to build animation list
	void processAnimationCycles(const OP_Node* objNode,
		int numFrames, int frameStart, float fps);

	/// Retrieve a ProtoSubMesh for the given material name 
	/// (creates if required, validates if re-using)
	ProtoSubMesh* createOrRetrieveProtoSubMesh(const Ogre::String& materialName,
		const Ogre::String& name, 
		bool hasNormals,
		TextureCoordDimensionList& texCoordDims,
		bool hasVertexColours);

	/// Bake the current list of proto submeshes, and clear list
	void bakeProtoSubMeshes();
	/// Bake a single ProtoSubMesh 
	void bakeProtoSubMesh(ProtoSubMesh* proto);
	/** Create and fill a vertex buffer */
	void createVertexBuffer(Ogre::VertexData* vd, unsigned short bufIdx, 
		UniqueVertexList& uniqueVertexList);
	/** Templatised method for writing indexes */
	template <typename T> void writeIndexes(T* buf, IndexList& indexes);

	/// Map of bones that are found to be of interest
	BoneList mBoneList;

	/// Animation list that has been built up for the objects being exported
	AnimationList mAnimList;



};


#endif
