#define DEBUG_1
#ifdef DEBUG_1
	#define DEBUG_2
	#define DEBUG_3
#endif
   
/* Header FILES */
   
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <Xm/Form.h>    /* Motif Form widget. */
#include <Xm/Frame.h>   /* Motif Frame widget. */
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>  /* For XA_RGB_DEFAULT_MAP. */
#include <X11/Xmu/StdCmap.h>  /* For XmuLookupStandardColormap. */
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <GL/GLwMDrawA.h>  /* Motif OpenGL drawing area. */

#include "mrilib.h"
#include "SUMA_suma.h"

/* CODE */



SUMA_SurfaceViewer *SUMAg_cSV; /*!< Global pointer to current Surface Viewer structure*/
SUMA_SurfaceViewer *SUMAg_SVv; /*!< Global pointer to the vector containing the various Surface Viewer Structures */
int SUMAg_N_SVv; /*!< Number of SVs stored in SVv */
SUMA_DO *SUMAg_DOv;	/*!< Global pointer to Displayable Object structure vector*/
int SUMAg_N_DOv = 0; /*!< Number of DOs stored in DOv */
SUMA_CommonFields *SUMAg_CF; /*!< Global pointer to structure containing info common to all viewers */

void SUMA_SetcSV (Widget w, XtPointer clientData, XEvent * event, Boolean * cont);

void SUMA_usage ()
   
  {/*Usage*/
          printf ("\n\33[1mUsage: \33[0m SUMA \n\t-spec <spec file> [-vp/-sa <VolParent>] [-ha AfniHostName]<\n\n");
			 SUMA_VolSurf_help(NULL);
			 SUMA_help_message(NULL);
			 printf ("\t  Version 1.2 Jan 15/02 -- Dec 31/01\n\n");
			 printf ("\t   0.2: works for showing a surface, using GLUT\n");
          printf ("\t Ziad S. Saad SSCC/NIMH/NIH ziad@nih.gov \tThu Dec 27 16:21:01 EST 2001 \n");
          exit (0);
  }/*Usage*/
     




