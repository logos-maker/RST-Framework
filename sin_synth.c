
#include <stdint.h>		// For variable declaration names.
#include <stdlib.h>		// For malloc function.
#include <stdio.h>		// For text in window
#include "libs/dsc.h"		// a lib for making oldschool wavetable synthesizers.
#include "libs/ikigui.h"	// cross platform audio plugin GUI library for tiled graphics and animations.
#include "libs/rst.h"		// definitions for making audio plugins compatible with the ABI.


//*********************
//   Plugin settings
//*********************

char brand_name[]   = "RST-PLUG";    		// Place your brand name inside ""
char product_name[] = "RST-TEMPLATE";		// Place your plug name inside ""
#define VERSION_NUMBER_OF_THIS_SPECIFIC_PLUG 1  // Version number for this plug is set to 1. Increase number for your plug when you make improvements.
#define TYPE_OF_PLUG SYNTHESIZER 		// Set this to EFFECT_UNIT or SYNTHESIZER
#define PARAMETER_COL 4
#define PARAMETER_ROW 1
#define NUMBER_OF_PARAMETERS PARAMETER_COL * PARAMETER_ROW  // Uniqe number of parameters in plug.
#define H_DISTANCE 64
#define V_DISTANCE 64

//*********************
//     Plugin GUI
//*********************

// The size of the editor in pixels - Debugging makes the plug wider
#define PLUG_WIDTH  H_DISTANCE*PARAMETER_COL
#define PLUG_HEIGHT V_DISTANCE*PARAMETER_ROW+64

// The graphic art (a normal BMP file converted to array declarations), is in the following files...
#include "gfx/knob.h"	// Embedded graphics for knobs in 32bit AARRGGBB BMP format.
#include "gfx/labels.h"  // Embedded graphics for text labels in 332bit AARRGGBB BMP format.
#include "gfx/font.h"
ikigui_image font;	// Global graphics for monospace text characters.
ikigui_image knob_anim;	// Global graphics for knobs.
ikigui_image bg;	// Global graphics for background art.
ikigui_image labels;	// Global graphics for text label art.

//************************
//     Plugin data 
//************************

struct data{     // Things that uniqe for each plug instance that is needed for this plug.
	ikigui_map font_map; // for textbased statusbar for debugging.
	ikigui_map label_map;// A tilemap for the test labels.
	ikigui_window mywin; // A plugin window declaration.
	ikigui_map knob_map; // A tilemap declaration.
	unsigned short phase; // Current phase in sine wave
	float note ;
	int SampleRate ;
	float volume ;
} data;


float SineTable[256]; // Sine wave lookup table


const float TwoPI=2.0f*3.1415926f;

float midi[127]; // MIDI note to frequency table
//*********************************************************************************
//  Some generic stuff not specific to this plug, no need to change anything here
//*********************************************************************************

struct patch{ // the patch that the DAW will save and restore when saving and loading audio projects.
	float knob[NUMBER_OF_PARAMETERS];
};

typedef struct{ // You don't need to change anything here, It's a some generic declarations that gets allocated when the DAW cratest a new plug instance
	struct plugHeader plughead; // It must be first in the struct. Note that each instance has this header.
	plugPtr (*hostcall) (plugHeader* effect, int32_t opcode, int32_t index, plugPtr value, void* ptr, float opt);
	struct patch pth; // all data to save and restore by host be the op-codes plugGetChunk and plugSetChunk functions.
	struct data dat; // buffers and variables for the audio algorithm.
	struct ERect myrect;
	int program_no; // the current preset number (not used in this plug).
	int knob_selected;
	float samplerate;
} plug_instance;

struct preset{ // general struct used for internal presets in the plug
	char preset_name[24];
	float param[NUMBER_OF_PARAMETERS];
};

char* terminal(plug_instance* plug){
	for(int i = 0 ; i < (plug->dat.font_map.columns*(plug->dat.font_map.rows-1)) ; i++) plug->dat.font_map.map[i] = plug->dat.font_map.map[i+plug->dat.font_map.columns] ; // Linefeed (scroll text up one line).
	char* commandline = &plug->dat.font_map.map[plug->dat.font_map.columns*(plug->dat.font_map.rows-1)]; // Address to the last line of text, where the new message is going to be written. // Line for debugg only
	for(int i = 0 ; i < plug->dat.font_map.columns ; i++) commandline[i]=0; // Clear out old text
	return commandline ; // Address to the last line of text, where the new message is going to be written.
}

//*************************
//     Plugin presets
//*************************
#define NUMBER_OF_PRESETS 4	// Number of presets inside the plug.

