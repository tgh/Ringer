/*
 * Copyright © 2009 Tyler Hayes
 * ALL RIGHTS RESERVED
 * [This program is licensed under the GPL version 3 or later.]
 * Please see the file COPYING in the source
 * distribution of this software for license terms.
 *
 * This LADSPA plugin takes a sample, makes a number of copies of it to the
 * output buffer, skips that number of samples in the input buffer, and
 * repeats the process.  This creates an effect similar to a ring modulator.
 * The number of copies to be made is controlled by the user.  The range
 * is 5 to 200 samples.
 *
 * Thanks to:
 * - Bart Massey of Portland State University (http://web.cecs.pdx.edu/~bart/)
 *   for suggesting LADSPA plugins as a project.
 */


//----------------
//-- INCLUSIONS --
//----------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ladspa.h"


//-----------------------
//-- DEFINED CONSTANTS --
//-----------------------
/*
 * These are the port numbers for the plugin
 */
// control port for the number of sample copies to be made
#define RINGER_COPY_COUNT 0
// input port
#define RINGER_INPUT 1
// output port
#define RINGER_OUTPUT 2

/*
 * Other constants
 */
// the plugin's unique ID given by Richard Furse (ladspa@muse.demon.co.uk)
#define UNIQUE_ID 4303
// number of ports involved
#define PORT_COUNT 3
// maximum number of samples to copy
#define MAX_COPIES 200
// minimum number of samples to copy
#define MIN_COPIES 5


//------------
//-- MACROS --
//------------
/*
 * This macro ensures the value used for the number of sample copies made in
 * the output buffer is between 5 and 200.
 */
#define LIMIT_BETWEEN_5_AND_200(x) (((x) < 5) ? 5 : (((x) > 200) ? 200 : (x)))


//-------------------------
//-- FUNCTION PROTOTYPES --
//-------------------------

// none


//--------------------------------
//-- STRUCT FOR PORT CONNECTION --
//--------------------------------


typedef struct
{
    // the number of copies to be placed into the output buffer.
    // NOTE: the number has to be an integer between 5 and 200, but this
    // variable is a pointer to a LADSPA_Data (a float), because the connection
    // to data_location in the connect_port() function cannot be made unless
    // they are of the same type.
    LADSPA_Data * copy_count;
    // data locations for the input & output audio ports
    LADSPA_Data * Input;
    LADSPA_Data * Output;
} Ringer;


//---------------
//-- FUNCTIONS --
//---------------


/*
 * Creates a plugin instance by allocating space for a plugin handle.
 * This function returns a LADSPA_Handle (which is a void * -- a pointer to
 * anything).
 */
LADSPA_Handle instantiate_Ringer()
{
    Ringer * ringer;

    // allocate space for an Ringer struct instance
    ringer = (Ringer *) malloc(sizeof (Ringer));

    // send the LADSPA_Handle to the host.  If malloc failed, NULL is returned.
    return ringer;
}

//-----------------------------------------------------------------------------


/*
 * Make a connection between a specified port and it's corresponding data
 * location. For example, the output port should be "connected" to the place in
 * memory where that sound data to be played is located.
 */
void connect_port_to_Ringer(LADSPA_Handle instance, unsigned long Port,
                            LADSPA_Data * data_location)
{
    Ringer * ringer;

    // cast the (void *) instance to (Ringer *) and set it to local pointer
    ringer = (Ringer *) instance;

    // direct the appropriate data pointer to the appropriate data location
    if (Port == RINGER_COPY_COUNT)
        ringer->copy_count = data_location;
    else if (Port == RINGER_INPUT)
        ringer->Input = data_location;
    else if (Port == RINGER_OUTPUT)
        ringer->Output = data_location;
}

//-----------------------------------------------------------------------------


/*
 * Here is where the rubber hits the road.  The actual sound manipulation
 * is done in run().
 */
