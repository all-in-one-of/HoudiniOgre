/*
-----------------------------------------------------------------------------
HoudiniOgre_ROP.cpp

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

#include "HoudiniOgre_ROP.h"

#include "OgreNoMemoryMacros.h"
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_Director.h>
#include <OBJ/OBJ_Node.h>
#include <OBJ/OBJ_Geometry.h>
#include <GU/GU_Detail.h>
#include <PRM/PRM_Parm.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_DialogScript.h>
#include <ROP/ROP_Shared.h>
#include <ROP/ROP_Error.h>
#include <MOT/MOT_Director.h>
#include <FS/FS_Info.h>
#include "OgreMemoryMacros.h"

#include "HoudiniOgre_Mesh.h"
#include "OgreStringConverter.h"



//---------------------------------------------------------------------
HoudiniOgre_ROP::HoudiniOgre_ROP(OP_Network *net, const char *name, OP_Operator *entry)
: ROP_Node(net, name, entry), mLogMgr(0), mResMgr(0), mMeshMgr(0), mSkelMgr(0), mMatMgr(0), mBufMgr(0)
{

}
//---------------------------------------------------------------------
HoudiniOgre_ROP::~HoudiniOgre_ROP()
{

}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
OP_Node* HoudiniOgre_ROP::myConstructor(OP_Network *net, const char*name, 
										OP_Operator *op)
{
	// This method is called every time a new ROP is created in Houdini
	return new HoudiniOgre_ROP(net, name, op);
}
//---------------------------------------------------------------------
unsigned HoudiniOgre_ROP::disableParms()
{
	int disableFlags = 0;

	/*
	disableFlags += enableParm("take", 0);
	disableFlags += enableParm("renderer", 0);
	disableFlags += enableParm("activelights", 0);
	disableFlags += enableParm("visiblefog", 0);
	disableFlags += enableParm("dispsop", 0);
	disableFlags += enableParm("initsim", 0);
	disableFlags += enableParm("binary", 0);
	disableFlags += enableParm("picture", 0);
	disableFlags += enableParm("background", 0);
	disableFlags += enableParm("dof", 0);
	disableFlags += enableParm("jitter", 0);
	disableFlags += enableParm("dither", 0);
	disableFlags += enableParm("gamma", 0);
	disableFlags += enableParm("sample", 0);
	disableFlags += enableParm("field", 0);
	disableFlags += enableParm("jitter", 0);
	disableFlags += enableParm("mblur", 0);
	disableFlags += enableParm("blur", 0);
	disableFlags += enableParm("tres", 0);
	disableFlags += enableParm("res", 0);
	disableFlags += enableParm("aspect", 0);
	disableFlags += enableParm("tprerender", 0);
	disableFlags += enableParm("prerender", 0);
	disableFlags += enableParm("preframe", 0);
	disableFlags += enableParm("tpost", 0);
	*/


	// Allow the base class to disable parameters as well.
	return disableFlags + ROP_Node::disableParms();


}
//---------------------------------------------------------------------
// ROP parameters
static PRM_Name outputParamName("output", "Output Path");
static PRM_Name exportModeParamName("exportMode", "Export Mode");
static PRM_Name objectTransformsParamName("transformMode", "Bake Object Transforms");
static PRM_Name generateTangentsName("genTangents", "Generate Tangents");
static PRM_Name tangentsTypeName("tangentsType", "Tangent VertexElement");
static PRM_Name generateEdgeListsName("genEdgeLists", "Generate Edge Lists");
static PRM_Name ikSampleRateName("iksamplerate", "IK Sample Rate");

//static PRM_Default outputDefault(0.0, "$HIP/");
static PRM_Default tangentsTypeDefault(0.0, "tangent");
static PRM_Default exportModeDefault(0.0, "each");
//static PRM_Default ikSampleRateDefault(5.0, "");
static PRM_Default ikSampleRateDefault(0.0, "5");
static PRM_Default selectedDefault(1.0);

