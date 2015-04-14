#include <math.h>
#include <stdio.h>

#define FrameSize 256
#define FrameShift 128
#define FrameStep 128//步长FrameSize-FrameShift
#define MFCCDim 12
#define MelChnNum 24
#define MFCC_fftLength 256
#define Fs 8000
#define Fl 0
#define Fh 0.5
#define N 61*2  //分帧后的帧数 1874(30S)  624(10S) 438(7s)  311(5s)   186(3s)  61(61s)

#define MixDim 16
#define GMM_Threshold 0.001
#define MinCov 0.01
#define size (N+1)*FrameShift  //240000(30S)  80000(10S) 56000(7s) 40000(5s)  24000(3s)  8000(1s)

int frameAmount;
double mixcoef[MixDim];
double mean[MFCCDim][MixDim];
double covmatrix[MixDim][MFCCDim];