void run_Ringer(LADSPA_Handle instance, unsigned long sample_count)
{
    Ringer * ringer = (Ringer *) instance;

    /*
     * NOTE: these special cases should never happen, but you never know--like
     * if someone is developing a host program and it has some bugs in it, it
     * might pass some bad data.
     */
    if (sample_count <= 1)
    {
        printf("\nEither 0 or 1 sample(s) were passed into the plugin.");
        printf("\nPlugin not executed.\n");
        return;
    }
    if (!ringer)
    {
        printf("\nPlugin received NULL pointer for plugin instance.");
        printf("\nPlugin not executed.\n");
        return;
    }

    LADSPA_Data * input; // to point to the input stream
    LADSPA_Data * output; // to point to the output stream

    // link the local pointers to their appropriate streams passed in through
    // instance
    input = ringer->Input;
    output = ringer->Output;

    // indexes for the input and output buffers
    unsigned long in_index = 0;
    unsigned long out_index = 0;

    // set the number of copies to be made using the defined macro
    const int SAMPLE_COPY_COUNT =
            LIMIT_BETWEEN_5_AND_200((int) *(ringer->copy_count));

    // go through the input buffer and make the necessary copies into the
    // output buffer
    while (in_index < sample_count)
    {
        // make the first copy (there will always be at least 2 samples due to
        // the first condition of this function)
        output[out_index++] = input[in_index];

        /*
         * Copy the current sample in the input buffer to the rest of the output
         * buffer if the input buffer index is anywhere between the set number
         * of copies being made until the end of the input buffer.
         *
         * NOTE: the ...0xFFFFFFFFFFFFFFFF... condition is for when the sample
         * count is less than the set number of copies being made.  If this were
         * not here, an underflow will occur since sample_count is an unsigned
         * integer, making it a huge positive number, which of course, in_index
         * will never be greater than or equal to.
         */
        if (in_index >= sample_count - (SAMPLE_COPY_COUNT - 1)
            || (sample_count - (SAMPLE_COPY_COUNT - 1))
               >= 0xFFFFFFFFFFFFFFFF - (SAMPLE_COPY_COUNT - 1))
        {
            while (out_index < sample_count)
                output[out_index++] = input[in_index];

            // get out of the loop since the output buffer is now full
            break;
        }

        // since in_index is not between the number of copies to the end of the
        // input buffer at this point, make the necessary copies to the output
        // buffer
        while (out_index < in_index + SAMPLE_COPY_COUNT)
            output[out_index++] = input[in_index];

        // jump ahead in the input buffer by the number of set copies to be made
        in_index += SAMPLE_COPY_COUNT;

        // get out of the loop if in_index is now beyond the end of the input
        // buffer
        if (in_index >= sample_count)
            break;
    }
}

//-----------------------------------------------------------------------------


/*
 * Frees dynamic memory associated with the plugin instance.  The host
 * better send the right pointer in or there's gonna be a leak!
 */
void cleanup_Ringer(LADSPA_Handle instance)
{
    if (instance)
        free(instance);
}

//-----------------------------------------------------------------------------

/*
 * Global LADSPA_Descriptor variable used in _init(), ladspa_descriptor(),
 * and _fini().
 */
LADSPA_Descriptor * Ringer_descriptor = NULL;


/*
 * The _init() function is called whenever this plugin is first loaded
 * by the host using it (when the host program is first opened).
 */
