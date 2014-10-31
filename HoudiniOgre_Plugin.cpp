/*
-----------------------------------------------------------------------------
HoudiniOgre_Plugin.cpp

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

// Houdini includes
#include "OgreNoMemoryMacros.h"
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include "OgreMemoryMacros.h"

// Ensure entry points HoudiniDSOVersion and HoudiniGetTagInfo are defined
// hcustom does this 'under the covers' by the looks via the VERSION tag 
#define UT_DSO_TAGINFO "Produced by Torus Knot Software for EDM Studio Inc"
#include <UT/UT_DSOVersion.h>


// This file contains the main DLL entry points required by Houdini

// ROP registration entry point
void DLLEXPORT newDriverOperator(OP_OperatorTable *table)
{
	// Install custom operators here
	HoudiniOgre_ROP::installOperator(table);
}

