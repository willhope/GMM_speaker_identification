#include "speaker.h"
#include "GMM.h"

double CalGmmProb( double mfcc[N][MFCCDim], int frameAmount)
{
	int i,j,k;
	double 	rstProb=0.0;
    double   PI=3.1415926;
    double x= pow(2*PI,(-MFCCDim/2));

	for( i=0; i<MixDim; i++ )
	{
		coefTemp[i]=1.0;
		for( j=0; j<MFCCDim; j++ )
			coefTemp[i] *= 1.0/covmatrix[i][j];
		coefTemp[i]=sqrt(coefTemp[i]);
		coefTemp[i]=x*coefTemp[i];
	}

	for( i=0; i<frameAmount; i++ )
		for( j=0; j<MixDim; j++)
		{
			expmatrix[i][j]=0.0;
			for( k=0; k<MFCCDim; k++)
				expmatrix[i][j]+=(pow( (mfcc[i][k]-mean[k][j]),2 ))*(-0.5)/covmatrix[j][k];
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
			expmatrix[i][j] -= expmatrixmax[i];

			expmatrix[i][j]=exp(expmatrix[i][j]);

			normProb[i][j]=expmatrix[i][j]*coefTemp[j];
		}

	for( i=0; i<frameAmount; i++)
	{
		mixnormProb[i]=0.0;
		for( j=0; j<MixDim; j++)
			mixnormProb[i] += normProb[i][j]*mixcoef[j];
	}

	for( i=0; i<frameAmount; i++)
	   	rstProb += log(mixnormProb[i])+expmatrixmax[i];

	return ( rstProb );
}




