struct preset presets[NUMBER_OF_PRESETS] = {	// the preset presets inside the plug. Change NUMBER_OF_PRESETS if changing the number of presets.
	{"FIRST",  0.0, 0.0, 0.0, 0.0, }, // First preset
	{"SECOND", 0.1, 0.1, 0.1, 0.1, }, // Second preset
	{"THIRD",  0.2, 0.2, 0.2, 0.2, }, // ...and so on
	{"FOURTH", 0.3, 0.3, 0.3, 0.3, }, // 
};
void getParameterName(int32_t index,  char* ptr){ // Names of all user parameters in the plug. Max 7 characters for each name(8 if the implicit \n is included), but the spec is futile. Hosts allows longer names for compatibiliy reasons as almost no plugins follows this limit.
        switch(index){ // Copy the name of the the right paramter to be displayed in the host.
                case  0: strcpy(ptr, "PAR 0"); break; // Name of the first  parameter is between ""
                case  1: strcpy(ptr, "PAR 1"); break; // Name of the second parameter is between ""
                case  2: strcpy(ptr, "PAR 2"); break; // ...and so on. 
                case  3: strcpy(ptr, "PAR 3"); break; //

                default: strcpy(ptr, "???");	 break; // A default name, reminding to add create any missing case for some parameter.
        }
};
void getParameterText(plug_instance *plug,int32_t index,char* ptr){ if(NULL!=gcvt(plug->pth.knob[index], 6, ptr)) return; }; // Host whant the indexed parameter value in text.


// *********************************************************
//  Some functions that need to be customized for your plug
// *********************************************************
void set_samplerate(plug_instance *plug){		// Is called by the DAW when it gives you the samplerate your plug needs to use...
	plug->dat.SampleRate = (int)plug->samplerate ;
}

void mouse_handling(plug_instance *plug){		// Mouse handling
        ikigui_get_events(&plug->dat.mywin);		// update window events
	struct mouse* m = &plug->dat.mywin.mouse ;	// Make a short hand name, for the code below

	if(m->left_click){ // Mouse down event
                plug->knob_selected = ikigui_mouse_pos(&plug->dat.knob_map, m->x , m->y);
                if(-1 != plug->knob_selected){ // if mouse pointer was over a tile
                        m->pressed = 1; // That we has sent to the host that we have grabbed something
                        plug->hostcall(&plug->plughead, dawAutomateStart, plug->knob_selected, 0, 0, 0); // Tell host we grabed the knob 
                }
        }
        if(m->pressed){ // Change pressed knob according to relative mouse movement.
                float temp = plug->pth.knob[plug->knob_selected] + (float)(plug->dat.mywin.mouse.old_y - plug->dat.mywin.mouse.y) * 0.01; // Relative mouse movement 
                if(0 > temp)            plug->pth.knob[plug->knob_selected] = 0; // knob can't go below 0.
                else if(1 < temp)       plug->pth.knob[plug->knob_selected] = 1; // knob can't go above 1.
                else                    plug->pth.knob[plug->knob_selected] = temp ; // the new knob value.
                
                plug->hostcall(&plug->plughead, dawAutomate,   plug->knob_selected, 0, 0, plug->pth.knob[plug->knob_selected]); // send new knob value to the DAW.
        }
        if(m->left_release && m->pressed){// Release of mouse button when previus informed to the host
                m->pressed = 0;
                plug->hostcall(&plug->plughead, dawAutomateEnd,   plug->knob_selected, 0, 0, 0); // Tell the DAW that we released the knob.
        }
        for(int i = 0 ; i < NUMBER_OF_PARAMETERS ; i++ ){ // Update the tile map, with all knob values.
                plug->dat.knob_map.map[i] = (char)(plug->pth.knob[i] * plug->dat.knob_map.max_index ); // Select animation frame for knob value.
        }
	if(m->left_click){
		sprintf(terminal(plug),"%3d %3d",plug->dat.mywin.mouse.x,plug->dat.mywin.mouse.y); // Debug demo: Print message on click
	}
}
void draw_graphics(plug_instance *plug){		// The DAW calls this when it wants to redraw the editor...
	ikigui_image_draw(&plug->dat.mywin.frame,&bg, 0, 0);		// Draw background.
	ikigui_map_draw(&plug->dat.knob_map,0,0,0);			// Draw knobs.
	ikigui_map_draw(&plug->dat.font_map,0,PLUG_WIDTH-8*32,64);	// Draw text debugging text.
}
void prepare_graphics(plug_instance *plug,void *ptr){	// The DAW calls this when it wants to open the editor window...

	// Create a background image for the plug - using alpha compositing
	ikigui_image_empty(&bg, PLUG_WIDTH,PLUG_HEIGHT);
	ikigui_draw_gradient(&bg,0x00eeeedd, 0x00999999);
	ikigui_bmp_include(&labels,labels_array); // Load label graphics.
	ikigui_map_init(&plug->dat.label_map, &bg,&labels,0,H_DISTANCE,V_DISTANCE,64,14,PARAMETER_COL,PARAMETER_ROW);
	for(int i = 0 ; i < NUMBER_OF_PARAMETERS ; i++) plug->dat.label_map.map[i] = i; // automap the labels
	ikigui_map_draw(&plug->dat.label_map,0,0,52);	// Draw text labels.

	// For the knob animation
	ikigui_bmp_include(&knob_anim,knob_array); // Load knob graphics.						
	ikigui_map_init(&plug->dat.knob_map, &plug->dat.mywin.frame,&knob_anim,0,H_DISTANCE,V_DISTANCE,64,64,PARAMETER_COL,PARAMETER_ROW); // Set columns and rows of knobs in the tile array, and tile width and hight.

	// For debugging text
	ikigui_bmp_include(&font,font_array);
	ikigui_map_init(&plug->dat.font_map,&plug->dat.mywin.frame,&font,ASCII,0,0,8,8,32,(PLUG_HEIGHT-64)>>3); // 32 col, 8 rows, 8 width, 8 height.
}
void destroy_graphics(plug_instance *plug,void *ptr){	// When the DAW closes the window...

}
void audioplugOpen(plugHeader *plugin){ 		// Is executed when the plug opens
	plug_instance *plug = (plug_instance*)plugin->object;

	// Generate MIDI -> Frequency table
	for(int i=0;i<127;i++)	midi[i]=440.0f*pow(2, ((float)i-69.0f)/12.0f);

	// Generate sine wave table
	for(int i=0;i<256;i++)	SineTable[i]= sin(TwoPI*((float)i/256.0f));

}

