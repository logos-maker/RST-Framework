// A template for audio fx units without MIDI and GUI

#include <stdint.h>	// For variable declaration names.
#include <stdlib.h>	// For malloc function.
#include <stdio.h>	// For text in window
#include <string.h>	// Missing for plug without ikiGUI
#include <math.h>	// Missing for plug
#include "libs/rst.h"	// definitions for making audio plugins compatible with the ABI.

//**************************************************************************************
//    Plugin settings - Info that is uniqe to your plug that the host want's to know
char brand_name[]   = "RST-OEM";    		// Place your brand name inside ""
char product_name[] = "TEMPLATE NIX";		// Place your plug name inside ""
#define VERSION_NUMBER_OF_THIS_SPECIFIC_PLUG 1  // Version number for this plug is set to 1. Increase number for your plug when you make new releases/improvements.
#define TYPE_OF_PLUG EFFECT_UNIT 		// Set this to EFFECT_UNIT or SYNTHESIZER
#define NUMBER_OF_PARAMETERS 1			// How many parameters that the DAW can automate. That will be listed by the host.
#define NUMBER_OF_PRESETS 1			// Number of presets inside the plug. Lowest amount is 1
#define NO_GUI					// If NO_GUI is defined, the GUI editor is turned of <-------- Turns of GUI system functions: mouse_handling() draw_graphics() prepare_graphics()
#define NO_MIDI					// If NO_MIDI is defined, the MIDI handling is turned of, and this only makes sense for a EFFECT_UNIT  <----- Turns of MIDI system function:  MIDI_in()

//**************************************************************************************************************************************************
//   Global variables/data for the plug -  You can only have things here that do not change and is the same for all instances/copies of your plug
//**************************************************************************************************************************************************
  /* Place your global variables here - not anything that's can have different state in different plug instances */


//***************************************************************************************************
//   Plugin data struct  -  Things that uniqe for each plug instance that is needed for this plug
//***************************************************************************************************

struct data{  // Place your variables inside the struct that is needed for the state for your plug (State is things that should be remembered).
  /* Place your variables for your plug state here */
} data;
#include "libs/rst_framework_head.c"	// Has to be below 'struct data'.


//*****************************************************
//   Patch information - Parameters, Names & Values
//*****************************************************

//*********************
//   Plugin presets
struct preset presets[NUMBER_OF_PRESETS] = {	// the preset presets inside the plug. Change NUMBER_OF_PRESETS if changing the number of presets.
	{"FIRST",  0.5, },	// First preset
};

//***********************************************
//   Names of all user parameters in the plug
void getParameterName(int32_t index,  char* ptr){
        switch(index){ // Copy the name of the the right paramter to be displayed in the host.
                case  0: strcpy(ptr, "PAR 0");	break; // Name of the first  parameter is between ""
                default: strcpy(ptr, "???");	break; // A default name, reminding to add create any missing case for some parameter.
        }
};

//********************************************
//   Generate parameter text to show in DAW 
void getParameterText(plug_instance *plug,int32_t index,char* ptr){ if(NULL!=gcvt(plug->pth.knob[index], 6, ptr)) return; }; // Host want the indexed parameter value in text.



//*****************************
//   User defined functions 
//***************************** 
  /* Place your functions here */

//*************************************************************
//   Some functions that need to be customized for your plug
//*************************************************************

void set_samplerate(plug_instance *plug){		// Is called by the DAW when it gives you the samplerate your plug needs to use...
	/* Place your code here thats going to run when when the DAW has given the sample rate in plug->samplerate as a float */
}
void audioplugOpen(plugHeader *plugin){ 		// Is executed when the plug opens
	plug_instance *plug = (plug_instance*)plugin->object;
	/* Place your code here thats going to run when a new instance/plug copy is started */
}
void audioplugClose(plugHeader *plugin){ 		// Is executed when the plug going to be closed
	plug_instance *plug = (plug_instance*)plugin->object;
	/* Place your code here thats going to run before the instance is going to be cloased */
}

//***********************************************************************************************************************************************************************************
//   Plugin algorithm - audio_in_out_float() The DAW calls this function to make the plugin process audio in buffer and fill with audio out data. Audio levels is between -1 to +1
void audio_in_out_float(plugHeader *plugin, float **inputs, float **outputs, int32_t sampleFrames){ plug_instance *plug = (plug_instance*)plugin->object;
	//printf("plug: Host want audio data sampleFrames %d\n",sampleFrames);
	for(int i = 0; i < sampleFrames; i++){ // Loop trough all the samples in buffer. Put your audio algorithm inside here....
		float sample = 0.0f ;
		outputs[LEFT ][i] = inputs[LEFT ][i] ;
		outputs[RIGHT][i] = inputs[RIGHT][i] ;
	}
}

#include "libs/rst_framework.c"