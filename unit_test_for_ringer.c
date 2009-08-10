// Unit test driver for the run() function of sb_ringer.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ladspa.h"

#define LIMIT_BETWEEN_5_AND_500(x) (((x) < 5) ? 5 : (((x) > 500) ? 500 : (x)))

void run_Ringer(LADSPA_Handle instance, unsigned long sample_count);


typedef struct
{
    LADSPA_Data * copy_count;
    LADSPA_Data * Input;
    LADSPA_Data * Output;
} Ringer;

char * filename;


int main(int argc, char * argv[])
{
    // exit if run without 3 arguments
    if (argc != 4)
    {
        printf("\nNeed 3 arguments: number of total samples, number of sample");
        printf(" copies, and filename for test results (in that order).\n");
        exit(-1);
    }

    // get the file name
    filename = malloc(sizeof (char) * strlen(argv[3]));
    if (!filename)
        exit(-1);
    strcpy(filename, argv[3]);

    const unsigned long BUFFER_SIZE = (unsigned long) atol(argv[1]);

    // create a pseudo Ringer instance
    Ringer * ringer = malloc(sizeof (ringer));
    if (!ringer)
    {
        free(filename);
        exit(-1);
    }

    // create a psuedo input buffer of audio samples.
    // the sample values are arbitrary, but they are sequential in order
    // to read the output easier.
    ringer->Input = malloc(sizeof (LADSPA_Data) * BUFFER_SIZE);
    if (!ringer->Input)
    {
        free(filename);
        free(ringer);
        exit(-1);
    }
    int i = 0;
    LADSPA_Data sample_val = 0.0f;
    for (i = 0; i < BUFFER_SIZE; ++i)
    {
        ringer->Input[i] = sample_val;
        sample_val += 1.0f;
    }

    // create the output buffer and initialize it to all zeroes
    ringer->Output = malloc(sizeof (LADSPA_Data) * BUFFER_SIZE);
    if (!ringer->Output)
    {
        free(filename);
        free(ringer->Input);
        free(ringer);
        exit(-1);
    }
    for (i = 0; i < BUFFER_SIZE; ++i)
        ringer->Output[i] = 0.0f;

    // set the copy count
    ringer->copy_count = malloc(sizeof (LADSPA_Data));
    if (!ringer->copy_count)
    {
        free(filename);
        free(ringer->Input);
        free(ringer->Output);
        free(ringer);
        exit(-1);
    }
    *(ringer->copy_count) = (LADSPA_Data) atof(argv[2]);

    run_Ringer(ringer, BUFFER_SIZE);

    free(ringer->Input);
    free(ringer->Output);
    free(ringer->copy_count);
    free(ringer);
    free(filename);

    return 0;
}


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

    // set the number of copies to be made
    const int SAMPLE_COPY_COUNT =
                           LIMIT_BETWEEN_5_AND_500((int) *(ringer->copy_count));

//----------------FILE INITIATION FOR TEST RESULTS-----------------------------
    FILE * write_file = NULL;
    write_file = fopen(filename, "w");
    if (!write_file)
    {
        printf("\n**Error: fail to create file 'test_results.txt'\n");
        return;
    }

    fprintf(write_file, "\nSample Count: %ld\n", sample_count);
//-----------------------------------------------------------------------------

    while (in_index < sample_count)
    {
        output[out_index++] = input[in_index];

//-----------------------------------------------------------------------------
        fprintf(write_file, "\n%f", input[in_index]);
//-----------------------------------------------------------------------------

        if (in_index >= sample_count - (SAMPLE_COPY_COUNT - 1)
            || (sample_count - (SAMPLE_COPY_COUNT - 1))
                >= 0xFFFFFFFFFFFFFFFF - (SAMPLE_COPY_COUNT - 1))
        {
            while (out_index < sample_count)

//-----------------------------------------------------------------------------
            {
                fprintf(write_file, "\n%f", input[in_index]);
//-----------------------------------------------------------------------------

                output[out_index++] = input[in_index];

//-----------------------------------------------------------------------------
            }
//-----------------------------------------------------------------------------

            break;
        }

        while (out_index < in_index + SAMPLE_COPY_COUNT)

//-----------------------------------------------------------------------------
        {
            fprintf(write_file, "\n%f", input[in_index]);
//-----------------------------------------------------------------------------

            output[out_index++] = input[in_index];

//-----------------------------------------------------------------------------
        }
//-----------------------------------------------------------------------------

        in_index += SAMPLE_COPY_COUNT;

        if (in_index >= sample_count)
            break;
    }

//-----------------------------------------------------------------------------
    fclose(write_file);
//-----------------------------------------------------------------------------
}