//*********************
//    Plugin MIDI
//*********************

// The DAW calls this function when it want to send a MIDI message to the plugin
int32_t MIDI_in(plug_instance* plug,struct plugEvents* ev){		// Take care of incomming MIDI events/messages
	for (int32_t i = 0; i < ev->number_of_events; i++) {		// Parse/loop through all incomming events.
		if ( (ev->MIDIMessages[i])->eventType != 1 ) continue;	// Accept only MIDI messages an no sysex.
		char* midiData = ev->MIDIMessages[i]->MIDIByte;

		switch (midiData[0] & 0xf0) { // Recive on all channels
			case 0x90:  // Note on
				plug->dat.note = midi[midiData[1] & 0x7F];
				plug->dat.volume = (float)(midiData[2] & 0x7F) * 0.007874016f; //(float)(1/127) ;	
				sprintf(terminal(plug),"NOTE ON  %2d, OFFSET %5d ", midiData[1] & 0x7F, ev->MIDIMessages[i]->sample_offset); // Print message on bottom row // See DEBUG
			break;      
			case 0x80:  // Note off
				plug->dat.volume = 0.0f ;					
				sprintf(terminal(plug),"NOTE OFF %2d, OFFSET %5d ", midiData[1] & 0x7F, ev->MIDIMessages[i]->sample_offset); // Print message on bottom row // See DEBUG
			break;
			default:    // Other MIDI message
				sprintf(terminal(plug),"UNKNOWN,  OFFSET %5d ", ev->MIDIMessages[i]->sample_offset); // Print message on bottom row // See DEBUG
			break;
		}
	}
	return 1;
}

//********************
//  Plugin algorithm
//********************

// The DAW calls this function to make the plugin process audio in buffer and fill with audio out data. Audio levels is between -1 to +1
void audio_in_out_float(plugHeader *plugin, float **inputs, float **outputs, int32_t sampleFrames){ plug_instance *plug = (plug_instance*)plugin->object;
	// plug->pth.knob[0] ; // example of getting the value of the first knob

	for(int i = 0; i < sampleFrames; i++){ // Loop trough all the samples in buffer. Put your audio algorithm inside here....
		float output= (float)(SineTable[plug->dat.phase>>8])*plug->dat.volume;
		plug->dat.phase+=(65535.0f*plug->dat.note/plug->dat.SampleRate);
	
		outputs[0][i] = output;
		outputs[1][i] = output;
	}
}

#include "framework.c"