/*******************************************************************************
 *
 * Drone control through voice recognition -- PC to drone communication
 * Team GYTAM, feb. 2016
 *
 *
 ******************************************************************************/
 
 /** Functions use to convert a WAVE file to mfcc coefficient
 *
 * @file
 */
 
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for memcmp
#include <stdint.h> // for int16_t and int32_t
#include <math.h>
#include <iostream>
#include "libmfccOptim.h"
#include "FFTReal.h"
#include "WavToMfcc.h"

/**
* Safety function to ensure the system use big endian
*
* @param none
* @return true or false
*/
int is_big_endian(void) {
	union {
		uint32_t i;
		char c[4];
	} bint = { 0x01000000 };
	return bint.c[0] == 1;
}

/**
* Read a wave file.
*
* @param p_wav (OUT) pointer to a file descriptor
* @param filename (IN) pointer to the name of the file
* @param p_header (OUT) pointer to a wave header structure
* @return none
*/
void wavRead(FILE **p_wav, char *filename, wavfile *p_header) {
	//fopen_s(p_wav, filename, "rb"); //Windows version
	*p_wav = fopen(filename, "rb");
	if (*p_wav == NULL) {
		fprintf(stderr, "Can't open input file %s\n", filename);
		exit(1);
	}

	// read header
	if (fread(p_header, sizeof(wavfile), 1, *p_wav) < 1) {
		fprintf(stderr, "Can't read input file header %s\n", filename);
		exit(1);
	}

	// if wav file isn't the same endianness than the current environment
	// we quit
	if (is_big_endian()) {
		if (memcmp((*p_header).id, "RIFX", 4) != 0) {
			fprintf(stderr, "ERROR: %s is not a big endian wav file\n", filename);
			exit(1);
		}
	}
	else {
		if (memcmp((*p_header).id, "RIFF", 4) != 0) {
			fprintf(stderr, "ERROR: %s is not a little endian wav file\n", filename);
			exit(1);
		}
	}

	if (memcmp((*p_header).wavefmt, "WAVEfmt ", 8) != 0
		|| memcmp((*p_header).data, "data", 4) != 0
		) {
		fprintf(stderr, "ERROR: Not wav format\n");
		exit(1);
	}
	if ((*p_header).format != 16) {
		fprintf(stderr, "\nERROR: not 16 bit wav format.");
		exit(1);
	}
	if (memcmp((*p_header).data, "data", 4) != 0) {
		fprintf(stderr, "ERROR: Prrroblem?\n");
		exit(1);
	}
}

/**
* Transform the wav extension into mfc extension.
*
* @param filename (IN) pointer to the name of the file
* @param mfcName OUT) pointer to the new name of the file with .mfc ext
* @return none
*/
void nameWavToMfc(char *filename, char * mfcName) {
	int i = 0;
	// Copy the name
	while (filename[i] != '.') {
		mfcName[i] = filename[i];
		i++;
	}
	// Change the extension
	mfcName[i++] = '.';
	mfcName[i++] = 'm';
	mfcName[i++] = 'f';
	mfcName[i++] = 'c';
	mfcName[i] = '\0';
}

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
void removeSilence(int16_t * x, int Nx, int16_t ** xFiltered, int * newLength, float threshold) {
	float xMin = INFINITY;
	float xMax = 0;
	float * x_abs = new float[Nx];
	for (int l = 0; l < Nx; l++) {
		x_abs[l] = (float)((x[l] < 0) ? -x[l] : x[l]);
		xMin = ((x_abs[l] < xMin) ? x_abs[l] : xMin);
		xMax = ((x_abs[l] > xMax) ? x_abs[l] : xMax);
	}
	xMax -= xMin;
	float *e = new float[Nx];
	float tmp;
	for (int l = 0; l < Nx; l++) {
		tmp = (x_abs[l] - xMin) / xMax;
		e[l] = tmp*tmp;
	}
	int firstVal = 0;
	while (firstVal < Nx && e[firstVal++] < threshold) {}
	firstVal--;
	int lastVal = Nx - 1;
	while (lastVal > 0 && e[lastVal--] < threshold) {}
	lastVal++;
	delete[] e, x_abs;
	//printf("%d : %d %d %d\n", lastVal - firstVal + 1, Nx, lastVal, firstVal);
	*xFiltered = new int16_t[lastVal - firstVal + 1];
	//printf("%d : %d %d %d\n", lastVal - firstVal + 1, Nx, lastVal, firstVal);
	int size = Nx - (lastVal - firstVal);
	if (size > 2048) {
		size = 2048;
	}
	for (int l = 0; l < Nx; l++) {
		if (l >= firstVal && l <= lastVal) {
			(*xFiltered)[l - firstVal] = x[l];
		}
	}
	*newLength = lastVal - firstVal + 1;
}