void _init()
{
    /*
     * allocate memory for Ringer_descriptor (it's just a pointer at this
     * point).
     * In other words create an actual LADSPA_Descriptor struct instance that
     * Ringer_descriptor will point to.
     */
    Ringer_descriptor = (LADSPA_Descriptor *)
            malloc(sizeof (LADSPA_Descriptor));

    // make sure malloc worked properly before initializing the struct fields
    if (Ringer_descriptor)
    {
        // assign the unique ID of the plugin given by Richard Furse
        Ringer_descriptor->UniqueID = UNIQUE_ID;

        /*
         * assign the label of the plugin. since there are no control features
         * for this plugin, "Ringer" is fine. (NOTE: it must not have white
         * spaces as per ladspa.h).
         * NOTE: in case you were wondering, strdup() from the string library
         * makes a duplicate string of the argument and returns the duplicate's
         * pointer (a char *).
         */
        Ringer_descriptor->Label = strdup("Ringer");

        /*
         * assign the special property of the plugin, which is any of the three
         * defined in ladspa.h: LADSPA_PROPERTY_REALTIME,
         * LADSPA_PROPERTY_INPLACE_BROKEN, and LADSPA_PROPERTY_HARD_RT_CAPABLE.
         * They are just ints (1, 2, and 4, respectively).  See ladspa.h for
         * what they actually mean.
         */
        Ringer_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;

        // assign the plugin name
        Ringer_descriptor->Name = strdup("Ringer");

        // assign the author of the plugin
        Ringer_descriptor->Maker = strdup("Tyler Hayes (tgh@pdx.edu)");

        /*
         * assign the copyright info of the plugin (NOTE: use "None" for no
         * copyright as per ladspa.h)
         */
        Ringer_descriptor->Copyright = strdup("GPL");

        // assign the number of ports for the plugin.
        Ringer_descriptor->PortCount = PORT_COUNT;

        /*
         * used for allocating and initailizing a LADSPA_PortDescriptor array
         * (which is an array of ints) since Adt_descriptor-> PortDescriptors
         * is a const *.
         */
        LADSPA_PortDescriptor * temp_descriptor_array;

        // allocate space for the temporary array with a length of the number
        // of ports (PortCount)
        temp_descriptor_array = (LADSPA_PortDescriptor *)
                calloc(PORT_COUNT, sizeof (LADSPA_PortDescriptor));

        /*
         * set the instance LADSPA_PortDescriptor array (PortDescriptors)
         * pointer to the location temp_descriptor_array is pointing at.
         */
        Ringer_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)
                temp_descriptor_array;

        /*
         * set the port properties by ORing specific bit masks defined in
         * ladspa.h.
         *
         * this one gives the control port that defines the number of sample
         * copies the properties that tell the host that this port takes input
         * (from the user) and is a control port (a port that is controlled by
         * the user).
         */
        temp_descriptor_array[RINGER_COPY_COUNT] = LADSPA_PORT_INPUT |
                LADSPA_PORT_CONTROL;

        /*
         * this one gives the input port the properties that tell the host that
         * this port takes input and is an audio port (not a control port).
         */
        temp_descriptor_array[RINGER_INPUT] = LADSPA_PORT_INPUT |
                LADSPA_PORT_AUDIO;

        /*
         * this gives the output port the properties that tell the host that
         * this port is an output port and that it is an audio port (I don't
         * see any situation where one might be an output port, but not an
         * audio port...).
         */
        temp_descriptor_array[RINGER_OUTPUT] = LADSPA_PORT_OUTPUT |
                LADSPA_PORT_AUDIO;

        /*
         * set temp_descriptor_array to NULL for housekeeping--we don't need
         * that local variable anymore.
         */
        temp_descriptor_array = NULL;

        /*
         * temporary local variable (which is a pointer to an array of arrays
         * of characters) for the names of the ports since Adt_descriptor->
         * PortNames is a const char * const *.
         */
        char ** temp_port_names;

        // allocate the space for two port names
        temp_port_names = (char **) calloc(PORT_COUNT, sizeof (char *));

        /*
         * set the instance PortNames array pointer to the location
         * temp_port_names is pointing at.
         */
        Ringer_descriptor->PortNames = (const char **) temp_port_names;

        // set the name of the control port for the number of sample copies to
        // be made
        temp_port_names[RINGER_COPY_COUNT] = strdup("Copies (samples)");

        // set the name of the input port
        temp_port_names[RINGER_INPUT] = strdup("Input");

        // set the name of the ouput port
        temp_port_names[RINGER_OUTPUT] = strdup("Output");

        // reset temp variable to NULL for housekeeping
        temp_port_names = NULL;

        /*
         * temporary local variable (pointerto a PortRangeHint struct) since
         * Adt_descriptor->PortRangeHints is a const *.
         */
        LADSPA_PortRangeHint * temp_hints;

        // allocate space for two port hints (see ladspa.h for info on 'hints')
        temp_hints = (LADSPA_PortRangeHint *)
                calloc(PORT_COUNT, sizeof (LADSPA_PortRangeHint));

        /*
         * set the instance PortRangeHints pointer to the location temp_hints
         * is pointed at.
         */
        Ringer_descriptor->PortRangeHints = (const LADSPA_PortRangeHint *)
                temp_hints;

        /*
         * set the port hint descriptors (which are ints).
         * For the control port, the BOUNDED masks from
         * ladspa.h tell the host that this control has limits (this one is 5
         * and 200 as defined in the Macro at the top).
         * The DEFAULT_LOW mask tells the host to set the control value upon
         * start (like for a gui) to a low value between the bounds.
         * The INTEGER mask tells the host that the control values should be in
         * integers.
         */
        temp_hints[RINGER_COPY_COUNT].HintDescriptor =
                (LADSPA_HINT_BOUNDED_BELOW
                 | LADSPA_HINT_BOUNDED_ABOVE
                 | LADSPA_HINT_DEFAULT_LOW
                 | LADSPA_HINT_INTEGER);
        // set the lower bound of the control
        temp_hints[RINGER_COPY_COUNT].UpperBound = (LADSPA_Data) MAX_COPIES;
        // set the upper bound of the control
        temp_hints[RINGER_COPY_COUNT].LowerBound = (LADSPA_Data) MIN_COPIES;
        // input and ouput don't need any range hints
        temp_hints[RINGER_INPUT].HintDescriptor = 0;
        temp_hints[RINGER_OUTPUT].HintDescriptor = 0;

        // reset temp variable to NULL for housekeeping
        temp_hints = NULL;

        // set the instance's function pointers to appropriate functions
        Ringer_descriptor->instantiate = instantiate_Ringer;
        Ringer_descriptor->connect_port = connect_port_to_Ringer;
        Ringer_descriptor->activate = NULL;
        Ringer_descriptor->run = run_Ringer;
        Ringer_descriptor->run_adding = NULL;
        Ringer_descriptor->set_run_adding_gain = NULL;
        Ringer_descriptor->deactivate = NULL;
        Ringer_descriptor->cleanup = cleanup_Ringer;
    }
}

