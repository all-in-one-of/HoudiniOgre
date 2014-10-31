/*
 * PROPRIETARY INFORMATION.  This software is proprietary to
 * Side Effects Software Inc., and is not to be reproduced,
 * transmitted, or disclosed in any way without written permission.
 *
 * Produced by:
 *      Side Effects
 *      123 Front St. West,
 *      Suite 1401
 *      Toronto, Ontario
 *      Canada M5J 2M2
 *      Tel: (416) 504 9876
 *
 *   NAME:	SOP library (C++)
 *
 *   COMMENTS:	A simple SOP to create a sphere and control the divisions
 */

// Some headers are needed
#include <UT/UT_DSOVersion.h>      // every DSO needs this header.
#include <PRM/PRM_Include.h>       // we will include some parameters 
#include <OP/OP_Operator.h>        // we are creating a new OP
#include <OP/OP_OperatorTable.h>   // and we need to register it in Houdini.

// headers we needed for the standalone application.
#include <UT/UT_Math.h>
#include <GU/GU_Detail.h>

#include "SOP_mySphere.h"

// Where the new Operator will be defined.
// new entries to the operator table for a given type of network. 
// Each entry in the table is an object of class OP_Operator which 
// basically defines everything Houdini requires in order to create nodes of the new type

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator("mysphere",                     // The short operator name need for Houdini
				       "mySphere",                     // Readable version of the name(for menus and such)
				       SOP_mySphere::myConstructor,    // method which constructs nodes of this type
				       SOP_mySphere::myTemplateList,   // A list of the templates defining the parameters to this operator.
	                               0,                              // Minimum  number of inputs required
		                       0,                              // maximum number of inputs required
		                       0));    // A list of any local variables used by the operator
		  
}

// parameters labelling
static PRM_Name        names[] = 
{
    // division = short parameter name recognized by Houdini
    // Division X/Y/Z is a more readable label for the user.
    PRM_Name("division",  "Division X/Y/Z"),
};


// declare some defaults values for the divisions parmater.
// If nothing is set, the default values will be zero.
static PRM_Default     divDefaults[] = 
{
    // we set a default for each component.
    PRM_Default(20), PRM_Default(40), PRM_Default(20)
};

// we can limit the user to set the divisions according to a range.
// Check PRM_Range.h for the options.
PRM_Range  divRange(PRM_RANGE_RESTRICTED, 0, PRM_RANGE_RESTRICTED, 100);

// TemplateList will be where all the parameters are defined.
PRM_Template
SOP_mySphere::myTemplateList[] = {

    // PRM_XYZ: Parameter type XYZ see $HFS/toolkit/html/op/prm.html
    // for the different types available
    // 3: it has 3 components, X, Y and Z.
    // &names[0]: we will associate the first PRM_NAME declared above.
    // PRM_Template can take more parameters, see PRM_Template.h
    PRM_Template(PRM_XYZ,    3, &names[0], divDefaults, 0, &divRange ),
};


// Node Constructor 
OP_Node *
SOP_mySphere::myConstructor( OP_Network *net, const char *name, OP_Operator *op )
{
    return new SOP_mySphere(net, name, op);
}

// Constructor for mySphere
SOP_mySphere::SOP_mySphere( OP_Network *net, const char *name, OP_Operator *op )
    : SOP_Node(net, name, op)
{}

// Destructor
SOP_mySphere::~SOP_mySphere() {}


// We include our sphere function here.
static
float sphere(const UT_Vector3 &p)
{
    float        x, y, z;
    x = p.x();
    y = p.y();
    z = p.z();

    return x*x + y*y + z*z- 1;
}


// Main where all the "cooking" will happen.
OP_ERROR
SOP_mySphere::cookMySop(OP_Context &context)
{ 
    // we include what was in the main() here.

    UT_BoundingBox        bbox;
    
    // This time we have divx, divy and divz as parameters.
    int                   divx, divy, divz;
    
    // because every SOP will have a gdp declared already
    // we do not need to re-declare it but it needs to be 
    // initialized using clearAndDestroy().
    gdp->clearAndDestroy();
    
    // Evaluate the values from the fields.
    divx = DIVX();
    divy = DIVY();
    divz = DIVZ();
        
    bbox.initBounds(   -1, -1, -1 );
    bbox.enlargeBounds( 1,  1,  1 );
    
    
    // build the geometry.
    gdp->polyIsoSurface(sphere, bbox, divx, divy, divz);
  
    return error();
}