/**
* Compute MFCC of a signal
*
* @param X_mfcc (OUT) Adress of a buffer to store MFCC coefficient
* @param length_xmfcc (OUT) Pointer to the length of the buffer
* @param x (IN) The signal to process
* @param Nx The (IN) length of the signal
* @param header (IN) wave header structure
* @param frame_length (IN) Frame length
* @param frame_step (IN) Step between each frame to allow overlapping. 160frame step = 10ms frames
* @param dim_mfcc (IN) Number of MFCC coefficient kept
* @param num_filter (IN) Number of filter for the mfcc algorithm
* @return none
*/
void computeMFCC(float **X_mfcc, int *length_xmfcc, int16_t *x, int Nx, int frequency, int frame_length, int frame_step, int dim_mfcc, int num_filter) {
	int power2of_sample = (int)powf(2.0, ceilf(logf((float)frame_length) / logf(2.0f))); // Smallest power of 2 > to frame_length
	int n_frames;
	
	if (Nx > power2of_sample) {
		//n_frames = (int)ceil((float)fmax((Nx - power2of_sample), 0) / (float)frame_step); // Number of frames - 1 (Because array starts at 0)
		n_frames = (Nx - power2of_sample) / frame_step + 1; // +1 for the upper int
		if (((Nx - power2of_sample - 1) / frame_step + 1) < n_frames) {
			// In this case Nx-power2of_sample is a multiple of frame_step -> +1 was wrong
			n_frames--;
		}
	}
	else {
		n_frames = 0;
	}
	
	
	int nb_fft_kept = power2of_sample;	// Number of MFCC coefficient kept
	float *han_window = new float[power2of_sample];

	//printf("size : %d\n", dim_mfcc*(n_frames + 1)); 

	// Memory allocation
	//FFTReal::flt_t	* const	X = new FFTReal::flt_t[power2of_sample];
	//FFTReal::flt_t * const fX_abs = new FFTReal::flt_t[nb_fft_kept];
	*X_mfcc = new float[dim_mfcc*(n_frames + 1)];
	*length_xmfcc = n_frames + 1;

	float	img;
	int k, i, m;
	float xnorm = 0;// Norm of the signal
	float xmean = 0;
	
	for (k = 0; k < Nx; k++) {
		xmean += x[k];
	}
	xmean = xmean / Nx;

	for (k = 0; k < Nx; k++) {
		x[k] -= (int16_t)xmean;
		xnorm += x[k]*x[k];
	}
	xnorm = sqrt(xnorm / Nx);
	

	// Build han window
	float pi_pi_on_power2of_sample = 9.86960440109f / power2of_sample;
	for (k = 0; k < power2of_sample; k++) {
		//han_window[k] = 1;
		han_window[k] = (1 - cos(k * pi_pi_on_power2of_sample)) / 2;
		//han_window[k] = 0.54 - 0.46*cos(k*pi_pi_on_power2of_sample);
		//han_window[k] = 0.42 - 0.5*cos(k*pi_pi_on_power2of_sample) + 0.08*cos(2 * k*pi_pi_on_power2of_sample);
	}
	
	FFTReal fft(power2of_sample);
	FFTReal::flt_t	* X = new FFTReal::flt_t[power2of_sample];
	FFTReal::flt_t * fX_abs = new FFTReal::flt_t[nb_fft_kept];
	FFTReal::flt_t	* fX = new FFTReal::flt_t[power2of_sample];
	
	//#pragma omp parallel for private(k,m,i,img) shared(han_window)
	for (k = 0; k <= n_frames; k++) {

		//printf("%d\n", k);
		//FFTReal::flt_t	* X = new FFTReal::flt_t[power2of_sample];
		//FFTReal::flt_t * fX_abs = new FFTReal::flt_t[nb_fft_kept];
		//FFTReal::flt_t	* fX = new FFTReal::flt_t[power2of_sample];

		// Store the corresponding frame from the signal normalised into X
		if (k < n_frames) {
			for (i = 0; i < power2of_sample; i++)
			{
				X[i] = han_window[i] * ((FFTReal::flt_t)x[k*frame_step + i] / xnorm);
			}
		}
		else {
			//printf("%d -- %d\n", frame_step, n_frames);
			for (i = frame_step*n_frames; i < Nx; i++)
			{
				// Last values of the signal
				X[i - frame_step*n_frames] = han_window[i - frame_step*n_frames] * ((FFTReal::flt_t)x[i] / xnorm);
			}
			for (i = Nx; i < frame_step*n_frames + power2of_sample; i++)
			{
				// The frame is padded with 0
				X[i - frame_step*n_frames] = (FFTReal::flt_t)0.0;
			}
		}
		

		// Compute FFT
		//#pragma omp critical 
		//{
		fft.do_fft(fX, X);
		//}
		//fft.rescale(X);

		// Compute norm of FFT
		int nb_zero_padded = (frame_step*k + power2of_sample - Nx);// Used to weight the last window according to the number of zero padded
		if (nb_zero_padded < 1) {
			nb_zero_padded = 1;
		}
		for (i = 0; i < nb_fft_kept; i++) {
			if (i > 0 && i < power2of_sample / 2) {
				img = fX[i + power2of_sample / 2];
				fX_abs[i] = (fX[i] * fX[i] + img * img) / nb_zero_padded;
			}
			else if (i == 0 || i == power2of_sample / 2) {
				fX_abs[i] = (fX[i] * fX[i]) / nb_zero_padded;
			}
			else {
				img = fX[3 * power2of_sample / 2 - i];
				fX_abs[i] = (fX[power2of_sample - i] * fX[power2of_sample - i] + img * img) / nb_zero_padded;
			}
		}
		
		
		// Compute MFCC coefficient		
		float * coefs = GetCoefficient(fX_abs, frequency, num_filter, nb_fft_kept, dim_mfcc);
		for (m = 0; m < dim_mfcc; m++) {
			(*X_mfcc)[k*dim_mfcc + m] = coefs[m];
		}		
		//delete[] fX_abs, fX, X;
	}
	delete[] fX_abs, fX, X;
}

/*** Deleted stuff .. 
 SMOOTHING :
 int16_t *x_abs = new int16_t[xFLength];
 int demi_span = 2;
 for (int k = 0; k < xFLength; k++) {
 if (k <= demi_span) {
 for (int i = 0; i <= 2 * k; i++) {
 x_abs[k] += xFiltered[i];
 }
 x_abs[k] = x_abs[k] / (2 * k + 1);
 }
 else {
 for (int i = k - demi_span; i <= k + demi_span; i++) {
 x_abs[k] += xFiltered[i];
 }
 x_abs[k] = x_abs[k] / (2 * demi_span + 1);
 }
 }
 for (int k = 0; k < xFLength; k++) {
 xFiltered[k] = x_abs[k];
 }
**/
