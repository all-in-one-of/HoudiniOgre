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
 *   COMMENTS:	A SOP that allows use to create a sphere or a cube
 *              according to the will of the user.
 */

#ifndef __SOP_mySphereCube_h__
#define __SOP_mySphereCube_h__

#include <SOP/SOP_Node.h>

class SOP_mySphereCube : public SOP_Node
{
public:
   
             SOP_mySphereCube(OP_Network *net, const char *name, OP_Operator *op);
    virtual ~SOP_mySphereCube();
    static   PRM_Template  myTemplateList[];
    static   OP_Node	  *myConstructor(OP_Network*, const char *,
					 OP_Operator *);

    
protected:


    virtual unsigned		 disableParms();
    virtual OP_ERROR		 cookMySop(OP_Context &context);

    // declaration of the functions to evaluate the div parameters.
    int     DIVX()          { return evalInt(   0, 0, 0 ); }
    int     DIVY()          { return evalInt(   0, 1, 0 ); }
    int     DIVZ()          { return evalInt(   0, 2, 0 ); }
   
    int	    OPTIONS()       { return evalInt(   1, 0, 0 ); }

    // we evaluate this parameter using the time, so we can animate it
    float   SIZE( float t ) { return evalFloat( 2, 0, t ); }
    
};

#endif