static PRM_Name tangentsChoices[] = { 
	PRM_Name("tangent", "Tangent Binding"),
	PRM_Name("texcoord", "Texcoord Binding"),
	PRM_Name() // terminator
};
static PRM_Name exportModeChoices[] = { 
	PRM_Name("each", "One Mesh Per Object"),
	PRM_Name("all", "Single Merged Mesh"),
	PRM_Name() // terminator
};
static PRM_ChoiceList tangentsTypeChoice(PRM_CHOICELIST_SINGLE, tangentsChoices);
static PRM_ChoiceList exportModeChoice(PRM_CHOICELIST_SINGLE, exportModeChoices);

static PRM_Range ikSampleRateRange(PRM_RANGE_UI, 1.0f, PRM_RANGE_UI, 100.0f);

static PRM_Template customTemplate[] = { 
	// ... additional parameters here ... 
	PRM_Template(PRM_FILE, 1, &outputParamName),
	PRM_Template(PRM_STRING, 1, &exportModeParamName, &exportModeDefault, &exportModeChoice),
	PRM_Template(PRM_TOGGLE, 1, &objectTransformsParamName, &selectedDefault),
	PRM_Template(PRM_TOGGLE, 1, &generateTangentsName),
	PRM_Template(PRM_STRING, 1, &tangentsTypeName, &tangentsTypeDefault, &tangentsTypeChoice),
	PRM_Template(PRM_TOGGLE, 1, &generateEdgeListsName, &selectedDefault),
	//PRM_Template(PRM_INT, 1, &ikSampleRateName, &ikSampleRateDefault, 0, &ikSampleRateRange),
	PRM_Template(PRM_STRING, 1, &ikSampleRateName, &ikSampleRateDefault),

	PRM_Template() }; 
	//---------------------------------------------------------------------
	OP_TemplatePair* HoudiniOgre_ROP::getCustomTemplatePair()
	{
		OP_TemplatePair* custom = new OP_TemplatePair(customTemplate); 
		return  new OP_TemplatePair( ROP_Node::getROPbaseTemplate(), custom);

	}
	//---------------------------------------------------------------------
