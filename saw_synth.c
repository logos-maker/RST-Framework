//*******************************************************
//     Debug mode, to write text in the plugin editor  
//*******************************************************

//#define DEBUG // remove commentation for debugging
#ifdef DEBUG
	#define PRINT sprintf
	#define DEBUG_W 8*32
	#include <stdio.h>
#else
	#define DEBUG_W 0
#endif

#include <stdint.h>		// For variable declaration names.
#include <stdlib.h>		// For malloc function.
#include "libs/dsc.h"		// a lib for making oldschool wavetable synthesizers.
#include "libs/ikigui.h"	// cross platform audio plugin GUI library for tiled graphics and animations.
#include "libs/rst.h"		// definitions for making audio plugins compatible with the ABI.


//*********************
//   Plugin settings
//*********************

char brand_name[]   = "DSC";    		// Place your brand name inside ""
char product_name[] = "DSC-SAW";		// Place your plug name inside ""
#define VERSION_NUMBER_OF_THIS_SPECIFIC_PLUG 1  // Version number for this plug is set to 1. Increase number for your plug when you make improvements.
#define TYPE_OF_PLUG SYNTHESIZER 		// Set this to EFFECT_UNIT or SYNTHESIZER
#define PARAMETER_COL 5
#define PARAMETER_ROW 2
#define NUMBER_OF_PARAMETERS PARAMETER_COL * PARAMETER_ROW  // Uniqe number of parameters in plug.
#define H_DISTANCE 70
#define V_DISTANCE 85


//*********************
//     Plugin GUI
//*********************

// The size of the editor in pixels - Debugging makes the plug wider
#define PLUG_WIDTH  H_DISTANCE*PARAMETER_COL+DEBUG_W+20
#define PLUG_HEIGHT V_DISTANCE*PARAMETER_ROW+20

// The graphic art (a normal BMP file converted to array declarations), is in the following files...
#include "gfx/knob.h"	 // Embedded graphics for knobs in 32bit AARRGGBB BMP format.
#include "gfx/labels.h"  // Embedded graphics for text labels in 332bit AARRGGBB BMP format.

#ifdef DEBUG
#include "gfx/font.h"
ikigui_image font;	// Global graphics for monospace text characters.
#endif

ikigui_image knob_anim;	// Global graphics for knobs.
ikigui_image bg;	// Global graphics for background art.
ikigui_image labels;	// Global graphics for text label art.


//************************
//     Plugin data 
//************************

struct data{     // Things that uniqe for each plug instance that is needed for this plug.
    // For graphics
    ikigui_map font_map; // for textbased statusbar for debugging.
    ikigui_map label_map;// A tilemap for the test labels.
    ikigui_map knob_map; // A tilemap declaration.
    ikigui_window mywin; // A plugin window declaration.

    // For audio
    dsc_voice voice[128] ; // declare a dsc_voice structs, one for each MIDI note.
    dsc_voice fenv[128] ;
    __u32 freqtab[256];    // a C array to note/lfo values.
    __s8 osc[256];	   // a C array to hold waveform samples for the oscillator.
    dsc_adsr a_adsr;	   // a envelope struct/object.
    dsc_adsr f_adsr;
    __s8 lfo_wave[256];	   // a waveform for modulating the pulse width of the oscillator.
    dsc_lfo lfo;	   // a LFO struct/object.
    float filt_buff[128][16];   // stereo buffer for filter
} data;


//*********************************************************************************
//  Some generic stuff not specific to this plug, no need to change anything here
//*********************************************************************************

