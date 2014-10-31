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
 *   COMMENTS:	A very Dummy SOP doing nothing
 */

#ifndef __SOP_mySphere_h__
#define __SOP_mySphere_h__

#include <SOP/SOP_Node.h>

class SOP_mySphere : public SOP_Node
{
public:
   
             SOP_mySphere(OP_Network *net, const char *name, OP_Operator *op);
    virtual ~SOP_mySphere();
    static   PRM_Template  myTemplateList[];
    static   OP_Node	  *myConstructor(OP_Network*, const char *,
					 OP_Operator *);

    
protected:
    
    virtual OP_ERROR		 cookMySop(OP_Context &context);

    // declaration of the functions to evaluate the div parameters.
    int DIVX()         { return evalInt( 0, 0, 0 ); }
    int DIVY()         { return evalInt( 0, 1, 0 ); }
    int DIVZ()         { return evalInt( 0, 2, 0 ); }


    
};

#endif