#ifdef _DEBUG
#   define INTERNAL_NAME "OgreExportDebug"
#   define UI_NAME "Ogre3D Debug Exporter"
#else
#   define INTERNAL_NAME "OgreExport"
#   define UI_NAME "Ogre3D Exporter"
#endif
	void HoudiniOgre_ROP::installOperator(OP_OperatorTable *table)
	{
		// This method is called to construct a new type of operator (sort of like
		// an operator factory it appears)
		OP_VariablePair	*base_var = new OP_VariablePair(ROP_Node::myVariableList);
		//OP_VariablePair	*myvar = new OP_VariablePair(ROP_IFD::myVariableList, base_var);

		OP_Operator	*op = new OP_Operator(
			INTERNAL_NAME,							// internal name
			UI_NAME,								// UI name
			HoudiniOgre_ROP::myConstructor,			// ROP constructor
			getCustomTemplatePair(),				// Parameter list
			0,										// Min # inputs
			0,										// Max # inputs
			base_var,								// Local variables (??)
			OP_FLAG_GENERATOR);						// Generator / source (??)
		table->addOperator(op);

	}
	//---------------------------------------------------------------------
	int HoudiniOgre_ROP::startRender(int nFrames, float tStart, float tEnd)
	{
		// Construct Ogre singletons 
		createSingletons();
		mLogMgr->createLog("HoudiniOgre.log", true, false);

		// Get all the params and validate
		if (!extractParams())
			return 0;

		// Derive FPS
		mFps = (float)(nFrames-1) / (tEnd - tStart);

		// convert start time to a frame (easier to iterate over accurately)
		// note this is zero-based not 1-based like in UI
		int frameStart = static_cast<int>(tStart / mFps);
		if (!mSnapshotPerFrame)
		{
			return exportGeometries(0, false, nFrames, frameStart);
		}

		return 1;
			
	}
	//---------------------------------------------------------------------
	int HoudiniOgre_ROP::extractParams()
	{
		// Get param values and do basic pre-validation
		PRM_Parm& outputParm = this->getParm(outputParamName.getToken());
		outputParm.getValue(0, mOutputPath, 0, 0);

		if (mOutputPath == "")
		{
			addError(ROP_RENDER_ERROR, "You must supply an output path!");
			return 0;
		}

		// Look for per-frame parameter in name
		// If it exists, we'll export meshes once per frame rather than just once
		if (mOutputPath.contains("$F"))
		{
			mSnapshotPerFrame = true;
		}
		else
		{
			mSnapshotPerFrame = false;
		}

		PRM_Parm& exportModeParm = this->getParm(exportModeParamName.getToken());
		UT_String exportMode;
		exportModeParm.getValue(0, exportMode, 0, 0);
		if (exportMode == "each")
		{
			mExportMeshPerObject = true;
		}
		else
		{
			mExportMeshPerObject = false;
		}

		PRM_Parm& objectTransformsParm = this->getParm(objectTransformsParamName.getToken());
		int objectTransforms;
		objectTransformsParm.getValue(0, objectTransforms, 0);
		mObjectTransforms = objectTransforms != 0;

		PRM_Parm& genTangentsParm = this->getParm(generateTangentsName.getToken());
		int genTangents;
		genTangentsParm.getValue(0, genTangents, 0);
		mGenerateTangents = genTangents != 0;

		PRM_Parm& tangentsTypeParm = this->getParm(tangentsTypeName.getToken());
		UT_String tangentsTypeStr;
		tangentsTypeParm.getValue(0, tangentsTypeStr, 0, 0);
		if (tangentsTypeStr == "tangent")
		{
			mTangentsSemantic = Ogre::VES_TANGENT;
		}
		else
		{
			mTangentsSemantic = Ogre::VES_TEXTURE_COORDINATES;
		}

		PRM_Parm& genEdgeListsParm = this->getParm(generateEdgeListsName.getToken());
		int genEdgeLists;
		genEdgeListsParm.getValue(0, genEdgeLists, 0);
		mGenerateEdgeLists = genEdgeLists != 0;

		PRM_Parm& ikSampleRateParm = this->getParm(ikSampleRateName.getToken());
		// Should be an int, but doesn't work atm
		// using a string fallback
		UT_String ikSample;
		ikSampleRateParm.getValue(0, ikSample , 0, 0);
		mIkSampleRate = Ogre::StringConverter::parseReal(Ogre::String(ikSample));
		if (mIkSampleRate == 0.0f)
		{
			mIkSampleRate = 5.0f;
		}

		return 1;

	}
	//---------------------------------------------------------------------
	int HoudiniOgre_ROP::exportGeometries(float t, bool snapshotting, 
		int numFrames, int frameStart)
	{

		// expand any Houdini symbols in output
		UT_String expandedOutput;
		OPgetDirector()->getChannelManager()->expandString(mOutputPath, expandedOutput, t);
		// note that we don't require that output path is a directory, it may
		// also include a file prefix, or it may be a final file. We will append 
		// the object name if we're exporting individual files

		// for outputpath containing spaces (such as Windows desktop), log to check that Houdini isn't adding surrounding quotes (DEE)
		Ogre::LogManager::getSingleton().logMessage("Expanded Output (Houdini): " + Ogre::String(expandedOutput));


		try
		{

			HoudiniOgre_Mesh mesh;

			// We want all object instances
			OP_Node* objectsNode = OPgetDirector()->getChild("obj");
			int numChildren = objectsNode->getNchildren();
			for (int i = 0; i < numChildren; ++i)
			{
				OP_Node* childObj = objectsNode->getChild(i);
				OP_Operator* op = childObj->getOperator();

				Ogre::LogManager::getSingleton().logMessage("Examining: " + Ogre::String(childObj->getName()));
				OBJ_Node* objNode = childObj->castToOBJNode();
				// only process 'geo' types
				if (objNode && op->getName() == "geo")
				{
					OBJ_Geometry* geo = objNode->castToOBJGeometry();
					// Only export displayed geom objects
					if (geo && geo->getDisplay() == 1)
					{
						Ogre::LogManager::getSingleton().logMessage("Parsing geo: " + Ogre::String(childObj->getName()));
						mesh.addGeometry(childObj, snapshotting, t, mObjectTransforms, 
							numFrames, frameStart, mFps, mIkSampleRate);

						// If we're exporting each object...
						if (mExportMeshPerObject)
						{
							Ogre::String filename(expandedOutput);  // verified no surrounding quotes (DEE)
							FS_Info fsinfo(expandedOutput);

							if (fsinfo.getIsDirectory())
							{
								if (!Ogre::StringUtil::endsWith(filename, "/"))
								{
									filename += "/";
								}
							}
							filename += childObj->getName();
							filename += ".mesh";

							mesh.Export(filename, mGenerateEdgeLists, 
								mGenerateTangents, mTangentsSemantic, 
								numFrames, frameStart, mFps, mIkSampleRate);
						}
					}

				}
			}

			// Exporting for the entire scene?
			if (!mExportMeshPerObject)
			{
				// Expected that this is a destination file
				Ogre::String filename(expandedOutput);
				FS_Info fsinfo(expandedOutput);
				
				if (fsinfo.getIsDirectory())
				{
					if (!Ogre::StringUtil::endsWith(filename, "/"))
					{
						filename += "/";
					}
					// add the basename of the .hip
					MOT_Director *mot = dynamic_cast<MOT_Director *>(OPgetDirector());
					// UT_PathFile / UT_PathFileInfo are undocumented and don't seem to give me the
					// base name without the path easily, just do it with strings
					Ogre::String hipFile(mot->getFileName());
					Ogre::String::size_type spos = hipFile.find_last_of('/');
					if (spos != Ogre::String::npos)
					{
						hipFile = hipFile.substr(spos+1);
					}
					if (Ogre::StringUtil::endsWith(hipFile, ".hip"))
					{
						hipFile = hipFile.substr(0, hipFile.size() - 4);
					}
					filename += hipFile;
				}
				if (!Ogre::StringUtil::endsWith(filename, ".mesh"))
				{
					filename += ".mesh";
				}
				mesh.Export(filename, mGenerateEdgeLists, mGenerateTangents, mTangentsSemantic, 
					numFrames, frameStart, mFps, mIkSampleRate);
			}
		}
		catch (Ogre::Exception& e)
		{
			addError(ROP_RENDER_ERROR, e.getFullDescription().c_str());
			cleanUpSingletons();

			return 0;
		}

		return 1;

	}
	//---------------------------------------------------------------------
	ROP_RENDER_CODE HoudiniOgre_ROP::renderFrame(float t, UT_Interrupt*)
	{
		// If we're exporting per-frame...
		if (mSnapshotPerFrame)
		{
			// note num frames set to 0 since we can't sample 
			// skeletal animation if we're snapshotting meshes per frame
			return (ROP_RENDER_CODE)exportGeometries(t, true);
		}

		return ROP_CONTINUE_RENDER;

	}
	//---------------------------------------------------------------------
	ROP_RENDER_CODE HoudiniOgre_ROP::endRender()
	{
		cleanUpSingletons();

		return ROP_CONTINUE_RENDER;
	}
	//---------------------------------------------------------------------
	void HoudiniOgre_ROP::createSingletons()
	{
		if (!mLogMgr)
			mLogMgr = new Ogre::LogManager();
		if (!mResMgr)
			mResMgr = new Ogre::ResourceGroupManager();
		if (!mMeshMgr)
			mMeshMgr = new Ogre::MeshManager();
		if (!mSkelMgr)
			mSkelMgr = new Ogre::SkeletonManager();
		if (!mMatMgr)
			mMatMgr = new Ogre::MaterialManager();
		if (!mBufMgr)
			mBufMgr = new Ogre::DefaultHardwareBufferManager();

	}
	//---------------------------------------------------------------------
	void HoudiniOgre_ROP::cleanUpSingletons()
	{
		delete mBufMgr;
		delete mMatMgr;
		delete mSkelMgr;
		delete mMeshMgr;
		delete mResMgr;
		delete mLogMgr;

		mBufMgr = 0;
		mMatMgr = 0;
		mSkelMgr = 0;
		mMeshMgr = 0;
		mResMgr = 0;
		mLogMgr = 0;
	}


