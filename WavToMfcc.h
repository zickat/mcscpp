/*******************************************************************************
 *
 * Drone control through voice recognition -- PC to drone communication
 * Team GYTAM, feb. 2016
 *
 * @author Tom Lucas
 *
 ******************************************************************************/
#pragma once
#include <stdint.h> // for int16_t and int32_t
#include <iostream>
 
 /** Functions use to convert a WAVE file to mfcc coefficient
 *
 * @file
 */
 
 /**
 * Structure for the header of a Wave file
 *
 **/
struct wavfile
{
	char        id[4];          // should always contain "RIFF"
	int     totallength;    // total file length minus 8
	char        wavefmt[8];     // should be "WAVEfmt "
	int     format;         // 16 for PCM format
	short     pcm;            // 1 for PCM format
	short     channels;       // channels
	int     frequency;      // sampling frequency
	int     bytes_per_second; // Number of bytes per seconde
	short     bytes_by_capture; // Number of bytes by capture
	short     bits_per_sample; // Number of bits per sample
	char        data[4];        // should always contain "data"
	int     bytes_in_data; // Data of the file
};

/**
* Safety function to ensure the system use big endian
*
* @param none
* @return true or false
*/
int is_big_endian(void);

/**
* Read a wave file.
*
* @param p_wav (OUT) pointer to a file descriptor
* @param filename (IN) pointer to the name of the file
* @param p_header (OUT) pointer to a wave header structure
* @return none
*/
void wavRead(FILE **p_wav, char *filename, wavfile *p_header);

/**
* Transform the wav extension into mfc extension.
*
* @param filename (IN) pointer to the name of the file
* @param mfcName OUT) pointer to the new name of the file with .mfc ext
* @return none
*/
void nameWavToMfc(char *filename, char * mfcName);

/**
* Remove silence of the signal at the start and the end
*
* @param x (IN) the signal
* @param Nx (IN) the length of the signal
* @param xFiltered (OUT) a buffer for the signal without silence
* @param newLength (OUT) the length of the signal without silence
* @param threshold (IN) for the sensibility (silence detection)
* @return The signal without silence
*/
void removeSilence(int16_t * x, int Nx, int16_t ** xFiltered, int * newLength, float threshold);
/**
* Compute MFCC of a signal
*
* @param OUT float **X_mfcc Adress of a buffer to store MFCC coefficient
* @param OUT int *length_xmfcc  Pointer to the length of the buffer
* @param IN int16_t *x The signal to process
* @param IN int Nx The length of the signal
* @param IN wavfile header wave header structure
* @param IN int frame_length Frame length
* @param IN int frame_step Step between each frame to allow overlapping. 160frame step = 10ms frames
* @param IN int dim_mfcc Number of MFCC coefficient kept
* @param IN int num_filter Number of filter for the mfcc algorithm
* @return none
*/
void computeMFCC(float **X_mfcc, int *length_xmfcc, int16_t *x, int Nx, int frequency, int sample_length, int sample_step, int dim_mfcc, int num_filter);


