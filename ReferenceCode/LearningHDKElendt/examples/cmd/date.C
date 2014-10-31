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
 *   COMMENTS:	Hscript command 'date' to return the date from the system.
 *              
 */



#include <UT/UT_DSOVersion.h> 
// header for any new command
#include <CMD/CMD_Manager.h> 
#include <CMD/CMD_Args.h>

// we need this library which will return the system time.
#include <time.h> 


// The callback function should be declared static to avoid possible 
// name clashes with other Houdini functions. The callback takes one 
// argument, a reference to a CMD_Args object. This object has two main 
// functions: to provide access to the command line options for the 
// command and to provide an output stream so you can return output to 
// the Houdini shell.
static 
void cmd_date( CMD_Args &args ) { 
    
    time_t       time_struct;   
    char        *time_string;    
    
        
    int          i;     
    
    // the 's' option left pads with some spaces,     // just for an example    
    if( args.found('s') )     
    { 
	int num_spaces =args.iargp( 's' ); 
	for( i = 0; i < num_spaces ; i++ )     
	    args.out() << " "; 
    }
    
    // call the standard C function 'time'.    
    // see 'man time' for details   
    time_struct = time(0);    
    time_string = ctime( &time_struct );   
    // printout the date to the args out stream    
    args.out() << time_string; 
} 


/*  * this function gets called once during Houdini initialization  */
void CMDextendLibrary( CMD_Manager *cman ) 
{  

    // install the date command into the command manager   
    // name = "date", 
    // options_string = "s:",     
    // callback = cmd_date     
    cman->installCommand("date", "s:", cmd_date ); 

}