/*!\**
File : SUMA.c
\author : Ziad Saad
Date : Thu Dec 27 16:21:01 EST 2001
   
Purpose : 
   
   
   
Input paramters : 
\param   
\param   
   
Usage : 
		SUMA ( )
   
   
Returns : 
\return   
\return   
   
Support : 
\sa   OpenGL prog. Guide 3rd edition
\sa   varray.c from book's sample code
   
Side effects : 
   
   
   
***/
int main (int argc,char *argv[])
{/* Main */
   static char FuncName[]={"SUMA"}; 
	int kar;
	SUMA_SurfaceObject *SO;
	SUMA_Axis *EyeAxis;
	SUMA_SFname *SF_name;
	SUMA_Boolean brk, SurfIn;
	char *VolParName, *NameParam, *specfilename = NULL, *AfniHostName;
	SUMA_SurfSpecFile Spec;   
	 	
  
   if (argc < 3)
       {
          SUMA_usage ();
          exit (1);
       }
	
	/* initialize Volume Parent and AfniHostName to nothing */
	VolParName = NULL;
	AfniHostName = NULL; 
	
	/* Allocate space for DO structure */
	SUMAg_DOv = Alloc_DisplayObject_Struct (SUMA_MAX_DISPLAYABLE_OBJECTS);
	
	/* allocate space for CommonFields structure */
	SUMAg_CF = SUMA_Create_CommonFields ();
	if (SUMAg_CF == NULL) {
		fprintf(SUMA_STDERR,"Error %s: Failed in SUMA_Create_CommonFields\n", FuncName);
		exit(1);
	}
	
	/* read in the surfaces */
	kar = 1;
	brk = NOPE;
	SurfIn = NOPE;
	while (kar < argc) { /* loop accross command ine options */
		/*fprintf(stdout, "%s verbose: Parsing command line...\n", FuncName);*/
		if (strcmp(argv[kar], "-h") == 0 || strcmp(argv[kar], "-help") == 0) {
			SUMA_usage ();
          exit (1);
		}
		
		if (!brk && (strcmp(argv[kar], "-vp") == 0 || strcmp(argv[kar], "-sa") == 0))
		{
			kar ++;
			if (kar >= argc)  {
		  		fprintf (SUMA_STDERR, "need argument after -vp or -sa ");
				exit (1);
			}
			VolParName = argv[kar];
			/*fprintf(SUMA_STDOUT, "Found: %s\n", VolParName);*/

			brk = YUP;
		}		
		
		if (!brk && strcmp(argv[kar], "-ah") == 0)
		{
			kar ++;
			if (kar >= argc)  {
		  		fprintf (SUMA_STDERR, "need argument after -ah ");
				exit (1);
			}
			AfniHostName = argv[kar];
			/*fprintf(SUMA_STDOUT, "Found: %s\n", AfniHostName);*/

			brk = YUP;
		}	
		if (!brk && strcmp(argv[kar], "-spec") == 0)
		{ 
			kar ++;
		  if (kar >= argc)  {
		  		fprintf (SUMA_STDERR, "need argument after -spec ");
				exit (1);
			}
			
			specfilename = argv[kar];
			/*fprintf(SUMA_STDOUT, "Found: %s\n", specfilename);*/
			brk = YUP;
		} 
		

		if (!brk) {
			fprintf (SUMA_STDERR,"Error %s: Option %s not understood. Try -help for usage\n", FuncName, argv[kar]);
			exit (1);
		} else {	
			brk = NOPE;
			kar ++;
		}
		
	}/* loop accross command ine options */

	
	if (specfilename == NULL) {
		fprintf (SUMA_STDERR,"Error %s: No spec filename specified.\n", FuncName);
		exit(1);
	}

	if(!SUMA_Assign_AfniHostName (SUMAg_CF, AfniHostName)) {
		fprintf (SUMA_STDERR, "Error %s: Failed in SUMA_Assign_AfniHostName", FuncName);
		exit (1);
	}
	
	/* load the specs file and the specified surfaces*/
		/* Load The spec file */
		if (!SUMA_Read_SpecFile (specfilename, &Spec)) {
			fprintf(SUMA_STDERR,"Error %s: Error in SUMA_Read_SpecFile\n", FuncName);
			exit(1);
		}	

		/* make sure only one group was read in */
		if (Spec.N_Groups != 1) {
			fprintf(SUMA_STDERR,"Error %s: One and only one group of surfaces is allowed at the moment (%d found).\n", FuncName, Spec.N_Groups);
			exit(1);
		}
		
		/* load the surfaces specified in the specs file, one by one*/			
		if (!SUMA_LoadSpec (&Spec, SUMAg_DOv, &SUMAg_N_DOv, VolParName)) {
			fprintf(SUMA_STDERR,"Error %s: Failed in SUMA_LoadSpec.\n", FuncName);
			exit(1);
		}
	
	/* create an Eye Axis DO */
	EyeAxis = SUMA_Alloc_Axis ("Eye Axis");
	if (EyeAxis == NULL) {
		SUMA_error_message (FuncName,"Error Creating Eye Axis",1);
		exit(1);
	}
		
	/* Store it into SUMAg_DOv */
	if (!SUMA_AddDO(SUMAg_DOv, &SUMAg_N_DOv, (void *)EyeAxis,  AO_type, SUMA_SCREEN)) {
		SUMA_error_message (FuncName,"Error Adding DO", 1);
		exit(1);
	}
	/*fprintf (SUMA_STDERR, "SUMAg_N_DOv = %d created\n", SUMAg_N_DOv);*/


	/* Allocate space (and initialize) Surface Viewer Structure */
	SUMAg_SVv = SUMA_Alloc_SurfaceViewer_Struct (SUMA_MAX_SURF_VIEWERS);
  
	/* Set the Current SV pointer to the very first SV*/
	SUMAg_cSV = &SUMAg_SVv[0];

 	/* Check on initialization */
	/*Show_SUMA_SurfaceViewer_Struct (SUMAg_cSV, stdout);*/

	#if 0
	/* Register all DOs with SV */
	for (kar=0; kar < SUMAg_N_DOv; ++kar) {
		if (!SUMA_RegisterDO(kar, SUMAg_cSV)) {
			SUMA_error_message (FuncName,"Failed to register DO", 1);
			exit(1);
		}
	}
	#endif
	#if 0
	/* register only the first surface and the remaining DOs */
	{
		SUMA_Boolean SurfIn = NOPE;
		for (kar=0; kar < SUMAg_N_DOv; ++kar) {
			if (!SUMA_isSO(SUMAg_DOv[kar]) || !SurfIn)
			{ /* register the first surface only and other non SO objects */
				/*fprintf(SUMA_STDERR," to register DOv[%d] ...\n", kar);*/
				if (!SUMA_RegisterDO(kar, SUMAg_cSV)) {
					SUMA_error_message (FuncName,"Failed to register DO", 1);
					exit(1);
				}
			}
			if (SUMA_isSO(SUMAg_DOv[kar])) { SurfIn = YUP; }
		}
	}	
	#endif 
	
	#if 1
	/* register all surface specs */
		/*fprintf(SUMA_STDERR,"%s: Registering SpecSO ...", FuncName);*/
		if (!SUMA_RegisterSpecSO(&Spec, SUMAg_cSV, SUMAg_DOv, SUMAg_N_DOv)) {
			fprintf(SUMA_STDERR,"Error %s: Failed in SUMA_RegisterSpecSO.\n", FuncName);
			exit(1);
		} 
		/*fprintf(SUMA_STDERR,"%s: Done.\n", FuncName);*/
		
	/* register all SOs of the first state */	
		/*fprintf(SUMA_STDERR,"%s: Registering All SO of the first group ...", FuncName);*/
		SUMAg_cSV->State = SUMAg_cSV->VSv[0].Name;
		SUMAg_cSV->iState = 0;
		for (kar=0; kar < SUMAg_cSV->VSv[0].N_MembSOs; ++ kar) {
			/*fprintf(SUMA_STDERR," About to register DOv[%d] ...\n", SUMAg_cSV->VSv[0].MembSOs[kar]);*/
				if (!SUMA_RegisterDO(SUMAg_cSV->VSv[0].MembSOs[kar], SUMAg_cSV)) {
					SUMA_error_message (FuncName,"Failed to register DO", 1);
					exit(1);
				}
		}
	/*	fprintf(SUMA_STDERR,"%s: Done.\n", FuncName);*/
		
	/* register all non SO objects */
	/*	fprintf(SUMA_STDERR,"%s: Registering All Non SO ...", FuncName);*/
		for (kar=0; kar < SUMAg_N_DOv; ++kar) {
			if (!SUMA_isSO(SUMAg_DOv[kar]))
			{ 
				/*fprintf(SUMA_STDERR," About to register DOv[%d] ...\n", kar);*/
				if (!SUMA_RegisterDO(kar, SUMAg_cSV)) {
					SUMA_error_message (FuncName,"Failed to register DO", 1);
					exit(1);
				}
			}
		}
	/*	fprintf(SUMA_STDERR,"%s: Done.\n", FuncName);*/
	#endif
	
	/* Set the Rotation Center */
	if (!SUMA_UpdateRotaCenter(SUMAg_cSV, SUMAg_DOv, SUMAg_N_DOv)) {
		SUMA_error_message (FuncName,"Failed to update center of rotation", 1);
		exit(1);
	}
	
	/* set the viewing points */
	if (!SUMA_UpdateViewPoint(SUMAg_cSV, SUMAg_DOv, SUMAg_N_DOv)) {
		SUMA_error_message (FuncName,"Failed to update view point", 1);
		exit(1);
	}

	/* Change the defaults of the eye axis to fit standard EyeAxis */
	SUMA_EyeAxisStandard (EyeAxis, SUMAg_cSV);

	
	/* Set the Current SO pointer to the first object read, tiz a surface of course*/
	SUMAg_cSV->Focus_SO_ID = 0;
	

  /* Create the Surface Viewer Window */
  if (!SUMA_X_SurfaceViewer_Create (SUMAg_cSV, argc, argv)) {
  	fprintf(stderr,"Error in SUMA_X_SurfaceViewer_Create. Exiting\n");
	return 1;
  }
  /* One Surface Viewer created and initialized */
  SUMAg_N_SVv += 1;
  
	/*Main loop */
	XtAppMainLoop(SUMAg_cSV->X->APP);

	
	/* Done, clean up time */
	  
	if (!SUMA_Free_Displayable_Object_Vect (SUMAg_DOv, SUMAg_N_DOv)) SUMA_error_message(FuncName,"DO Cleanup Failed!",1);
	if (!SUMA_Free_SurfaceViewer_Struct_Vect (SUMAg_SVv, SUMAg_N_SVv)) SUMA_error_message(FuncName,"SUMAg_SVv Cleanup Failed!",1);
	if (!SUMA_Free_CommonFields(SUMAg_CF)) SUMA_error_message(FuncName,"SUMAg_CF Cleanup Failed!",1);
	
  return 0;             /* ANSI C requires main to return int. */
}/* Main */ 


