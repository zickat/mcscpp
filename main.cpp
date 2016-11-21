//#include <jni.h>
#include <string>
#include "dtw.h"
#include "WavToMfcc.h"
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include <limits>

#define nbMots 13
#define nbLocuteurs 17

std::string vocabulaire[] = {"arretetoi", "atterrissage", "avance", "decollage", "droite", "etatdurgence", "faisunflip", "gauche", "plusbas", "plushaut", "recule", "tournedroite", "tournegauche"};
std::string locuteurs[] = {"M01", "M02","M03","M04", "M05","M06","M07","M08", "M09", "M10", "M11", "M12", "M13", "F02","F03","F04", "F05"};


using namespace std;


char * fichier(std::string ordre, std::string locuteur){
    std::stringstream ss;
    ss << "corpus/dronevolant_nonbruite/" << locuteur << "_" << ordre << ".wav";
    return strdup(ss.str().c_str());
}

void getMyMFCC(char * filename, float *** buffer, int * size){
    struct wavfile mywav;
    FILE * f;

    *buffer = new float*; 

    wavRead(&f, filename, &mywav);

    int16_t ** bufferSilence = new int16_t*;
    int newLength;
    int16_t * wavbuffer;

    wavbuffer = new int16_t[mywav.bytes_in_data];
    int16_t b;
    
    int i = 0;

	while(fread(&b, sizeof(int16_t), 1, f) > 0){
		wavbuffer[i++] = b;
	}

   //int n = fread(wavbuffer, sizeof(int16_t), mywav.bytes_in_data/sizeof(int16_t), f);

    //removeSilence(wavbuffer, mywav.bytes_in_data/sizeof(int16_t), bufferSilence, &newLength, 10);
    //delete(wavbuffer);

    //computeMFCC(*buffer, size, * bufferSilence, newLength, mywav.frequency, 512, 256, 13, 26);
    computeMFCC(*buffer, size, wavbuffer, mywav.bytes_in_data/sizeof(int16_t), mywav.frequency, 512, 256, 13, 26);
    delete[] wavbuffer, f;
}

int findWord(char * wavfile){
    int MatriceConfusion[nbMots] = {0};
    float inf = std::numeric_limits<float>::infinity();
    float ** buffWav;
    int sizeWav;
    getMyMFCC(wavfile, &buffWav, &sizeWav);
    printf("Running ...\n");
    for (int i = 0; i < nbLocuteurs; ++i)
    {
        float min = inf;
        int indiceMin = 0; 
        for (int j = 0; j < nbMots; ++j)
        {
            float ** buffLoc;
            int sizeLoc;
            getMyMFCC(fichier(vocabulaire[j], locuteurs[i]), &buffLoc, &sizeLoc);
            float cout = dtw(sizeLoc, sizeWav, 13, *buffLoc, *buffWav);
            if(cout < min){
                min = cout;
                indiceMin = j;
            }
            delete(*buffLoc);
            delete(buffLoc);
        }
        MatriceConfusion[indiceMin] ++;
    }
    int max = 0;
    int indiceMax = 0;
    for (int i = 0; i < nbMots; ++i)
    {
        if(MatriceConfusion[i] > max){
            max = MatriceConfusion[i];
            indiceMax = i;
        }
        printf("%d\n", MatriceConfusion[i]);
    }
    return indiceMax;
}


int main(int argc, char const *argv[]){
    if(argc < 2){
        fprintf(stderr, "Usage : %s fichier_wav\n", argv[0]);
        exit(1);
    }

    char * filename = strdup(argv[1]);

    int indMot = findWord(filename);
    printf("L'ordre reconnu est : %s\n", vocabulaire[indMot].c_str());
    return 0;
}

