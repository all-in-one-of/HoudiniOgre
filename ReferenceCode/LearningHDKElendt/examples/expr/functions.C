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
 *   COMMENTS:	Expression functions min and max.
 *              
 */

// As for command we needed basic headers for creating a new expression
#include <math.h>
#include <UT/UT_DSOVersion.h>
#include <EXPR/EXPR.h>
#include <CMD/CMD_Manager.h>


// Callback function to evaluate the max of two numbers
static void
fn_max(EV_FUNCTION *, EV_SYMBOL *result, EV_SYMBOL **argv, int)
{
    // grab the argument 1 and 2 and compare them
    if (argv[0]->value.fval > argv[1]->value.fval)
	result->value.fval = argv[0]->value.fval;
    else 
	result->value.fval = argv[1]->value.fval;
}

// Callback function to evaluate the minimum of two numbers
static void
fn_min(EV_FUNCTION *, EV_SYMBOL *result, EV_SYMBOL **argv, int)
{
    if (argv[0]->value.fval > argv[1]->value.fval)
	result->value.fval = argv[1]->value.fval;
    else 
	result->value.fval = argv[0]->value.fval;
}

// A couple of defines to make life a lot easier for us
#define EVF	EV_TYPEFLOAT // return type in this case a float

// we declared the array of arguments that the function will take.
static int	floatArgs[] = { EVF, EVF };

// We are creating a table with the function we want to register.
static EV_FUNCTION funcTable[] = {
    
    // register the function max and min:
    // 0         is the flag, if your function is time dependant, set it to anything but 0.
    // "max"     is the name of the funtion. 
    // 2         is the number of arguments.
    // EVF       is the return type.
    // floatArgs is the argument type in our case floats.
    // fn_max    is the callback
    EV_FUNCTION(0, "max",	2, EVF,	floatArgs,	fn_max),
    EV_FUNCTION(0, "min",	2, EVF,	floatArgs,	fn_min),
    EV_FUNCTION(),
};


// Initialization of the expressions.
void
CMDextendLibrary(CMD_Manager *)
{
    int		i;
    // go through the items of our table and initialize them
    for (i = 0; funcTable[i].getName(); i++)
	ev_AddFunction(&funcTable[i]);
}
