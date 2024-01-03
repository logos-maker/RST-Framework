#include <stdint.h>	// For variable declaration names.
#include <stdlib.h>	// For malloc function.
#include <stdio.h>	// For text in window
#include "libs/rst.h"	// definitions for making audio plugins compatible with the ABI.
#include "libs/ikigui.h"// cross platform audio plugin GUI library for tiled graphics and animations.

//**************************************************************************************
//    Plugin settings - Info that is uniqe to your plug that the host want's to know
char brand_name[]   = "DSC";    		// Place your brand name inside ""
char product_name[] = "DSC-TRANSPORT";		// Place your plug name inside ""
#define VERSION_NUMBER_OF_THIS_SPECIFIC_PLUG 1  // Version number for this plug is set to 1. Increase number for your plug when you make new releases/improvements.
#define TYPE_OF_PLUG SYNTHESIZER 		// Set this to EFFECT_UNIT or SYNTHESIZER
#define NUMBER_OF_PARAMETERS 1			// How many parameters that the DAW can automate. That will be listed by the host.
#define NUMBER_OF_PRESETS 1			// Number of presets inside the plug. Lowest amount is 1

//**************************************************************************************************************************************************
//   Global variables/data for the plug -  You can only have things here that do not change and is the same for all instances/copies of your plug
//**************************************************************************************************************************************************
  /* Place your global variables here - not anything that's can have different state in different plug instances */


//******************************
//   Plugin GUI declarations

  /* Place your declations for your GUI here */

// The size of the editor in pixels - Debugging makes the plug wider
#define PLUG_WIDTH  8*32
#define PLUG_HEIGHT 8*16

// The graphic art (a normal BMP file converted to array declarations), is in the following files...
#include "gfx/font.h"   // Embedded graphics for monospace font in 32bit AARRGGBB BMP format.
ikigui_image font;	// Global graphics for monospace text characters.
ikigui_image bg;	// Global graphics for background art.



//***************************************************************************************************
//   Plugin data struct  -  Things that uniqe for each plug instance that is needed for this plug
//***************************************************************************************************

struct data{  // Place your variables inside the struct that is needed for the state for your plug (State is things that should be remembered). 
  /* Place your variables for your plug state here */
	ikigui_map font_map; // for textbased statusbar for debugging.
	ikigui_window mywin; // A plugin window declaration.
	int set_off;
} data;
#include "libs/rst_framework_head.c"	// Has to be below 'struct data'.

//*****************************************************
//   Patch information - Parameters, Names & Values
//*****************************************************

//*********************
//   Plugin presets
struct preset presets[NUMBER_OF_PRESETS] = {	// the preset presets inside the plug. Change NUMBER_OF_PRESETS if changing the number of presets.
	{"NOT USED",  0.5, },	// First preset
};

//***********************************************
//   Names of all user parameters in the plug
void getParameterName(int32_t index,  char* ptr){
        switch(index){ // Copy the name of the the right paramter to be displayed in the host.
                case  0: strcpy(ptr, "PAR 0"); break; // Name of the first  parameter is between ""
                default: strcpy(ptr, "???");	 break; // A default name, reminding to add create any missing case for some parameter.
        }
};

//********************************************
//   Generate parameter text to show in DAW 
void getParameterText(plug_instance *plug,int32_t index,char* ptr){ if(NULL!=gcvt(plug->pth.knob[index], 6, ptr)) return; }; // Host want the indexed parameter value in text.

//*****************************
//   User defined functions  
//*****************************
  /* Place your functions here */

//***********************************************
//   Some functions for administative purposes
//***********************************************

void set_samplerate(plug_instance *plug){		// Is called by the DAW when it gives you the samplerate your plug needs to use...
	/* Place your code here thats going to run when when the DAW has given the sample rate in plug->samplerate as a float */
}
void audioplugOpen(plugHeader *plugin){ 		// Is executed when the plug opens
	/* Place your code here thats going to run when a new instance/plug copy is started */
}
void audioplugClose(plugHeader *plugin){ 		// Is executed when the plug going to be closed
	plug_instance *plug = (plug_instance*)plugin->object;
	/* Place your code here thats going to run before the instance is going to be cloased */
}


//**************************
//   GUI related functions
//**************************

