#include "speaker.h"
#include "vad/wb_vad.h"
#include "mfcc.h"

extern void fft( double data[],int nn,int isign );

void mfcc_feature(short int Data[size],int person_id,int num, int mode, int *frameAmount,double (*MFCC)[12])
{
    VadVars  st;
    int vad = 0;
    int speech_frame = 0;
    int i,j,k = 0;
    char FilePath[60];
    FILE *fp = NULL;

    for( i=0; i<*frameAmount; i++ )
        for( j=0; j<FrameSize; j++ )
            postProcData[i][j<<1]=Data[i*FrameStep+j]*hamming[j];

        	//fft matrix
    for( i=0; i<*frameAmount; i++ )
        fft(postProcData[i],MFCC_fftLength,1);

            //spectraP matrix, frameAmount*MFCC_fftLength/2+1
    for( i=0; i<*frameAmount; i++ )
        for( j=0; j<MFCC_fftLength/2+1; j++ )
            spectraP[i][j]=postProcData[i][j<<1]*postProcData[i][j<<1]+postProcData[i][(j<<1)+1]*postProcData[i][(j<<1)+1];

    for( i=0; i<*frameAmount; i++ )
        for( j=0; j<FrameSize*2; j++ )
            postProcData[i][j]=0;
        	//melSpectraP matrix,MelChnNum*frameAmount
    for( i=0; i<MelChnNum; i++ )
        for( k=0; k<*frameAmount; k++ )
        {
            for( j=0; j<MFCC_fftLength/2+1; j++ )
            {
                if( j==0 )
                    melSpectraP[i][k]=bank[i][j]*spectraP[k][j];
                else
                    melSpectraP[i][k]+=bank[i][j]*spectraP[k][j];
            }
            melSpectraP[i][k]=log(melSpectraP[i][k]);
        }

          	//mfcc matrix, frameAmount*MFCCDim
    for( k=0; k<*frameAmount; k++ )
        for( i=0; i<12; i++ )
            for( j=0; j<MelChnNum; j++ )
            {
                if( j==0 )
                    MFCC[k][i]=dctcoef[i][j]*melSpectraP[j][k];
                else
                    MFCC[k][i]+=dctcoef[i][j]*melSpectraP[j][k];
            }

        	//Ceplift
    for( i=0; i<*frameAmount; i++ )
        for( j=0; j< 12; j++ )
            MFCC[i][j]*=cepLifter[j];
/*
    for(j = 0; j < 12; j++)		//计算一阶差分
        for(i = 0; i< frameAmount; i++)
        {
            if(i == frameAmount - 1) 	dmel_cep[i][j]=mel_cep[i][j]-mel_cep[0][j];
            else		dmel_cep[i][j]=mel_cep[i][j]-mel_cep[i+1][j];
        }

    for(j = 0; j < 12; j++)		//计算二阶差分
        for(i = 0; i< frameAmount; i++)
        {
            if(i == frameAmount - 1) 	ddmel_cep[i][j]=dmel_cep[i][j]-dmel_cep[0][j];
            else		ddmel_cep[i][j]=dmel_cep[i][j]-dmel_cep[i+1][j];
        }

    for(i = 0; i < frameAmount; i++)			//MFCC,一阶,二阶赋值给数组MFCC
        for(j = 0; j < 12; j++)
        {
            MFCC[i][j]=mel_cep[i][j];
            MFCC[i][j+12]=dmel_cep[i][j];
            MFCC[i][j+2*12]=ddmel_cep[i][j];
        }
    */
    if(mode == 1)
        sprintf(FilePath,"train/F0%d_%d.mfcc",person_id,num);
        //sprintf(FilePath,"train/0%d_train.wav",m);
    if(mode == 0)
        sprintf(FilePath,"recog/F0%d_%d.mfcc",person_id,num);

    fp= fopen(FilePath,"wb");

    for( i=0; i<*frameAmount; i++ )
    {
        for( j=0; j<MFCCDim; j++ )
            fprintf(fp,"%lf ",MFCC[i][j]);
        fprintf(fp,"\n");
    }
    fclose(fp);



}