struct patch{ // the patch that the DAW will save and restore when saving and loading audio projects.
    float knob[NUMBER_OF_PARAMETERS];
};
struct my_mouse{
    int pressed ;
    int down_x ;
    int down_y ;
    int old_button_press; // What the mouse buttons was before
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

#ifdef DEBUG
char* terminal(plug_instance* plug){
	for(int i = 0 ; i < (32*(plug->dat.font_map.rows-1)) ; i++) plug->dat.font_map.map[i] = plug->dat.font_map.map[i+32] ; // Linefeed (scroll text up one line).
	return &plug->dat.font_map.map[32*(plug->dat.font_map.rows-1)]; // Address to the last line of text, where the new message is going to be written. // Line for debugg only
}
#endif

//*************************
//     Plugin presets
//*************************
#define NUMBER_OF_PRESETS 4	// Number of presets inside the plug.

struct preset presets[NUMBER_OF_PRESETS] = {	// the preset presets inside the plug. Change NUMBER_OF_PRESETS if changing the number of presets.
	{"SHORT",  0.1, 0.22, 0.07, 0.77, 0.0, 0.22, 0.27, 0.77 }, // First preset
	{"SPIKE",  0.1, 0.22, 0.07, 0.77, 0.0, 0.22, 0.27, 0.77 }, // Second preset
	{"SLOW",   0.6, 0.7,  0.1,  0.7,  0.0, 0.22, 0.27, 0.77 }, // ...and so on
	{"FAST",   0.7, 0.7,  0.1,  0.5,  0.0, 0.22, 0.27, 0.77 }, // 
};
void getParameterName(int32_t index,  char* ptr){ // Names of all user parameters in the plug. Max 7 characters for each name(8 if the implicit \n is included), but the spec is futile. Hosts allows longer names for compatibiliy reasons as almost no plugins follows this limit.
        switch(index){ // Copy the name of the the right paramter to be displayed in the host.
                case  0: strcpy(ptr, "AMP-ATTACK "); break; // Name of the first  parameter is between ""
                case  1: strcpy(ptr, "AMP-DECAY  "); break; // Name of the second parameter is between ""
                case  2: strcpy(ptr, "AMP-SUSTAIN"); break; // ...and so on. 
                case  3: strcpy(ptr, "AMP-RELEASE"); break; //
                case  4: strcpy(ptr, "FIL-ATTACK "); break; // Name of the first  parameter is between ""
                case  5: strcpy(ptr, "FIL-DECAY  "); break; // Name of the second parameter is between ""
                case  6: strcpy(ptr, "FIL-SUSTAIN"); break; // ...and so on. 
                case  7: strcpy(ptr, "FIL-RELEASE"); break; //
                default: strcpy(ptr, "???");	 break; // A default name, reminding to add create any missing case for some parameter.
        }
};
void getParameterText(plug_instance *plug,int32_t index,char* ptr){ if(NULL!=gcvt(plug->pth.knob[index], 6, ptr)) return; }; // Host whant the indexed parameter value in text.



// *******************************
//  Global variables for the plug
// *******************************
__u32 rate[256];
ikigui_rect hitbox;

// *********************************************************
//  Some functions that need to be customized for your plug
// *********************************************************
void set_samplerate(plug_instance *plug){ // Is called by the DAW when it gives you the samplerate your plug needs to use...
	dsc_create_note_table((int)plug->samplerate,440, plug->dat.freqtab);
	dsc_create_lfohz_table(plug->samplerate, 256, lfohz);
}

// ****************
//  Mouse handling
// ****************
void mouse_handling(plug_instance *plug){
        ikigui_get_events(&plug->dat.mywin);		// update window events
	struct mouse* m = &plug->dat.mywin.mouse ;	// Make a short hand name, for the code below

	if(m->left_click){ // Mouse down event
                plug->knob_selected = ikigui_mouse_pos(&plug->dat.knob_map, m->x -16, m->y-16);
                if(-1 != plug->knob_selected){ // if mouse pointer was over a tile
                        m->pressed = 1;
                        plug->hostcall(&plug->plughead, dawAutomateStart, plug->knob_selected, 0, 0, 0); // Tell host we grabed the knob 
                }
        }
        if(m->pressed){ // Change pressed knob according to relative mouse movement.
                float temp = plug->pth.knob[plug->knob_selected] + (float)(plug->dat.mywin.mouse.old_y - plug->dat.mywin.mouse.y) * 0.01; 
                if(0 > temp)            plug->pth.knob[plug->knob_selected] = 0; // knob can't go below 0.
                else if(1 < temp)       plug->pth.knob[plug->knob_selected] = 1; // knob can't go above 1.
                else                    plug->pth.knob[plug->knob_selected] = temp ; // the new knob value.
                
                plug->hostcall(&plug->plughead, dawAutomate,   plug->knob_selected, 0, 0, plug->pth.knob[plug->knob_selected]); // send new knob value to the DAW.
        }

        if(m->pressed && (m->left_release)){ // Release of mouse button
                m->pressed = 0;
                plug->hostcall(&plug->plughead, dawAutomateEnd,   plug->knob_selected, 0, 0, 0); // Tell the DAW that we released the knob.
        }
        for(int i = 0 ; i < NUMBER_OF_PARAMETERS ; i++ ){ // Update the tile map, with all knob values.
                plug->dat.knob_map.map[i] = (char)(plug->pth.knob[i] * plug->dat.knob_map.max_index ); // Select animation frame for knob value.
        }
	#ifdef DEBUG
	if(m->right_click && ikigui_mouse_hit(&hitbox, plug->dat.mywin.mouse.x, plug->dat.mywin.mouse.y)){ // For demo: click the amp label in the plugin editor
		sprintf(terminal(plug)," HIT ");
	}
	#endif
}
void draw_graphics(plug_instance *plug){ 	   			     // The DAW calls this when it wants to redraw the editor...
	ikigui_image_draw(&plug->dat.mywin.frame,&bg, 0, 0); 		     // Draw background.
	ikigui_map_draw(&plug->dat.knob_map,0,10,20);			     // Draw knobs.
	ikigui_map_draw(&plug->dat.font_map,0,PLUG_WIDTH-8*32,0);      // Draw text debugging text.
}
void prepare_graphics(plug_instance *plug,void *ptr){	// The DAW calls this when it wants to open the editor window...

	// Create a background image for the plug - using alpha compositing
	ikigui_image_empty(&bg, PLUG_WIDTH,PLUG_HEIGHT); // <- här vet att bilden inte är i fönstret, för koden som behandlar bilden i efterhand.
	ikigui_draw_gradient(&bg,0x00eeeedd, 0x00999999);
	ikigui_bmp_include(&labels,labels_array); // Load label graphics.
	ikigui_map_init(&plug->dat.label_map, &bg,&labels,0,H_DISTANCE,V_DISTANCE,64,14,PARAMETER_COL,PARAMETER_ROW);

	for(int i = 0 ; i < NUMBER_OF_PARAMETERS ; i++) plug->dat.label_map.map[i] = i; // automap the labels
	plug->dat.label_map.map[0] = 4; // PWM
	plug->dat.label_map.map[1] = 0; // ATTACK
	plug->dat.label_map.map[2] = 1; // DECAY
	plug->dat.label_map.map[3] = 2; // SUSTAIN
	plug->dat.label_map.map[4] = 3; // RELEASE

	plug->dat.label_map.map[5] = 10;// CUTOFF
	plug->dat.label_map.map[6] = 0; // ATTACK
	plug->dat.label_map.map[7] = 1; // DECAY
	plug->dat.label_map.map[8] = 2; // SUSTAIN
	plug->dat.label_map.map[9] = 3; // RELEASE

	ikigui_rect source = {.x = 0 , .y = 14*8, .h = 14 , .w = 64};
	ikigui_blit_alpha(&bg,&labels, 115+35, 8, &source); // AMP
	ikigui_blit_area(115+35, 8, &source,&hitbox);
	source.y = 14*9 ; 
	ikigui_blit_alpha(&bg,&labels, 115+35, 8+V_DISTANCE, &source); // FILTER

	plug->dat.label_map.renderer->bg_color= 0x00FFFFFF ; 
	ikigui_map_draw(&plug->dat.label_map,0,10,71);	// Draw text labels.

	// For the knob animation
	ikigui_bmp_include(&knob_anim,knob_array); // Load knob graphics.						
	ikigui_map_init(&plug->dat.knob_map, &plug->dat.mywin.frame,&knob_anim,0,H_DISTANCE,V_DISTANCE,64,64,PARAMETER_COL,PARAMETER_ROW);	// Set columns and rows of knobs in the tile array, and tile width and hight.

	#ifdef DEBUG
	// For debugging text
	ikigui_bmp_include(&font,font_array);
	ikigui_map_init(&plug->dat.font_map,&plug->dat.mywin.frame,&font,ASCII,0,0,8,8,32,PLUG_HEIGHT>>3); // 32 col, 8 rows, 8 width, 8 height.
	#endif
}
void destroy_graphics(plug_instance *plug,void *ptr){ // When the DAW closes the window...

}
void audioplugOpen(plugHeader *plugin){ // Is executed when the plug opens
	plug_instance *plug = (plug_instance*)plugin->object;

	dsc_create_saw(plug->dat.lfo_wave); // Used for PWM modulation
	plug->dat.lfo.wave = plug->dat.lfo_wave;
	for(int i = 0 ; i < 128 ; i++){
		plug->dat.voice[i].osc = plug->dat.osc ; // connect waveform
		plug->dat.voice[i].note = i;
	}	
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
			case 0x90: dsc_note_on(&plug->dat.voice[(midiData[1] & 0x7F)], (midiData[1] & 0x7F) , (midiData[2] & 0x7F),0 ); dsc_lfo_reset(&plug->dat.lfo);
				   dsc_note_on(&plug->dat.fenv [(midiData[1] & 0x7F)], 0 , (midiData[2] & 0x7F) ,1);
				#ifdef DEBUG		
				   sprintf(terminal(plug),"NOTE ON  %2d, OFFSET %5d ", midiData[1] & 0x7F, ev->MIDIMessages[i]->sample_offset); // Print message on bottom row // See DEBUG
				#endif
			break;  // Note on      
			case 0x80: dsc_note_off(&plug->dat.voice[(midiData[1] & 0x7F)]);
				   dsc_note_off(&plug->dat.fenv [(midiData[1] & 0x7F)]);
				#ifdef DEBUG						
				   sprintf(terminal(plug),"NOTE OFF %2d, OFFSET %5d ", midiData[1] & 0x7F, ev->MIDIMessages[i]->sample_offset); // Print message on bottom row // See DEBUG
				#endif
			break;  // Note off
			default:
				#ifdef DEBUG
				   sprintf(terminal(plug),"UNKNOWN,  OFFSET %5d ", ev->MIDIMessages[i]->sample_offset); // Print message on bottom row // See DEBUG
				#endif
			break;
		}
	}
	return 1;
}