void mouse_handling(plug_instance *plug){		// Mouse handling
        ikigui_get_events(&plug->dat.mywin);		// update window events
	/* Place your code here */
}
void draw_graphics(plug_instance *plug){		// The DAW calls this when it wants to redraw the editor...
	ikigui_image_draw(&plug->dat.mywin.frame,&bg, 0, 0);			// Draw background.

	daw_transport *transport;
	transport = (daw_transport*)plug->hostcall(&plug->plughead, dawGetTime,   0, 0, 0, 0);
	char row = 0;
	char* commandline = commandline= &plug->dat.font_map.map[plug->dat.font_map.columns*row];
	sprintf(commandline,"PLAY SAMPLE %12.4f",transport->play_sample);
	commandline += plug->dat.font_map.columns; // new line
	sprintf(commandline,"SAMPLE RATE %12.4f",transport->play_samplerate);
	commandline += plug->dat.font_map.columns; // new line
	sprintf(commandline,"SYSTEM TIME %16.0f",transport->system_clock_ns); // Time since the computer started
	commandline += plug->dat.font_map.columns; // new line
	sprintf(commandline,"PLAY PPQ    %12.4f",transport->play_ppq);
	commandline += plug->dat.font_map.columns; // new line
	sprintf(commandline,"PLAY BPM    %12.4f",transport->play_bpm);
	commandline += plug->dat.font_map.columns; // new line
	sprintf(commandline,"PLAY START  %12.4f",transport->ppq_play_start);
	commandline += plug->dat.font_map.columns; // new line
	sprintf(commandline,"LOOP START  %12.4f",transport->ppq_loop_start);
	commandline += plug->dat.font_map.columns; // new line
	sprintf(commandline,"LOOP END    %12.4f",transport->ppq_loop_end);
	commandline += plug->dat.font_map.columns; // new line
	sprintf(commandline,"LOOP END    %12.4f",transport->ppq_loop_end);

	commandline += plug->dat.font_map.columns;
	if(transport->flags & DAW_PLAYING) sprintf(commandline,"DAW is Playing");
	else sprintf(commandline,"DAW is Stopped");

	commandline += plug->dat.font_map.columns;;
	if(transport->flags & DAW_LOOPING) sprintf(commandline,"DAW Loop on ");
	else sprintf(commandline,"DAW Loop off");

	commandline += plug->dat.font_map.columns;
	if(transport->flags & DAW_RECORDING) sprintf(commandline,"DAW is Recording    ");
	else sprintf(commandline,"DAW is not recording");

	if(transport->flags & DAW_CHANGED) plug->dat.set_off = 1;
	commandline += plug->dat.font_map.columns;

	if(plug->dat.set_off) sprintf(commandline,"Transport changed before  ");
	else sprintf(commandline,"Transport not ever changed");

	commandline += plug->dat.font_map.columns;
	if(transport->flags & DAW_AUTOMATION_WRITE) sprintf(commandline,"DAW AUTO READ       ");
	else sprintf(commandline,"DAW AUTO NOT READING");

	commandline += plug->dat.font_map.columns;
	if(transport->flags & DAW_AUTOMATION_READ) sprintf(commandline,"DAW AUTO WRITE      ");
	else sprintf(commandline,"DAW AUTO NOT WRITING");

	ikigui_map_draw(&plug->dat.font_map,0,0,0);// Draw text debugging text.
}
void prepare_graphics(plug_instance *plug,void *ptr){	// The DAW calls this when it wants to open the editor window...

	// Create a background image for the plug - using alpha compositing
	ikigui_image_empty(&bg, PLUG_WIDTH,PLUG_HEIGHT);
	ikigui_draw_gradient(&bg,0x00eeeedd, 0x00999999);

	// Character display
	ikigui_bmp_include(&font,font_array);
	ikigui_map_init(&plug->dat.font_map,&plug->dat.mywin.frame,&font,ASCII,0,0,8,8,32,16); // 32 col, 8 rows, 8 width, 8 height.
}
void destroy_graphics(plug_instance *plug,void *ptr){	// When the DAW closes the window...

}

//***********************************************
//   Audio & MIDI port communciation functions  
//***********************************************

//*************************************************************************************************************
//    Plugin MIDI - MIDI_in() The DAW calls this function when it want to send a MIDI message to the plugin.
int32_t MIDI_in(plug_instance* plug,plugEvents* ev){		// Take care of incomming MIDI events/messages
	for (int32_t i = 0; i < ev->number_of_events; i++) {		// Parse/loop through all incomming events.
		if ( (ev->MIDIMessages[i])->eventType != 1 ) continue;	// Accept only MIDI messages an no sysex.
		char* midiData = ev->MIDIMessages[i]->MIDIByte;

		switch (midiData[0] & 0xf0) { // Recive on all channels
			case 0x90:  // Note on

			break;      
			case 0x80:  // Note off

			break;
			default:    // Other types of MIDI messages

			break;
		}
	}
	return 1;
}

//***********************************************************************************************************************************************************************************
//   Plugin algorithm - audio_in_out_float() The DAW calls this function to make the plugin process audio in buffer and fill with audio out data. Audio levels is between -1 to +1
void audio_in_out_float(plugHeader *plugin, float **inputs, float **outputs, int32_t sampleFrames){ plug_instance *plug = (plug_instance*)plugin->object;
	//printf("plug: Host want audio data sampleFrames %d\n",sampleFrames);
	for(int i = 0; i < sampleFrames; i++){ // Loop trough all the samples in buffer. Put your audio algorithm inside here....
		outputs[0][i] = 0.0f;
		outputs[1][i] = 0.0f;
	}
}

#include "libs/rst_framework.c"