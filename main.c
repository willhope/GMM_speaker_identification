#include "speaker.h"
#include <time.h>

extern void mfcc_feature(short int Data[size],int person_id,int num, int mode, int *frameAmount,double (*MFCC)[12]);
extern void GmmTrainning(double mfcc[N][MFCCDim], int frameAmount, int person_id );
extern double CalGmmProb( double mfcc[N][MFCCDim], int frameAmount );

short int voiceData[size];
double mel_cep[N][12];

int frameAmount = 0;



void read_voice(short int voiceData[size],int person_id,int num,int mode,int *frameAmount)
{
    FILE *fp = NULL;
    int i = 0;
    int frame_num = 0;
    double maxData, minData;
    int voiceDataCnt;
    short int shortData[FrameShift];
    char FilePath[60];


    if(mode == 1)
        sprintf(FilePath,"train/F0%d_%d.WAV",person_id,num);
                //sprintf(FilePath,"train/0%d_train.wav",m);
    if(mode == 0)
        sprintf(FilePath,"recog/F0%d_%d.WAV",person_id,num);
                //sprintf(FilePath,"recog/0%d_test.wav",m);
    printf("%s \n",FilePath);
    fp=fopen(FilePath,"rb");
    if( fp==NULL )
        printf("can not open the WAV file\n");


    voiceDataCnt=0;
    maxData=0;
    minData=0;



    while( fread(shortData,sizeof(short int),FrameShift,fp)==FrameShift && frame_num<(N+1) )
    {

        for( i=0; i<FrameShift; i++)
        {
            voiceData[voiceDataCnt]= shortData[i];
            if(maxData<shortData[i])
                maxData=shortData[i];
            if(minData>shortData[i])
                minData=shortData[i];
            voiceDataCnt++;
        }

        frame_num++;

    }


    frame_num--;
    fclose(fp);
    *frameAmount = frame_num;
    /*printf("frame_num=%d ",frame_num);
    printf("voiceDataCnt=%d\n",voiceDataCnt);*/


    /*for( i=0; i<voiceDataCnt; i++ )
        voiceData[i]=voiceData[i] - minData/(maxData - minData);*/
}

void load_Gmmmodel(int person_id)
{
    char string[20];
    char FilePath[60];
    int i,j = 0;
    FILE *fp = NULL;

    sprintf(FilePath,"model/%d.gmm",person_id);
    fp=fopen(FilePath,"rb");

    fscanf(fp,"%s",string);
    for(i = 0; i < MixDim; i++)
        fscanf(fp,"%lf ",&mixcoef[i]);

    fscanf(fp,"%s",string);
    for(i = 0; i < MFCCDim; i++)
    {
        for(j = 0; j < MixDim; j++)
            fscanf(fp,"%lf ",&mean[i][j]);
    }

    fscanf(fp,"%s",string);
    for(i = 0; i < MixDim; i++)
    {
        for(j = 0; j < MFCCDim; j++)
            fscanf(fp,"%lf ",&covmatrix[i][j]);
    }

    fclose(fp);
}

int main()
{
	int  i,j,k,m,n;
	int mode = 0;

    int corec_num = 0;
    float corec_rate = 0.0;

	double rstProbArray[20];
	double rstProbmax;
	int Number=1;


	time_t  train_start,train_end,recog_start,recog_end;

	printf("---------********speaker recogniton_v1**********-----------\n");
	printf("              training ? or recognition ? \n");
    printf("          training and recognition: set mode equal 1\n");
    printf("          recognition directly: set mode equal 0\n");
    printf("mode : ");
    scanf("%d",&mode);



    train_start=time(0);

    if(mode == 1)
    {
        for( m=1; m<=7; m++)
            for( n=1; n<=3; n++)
            {
                read_voice(voiceData,m,n,mode,&frameAmount);

                mfcc_feature(voiceData,m,n,mode,&frameAmount,mel_cep);

                GmmTrainning(mel_cep,frameAmount,m);

            }


    }

    train_end=time(0);


	recog_start = time(0);
    mode = 0;
    if(mode == 0)
    {
        for( m=1; m<=7; m++)
            for( n=1; n<=3; n++)
            {
                read_voice(voiceData,m,n,mode,&frameAmount);

                mfcc_feature(voiceData,m,n,mode,&frameAmount,mel_cep);

                for ( k=1; k<=7; k++ )
                {
                    load_Gmmmodel(k);
                    rstProbArray[k]=CalGmmProb(mel_cep,frameAmount);
                    //printf("%f ",rstProbArray[k]);
                    if( k==1 )
                        rstProbmax=rstProbArray[1];

                    if ( rstProbmax <= rstProbArray[k] )
                    {
                        rstProbmax=rstProbArray[k];
                        Number=k;
                    }
                }

                printf("The result of recognition: No.%d \n",Number);
                if(Number == m)
                {
                    printf("recognize correctly :)\n");
                    corec_num ++;
                }
                else
                {
                    printf("recognize wrong!!!\n");
                }
            }
        corec_rate = (float)corec_num / 21;
        printf("recognition rate: %f \n", corec_rate);
    }




	recog_end=time(0);

	printf("the training time: %f\n",difftime(train_end,train_start));
	printf("the recognition time: %f\n",difftime(recog_end,recog_start));

	return 0;
}
