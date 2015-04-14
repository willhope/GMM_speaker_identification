#include "speaker.h"
#include "GMM.h"

void GmmTrainning(double mfcc[N][MFCCDim], int frameAmount, int person_id )
{
	int i,j,r,step;
	int count=0;
	double newProb,temp;
	double recProb=0.0;
    double   PI=3.1415926;
    double x= pow(2*PI,(-MFCCDim/2));

    char FilePath[60];
    FILE *fp = NULL;
	/*Initialization*/
	for( i=0; i<MixDim; i++ )
		mixcoef[i]=1.0/MixDim;

    step=(int)floor(frameAmount/MixDim);
	for( j=0; j<MixDim; j++ )
	    for( i=0; i<MFCCDim; i++ )
	     	mean[i][j]=mfcc[step*(j+1)-1][i];

	for( i=0; i<MixDim; i++ )
		for( j=0; j<MFCCDim; j++ )
			covmatrix[i][j]=1.0;

		/*Iterative processing*/
	while(1)
	{
	    	//step 3.1   renew probability of the model
		for( i=0; i<MixDim; i++ )
		{
			coefTemp[i]=1.0;
		    for( j=0; j<MFCCDim; j++ )
		     	coefTemp[i]*=1.0/covmatrix[i][j];
		    coefTemp[i]=sqrt(coefTemp[i]);
		    coefTemp[i]=x*coefTemp[i];
		}

		for( i=0; i<frameAmount; i++ )
			for( j=0; j<MixDim; j++)
			{
				expmatrix[i][j]=0.0;
				for( r=0; r<MFCCDim; r++)
					expmatrix[i][j]+=(pow( (mfcc[i][r]-mean[r][j]),2 ))*(-0.5)/covmatrix[j][r];
			}

		for( i=0; i<frameAmount; i++ )
			for( j=0; j<MixDim; j++)
			{
				if( j==0 )  expmatrixmax[i]=expmatrix[i][j];

		    	if( expmatrixmax[i]<expmatrix[i][j] )
					expmatrixmax[i]=expmatrix[i][j];
			}

		for( i=0; i<frameAmount; i++ )
			for( j=0; j<MixDim; j++)
			{
				expmatrix[i][j]-=expmatrixmax[i];

				expmatrix[i][j]=exp(expmatrix[i][j]);

				normProb[i][j]=expmatrix[i][j]*coefTemp[j];
			}

			//step 3.2  compare threshold value for iteration
	   	for( i=0; i<frameAmount; i++)
		{
		    mixnormProb[i]=0.0;
		    for( j=0; j<MixDim; j++)
			   	mixnormProb[i]=mixnormProb[i]+normProb[i][j]*mixcoef[j];
		}

      	newProb=0.0;
	    for( i=0; i<frameAmount; i++)
		   	newProb=newProb+log(mixnormProb[i])+expmatrixmax[i];

	    temp = newProb-recProb;
        //printf("%f\n ",temp);
	    if( ( temp<(GMM_Threshold*frameAmount) ) && ( count>0 ) )
			break;

	    	//step 3.3   E process, a probability matrix, n*MixDim
	   	for( i=0; i<frameAmount; i++)
		    for( j=0; j<MixDim; j++)
			   	postProb[i][j]=normProb[i][j]*mixcoef[j]/mixnormProb[i];

	    	//step 3.4: M process
	    for( i=0; i<MixDim; i++)
		{
		   	sumPostProb[i]=0.0;
		   	for( j=0; j<frameAmount; j++)
			   	sumPostProb[i]+=postProb[j][i];
		}

	    	//step 3.4.1: renew mixcoef
	   	for( i=0; i<MixDim; i++)
		   	mixcoef[i]=sumPostProb[i]/frameAmount;

		    //step 3.4.2: renew mean
		for( i=0; i<MFCCDim; i++ )
			for( j=0; j<MixDim; j++ )
			{
				sumPostProb1[i][j]=0.0;
				for( r=0; r<frameAmount; r++ )
			    	sumPostProb1[i][j]+=mfcc[r][i]*postProb[r][j];

				mean[i][j]=sumPostProb1[i][j]/sumPostProb[j];
			}

	    	//step 3.4.3: renew covmatrix
	   	for( i=0; i<MixDim; i++)
		   	for( j=0; j<MFCCDim; j++)
				{
			    	sumPostProb2[i][j]=0.0;
			    	for( r=0; r<frameAmount; r++)
				    	sumPostProb2[i][j]+=mfcc[r][j]*mfcc[r][j]*postProb[r][i];
					sumPostProb2[i][j]=sumPostProb2[i][j]/sumPostProb[i];
				}

	   	for( i=0; i<MixDim; i++)
	     	for( j=0; j<MFCCDim; j++)
			{
		   		covmatrix[i][j]=sumPostProb2[i][j]-mean[j][i]*mean[j][i];
			   	if( covmatrix[i][j]<=MinCov )
			       	covmatrix[i][j]=MinCov;
			}

	     	//step 3.5: prepare for next iteration
	  	recProb = newProb;
        count++;
	}

	printf("count=%d  \n",count);

    sprintf(FilePath,"model/%d.gmm",person_id);

    fp= fopen(FilePath,"wb");

    fprintf(fp,"mixcoef:\n");
    for(i = 0; i < MixDim; i++)
        fprintf(fp,"%lf ",mixcoef[i]);

    fprintf(fp,"\nmean:\n");
    for(i = 0; i < MFCCDim; i++)
    {
        for(j = 0; j < MixDim; j++)
            fprintf(fp,"%lf ",mean[i][j]);
        fprintf(fp,"\n");
    }

    fprintf(fp,"covmatrix:\n");
    for(i = 0; i < MixDim; i++)
    {
        for(j = 0; j < MFCCDim; j++)
            fprintf(fp,"%lf ",covmatrix[i][j]);
        fprintf(fp,"\n");
    }

    fclose(fp);
    return;
}