//-----------------------------------------------------------------------------


/*
 * Returns a descriptor of this plugin.
 *
 * NOTE: this function MUST be called 'ladspa_descriptor' or else the plugin
 * will not be recognized.
 */
const LADSPA_Descriptor * ladspa_descriptor(unsigned long index)
{
    if (index == 0)
        return Ringer_descriptor;
    else
        return NULL;
}

//-----------------------------------------------------------------------------


/*
 * This is called automatically when the host quits (when this dynamic library
 * is unloaded).  It frees all dynamically allocated memory associated with
 * the descriptor.
 */
void _fini()
{
    if (Ringer_descriptor)
    {
        free((char *) Ringer_descriptor->Label);
        free((char *) Ringer_descriptor->Name);
        free((char *) Ringer_descriptor->Maker);
        free((char *) Ringer_descriptor->Copyright);
        free((LADSPA_PortDescriptor *) Ringer_descriptor->PortDescriptors);

        int i = 0;
        for (i = 0; i < Ringer_descriptor->PortCount; ++i)
            free((char *) (Ringer_descriptor->PortNames[i]));

        free((char **) Ringer_descriptor->PortNames);
        free((LADSPA_PortRangeHint *) Ringer_descriptor->PortRangeHints);

        free(Ringer_descriptor);
    }
}

// ------------------------------- EOF ----------------------------------------
