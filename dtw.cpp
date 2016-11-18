/*******************************************************************************
 *
 * Drone control through voice recognition -- PC to drone communication
 * Team GYTAM, feb. 2016
 *
 *
 ******************************************************************************/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for memcmp
#include <stdint.h> // for int16_t and int32_t
#include <math.h>
#include <iostream>
#include "dtw.h"
#include <limits>
#include <cfloat>

float min(float * tabTest, int nb);

float distanceMatricielle(float * matrice1, int size1, float * matrice2, int size2, int x, int j, int dim);

int coord(int i, int j, int nbL);

int size(int indice, int dim_mfcc, int fin);

/**
* Dtw function that given two matrix of cep coefficient computes distance
* between those two signals.
*  @param n_ck      Dimension of unknow signal
*  @param n_cunk    Dimension of know signal
*  @param dim_mfcc  Size of nfcc decompostion base
*  @param c_k       Matrix of know signal
*  @param c_unk     Matrix of unknow signal
*  @return Distance between the two signals
*/

float dtw(int n_ck, int n_cunk, int dim_mfcc, float* c_k, float* c_unk) {

    float ** g;
    float distance;
    float inf = std::numeric_limits<float>::infinity();
    float testmin[3];
    float indMin;

    //printf("%d - %d\n", n_ck, n_cunk);

    //g = (float**)malloc((n_ck+1)* sizeof(float *));
    g = new float * [dim_mfcc + 1];
    for (int i = 0; i < dim_mfcc+1; ++i) {
        //g[i] = (float*)malloc((n_cunk+1)* sizeof(float));
        g[i] = new float[dim_mfcc + 1];
    }

    g[0][0] = 0;
    for (int i = 1; i < dim_mfcc+1; ++i) {
        g[0][i] = inf;
	g[i][0] = inf;
    }

    for (int i = 1; i < dim_mfcc+1; i ++) {
        //printf("0\n");
        //g[i][0] = inf;
        for (int j = 1; j < dim_mfcc + 1; j ++) {
            //distance = distanceMatricielle(c_k, size(i, n_ck/dim_mfcc, n_ck), c_unk, size(i, n_cunk/dim_mfcc, n_cunk), i, j, dim_mfcc);
            distance = distanceMatricielle(c_k, n_ck, c_unk, n_cunk, i, j, dim_mfcc);
            //printf("%f\n", distance);
	    // if(i == j) distance = 0; else distance = 10;
            testmin[0] = g[i-1][j] + distance;
            testmin[1] = g[i-1][j-1] + distance;
            testmin[2] = g[i][j-1] + distance;
            g[i][j] = min(testmin, 3);
	    //g[i][j] = distance;
	   //printf("%f  ", g[i][j]);
        }
        //printf("\n");
    }
    //printf("%d - %d\n", n_ck, n_cunk);
    //printf("fin\n");
    //printf("%f\n", g[dim_mfcc][dim_mfcc]/(dim_mfcc + dim_mfcc));
    /*for(int i=0; i < dim_mfcc + 1; i++){
    	for(int j=0; j < dim_mfcc + 1; j++){
    	     printf("%2.2f   ", g[i][j]);
    	}
    	printf("\n");
    }
*/    return g[dim_mfcc][dim_mfcc]/(dim_mfcc + dim_mfcc);

}

float min(float * tabTest, int nb){
    float valMin = std::numeric_limits<float>::infinity() - 1;
    //printf("inf = %f\n", valMin);
    for (int i = 0; i < nb; ++i) {
        //printf("%f ? %f\n", tabTest[i], valMin);
        if(tabTest[i] < valMin){
            valMin = tabTest[i];
        }
    }
    //printf("min = %f\n", valMin);
    return valMin;
}

float distanceMatricielle(float * matrice1, int size1, float * matrice2, int size2, int x, int j, int dim){
    float summ = 0;
    int i;
    //printf("%d %d\n", size1, size2);
    for (i = 0; i < size1-1 && i < size2-1; ++i) {
        summ += (matrice1[coord(x,i,dim)] - matrice2[coord(j,i,dim)])*(matrice1[coord(x,i,dim)] - matrice2[coord(j,i,dim)]);
    }
    for(; i < size1-1; ++i){
        int c = coord(x,i,dim);
        summ += matrice1[c]*matrice1[c];
    }
    for(; i < size2-1; ++i){
        int c = coord(j,i,dim);
        summ += matrice2[c]*matrice2[c];
    }
    return (float)sqrt(summ);
}

int coord(int i, int j, int nbL){
    return i + j*nbL;
}

int size(int indice, int dim_mfcc, int fin){
    if(indice + dim_mfcc > fin){
        return dim_mfcc + indice - fin;
    }
    return dim_mfcc;
}