//********************
//  Plugin algorithm
//********************
float dsc_filter(plug_instance *plug, int channel, int poles, float cutoff, float filt_in){
	float filt_out;
	for(int k = 0 ; k < poles ; k++){	plug->dat.filt_buff[channel][k+1] = ((filt_in - plug->dat.filt_buff[channel][k]) * cutoff) + plug->dat.filt_buff[channel][k]; } // LOWPASS FILTER
	plug->dat.filt_buff[channel][0] = filt_out = plug->dat.filt_buff[channel][poles]; // FILTER FEEDBACK
	return filt_out;
}
// The DAW calls this function to make the plugin process audio in buffer and fill with audio out data. Audio levels is between -1 to +1
void audio_in_out_float(plugHeader *plugin, float **inputs, float **outputs, int32_t sampleFrames){ plug_instance *plug = (plug_instance*)plugin->object;
	plug->dat.a_adsr.att = 255 * plug->pth.knob[1] ;
	plug->dat.a_adsr.dec = 255 * plug->pth.knob[2] ;
	plug->dat.a_adsr.sus = 255 * plug->pth.knob[3] ;
	plug->dat.a_adsr.rel = 255 * plug->pth.knob[4] ;

	plug->dat.f_adsr.att = 255 * plug->pth.knob[6] ;
	plug->dat.f_adsr.dec = 255 * plug->pth.knob[7] ;
	plug->dat.f_adsr.sus = 255 * plug->pth.knob[8] ;
	plug->dat.f_adsr.rel = 255 * plug->pth.knob[9] ;

	//float cutoff = (plug->pth.knob[4]*plug->pth.knob[4]*plug->pth.knob[4]*plug->pth.knob[4]);
	dsc_create_lfohz_table(plug->samplerate, 1, lfohz); // calculate tables for 1 sample buffer
	dsc_create_pwm(plug->dat.osc, (__u8)(255 * plug->pth.knob[0] ));
	for(int j = 0; j < sampleFrames; j++){ // Loop trough all the samples in buffer. Put your audio algorithm inside here....
		outputs[0][j] = 0.0f;
		outputs[1][j] = 0.0f;

		for(int k = 0 ; k < 128 ; k++ ){ // iterate all voices
			if(plug->dat.fenv[k].on){
				dsc_env_adsr(&plug->dat.fenv[k], &plug->dat.f_adsr); // Refresh volume for the voices.
			}
			if(plug->dat.voice[k].on){ // calculate only active voices
				dsc_env_adsr(&plug->dat.voice[k], &plug->dat.a_adsr); // Refresh volume for the voices.

				//float sample = (float)(dsc_wavetable_saw(&plug->dat.voice[k], plug->dat.freqtab[k])) * 0.000015259f ;
				//float sample = dsc_filter(plug,k,4,cutoff,(float)(dsc_wavetable_saw(&plug->dat.voice[k], plug->dat.freqtab[k])) * 0.000015259f) ;
				float cutoff = (float)(plug->dat.fenv[k].dca * 0.003921569f) + (plug->pth.knob[5] - 0.5f) ; // [5] is cutoff
				if(cutoff<0)cutoff=0; if(cutoff>1)cutoff=1;
				//float sample = dsc_filter(plug,k,4,cutoff*cutoff,(float)(dsc_wavetable_saw(&plug->dat.voice[k], plug->dat.freqtab[k])) * 0.000015259f) ;
				float sample = dsc_filter(plug,k,4,cutoff*cutoff,(float)(dsc_wavetable_voice(&plug->dat.voice[k], plug->dat.freqtab[k])) * 0.000015259f) ;
				outputs[0][j] += sample;
				outputs[1][j] += sample;
			}
		}
	}
}

#include "framework.c"