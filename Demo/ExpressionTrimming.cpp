#include "ExpressionTrimming.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <conio.h>
#include <vector>
#include "math.h"

# define MAX_DATA_LIMIT 15000
using namespace std;

fstream f1,f2,f3;

int digits = 10, utterences = 20, dataCount = 0, frameSize = 320, maxRange = 100000, iteration1 = 3, iteration2 = 20, testUtterences = 10;
long double energyPerFrame[100000], operand1[100000], operand2[100000], operator1[100000];
long double data[100000], Ri[13], Ai[13], frame[320], Ci[13] ;
static int p = 12;



fstream fi,fo;

//input variables
int N=5;
int M=32;
int T=0;
long double a[6][6];
long double a_avg[6][6];
long double b[6][33];
long double b_avg[6][33];
long double pi[6] = { 0.0, 1.0, 0.0, 0.0, 0.0, 0.0 };
int obs[500];

//solution to problem 1 variables
long double alpha[500][6];
long double beta[500][6];
long double prob1 = 0.0;

//solution to problem 2 variables
long double gamma[500][6];
long double delta[500][6];
long double psy[500][6];
long double pStar;
int qStar[500];

//solution to problem 3 variables
long double et[500][6][6];

long double newpi[6];
long double newa[6][6];
long double newb[6][33];
long double XiSum[6][6];
long double gammaSum[6];

//tokhura weights
long double tw[12] = {1.0, 3.0, 7.0, 13.0, 19.0, 22.0, 25.0, 33.0, 42.0, 50.0, 56.0, 61.0};

void initialization()
{
	for(int i =0; i< 500; i++)
	{
		for(int j = 0; j<6; j++)
		{
			alpha[i][j]=0;
			beta[i][j]=0;
			delta[i][j]=0;
			psy[i][j]=0;
			gamma[i][j]=0;
		}
		qStar[i]=0;
	}
	for(int k = 0; k<500; k++)
	{
		for(int i =0; i< 6; i++)
		{
			for(int j = 0; j<6; j++)
			{
				et[k][i][j]=0;
				newa[i][j] = 0;
				XiSum[i][j]= 0;
			}
			newpi[i]=0;
			for(int j = 0; j<33; j++)
			{
				newb[i][j]=0;
			}
			gammaSum[i] = 0;
		}
	}
	prob1 = 0.0;
	pStar = 0;

}


//to normalize the data within the range +- 5000;
void normalize(long double maxVal, long double minVal)
{
	if( abs(minVal) > maxVal )
		maxVal = abs(minVal);
	long double range = 5000;
	for(int i = 0; i < dataCount; ++i)
		data[i] = ((data[i])/maxVal)*range;
}

//To remove the dc shift that exists due to ambient noise
void removeDcShift(long double avg)
{
	int nc=0;
	int noise[200000];
	long double value = 0.0;
	long double max = 0.0;
	FILE* fp = fopen("silence.txt","r");
	if(fp) {
		while(fscanf(fp,"%d",&noise[nc++]) != EOF);
		nc--;
		for(int i=0 ; i<nc ; i++)
			value += noise[i];
		value /= nc;
		for(int i=0 ; i<dataCount ; i++){
			data[i] -= value;
			if(max < abs(data[i])) max = abs(data[i]);
		}
		for(int i=0 ; i<dataCount ; i++){
			data[i] = (data[i]*5000)/max;
		}
	}
	else printf("cant open file");
}

// fetching signal from txt file intially
void fetchData()
{
	for(int i=0; i<maxRange; i++)
		data[i]=0;
	long double val =0, maxVal =0, minVal =0, sum =0, avg =0;
	int flag = 0;
	dataCount = 0;
	while ( f1>>val )
	{
		dataCount++;
		data[dataCount-1] = val;
		sum += val;
		if(flag = 0)
		{
			maxVal = val;
			minVal = val;
			flag = 1;
		}
		if( val > maxVal)
			maxVal = val;
		else if( val < minVal)
			minVal = val;
	}

	//padding zeros if number of frames is not integer
	float i = floorf(dataCount/frameSize);
	if( (dataCount/frameSize) - i != 0)
	{
		int noOfZerosPadded = frameSize - (dataCount - i*frameSize);
		for(int j = 0; j<noOfZerosPadded; j++)
			data[dataCount +j] = 0;
	}
	avg = sum/dataCount;
	removeDcShift(avg);
	normalize(maxVal, minVal);
}

//finds energy for each frame, stores it and returns frame number whose energy was maximum
void findEnergy()
{
	for(int i=0; i<maxRange; i++)
		energyPerFrame[i] = 0;
	int remainder = dataCount%frameSize;
	for( int i=0; i<(frameSize - remainder) ; i++)
		data[dataCount + i] = 0;
	int noOfFrames = ceilf(dataCount/frameSize);
	int maxEnergyFrame = 0, flag = 0;

	for (int i = 0; i < noOfFrames; i++)
	{
		long double energy = 0;
		for(int j = ((i == 0) ? 0 : (frameSize*i)) ;
			j < ((i== (dataCount/frameSize - 1))? dataCount : (frameSize*(i+1)));
			j++)
		{
			energy = (energy + (data[j])*(data[j]));
		}
		energy = energy/frameSize;
		energyPerFrame[i] = energy;
	}
}

markers setMarkers()
{
	markers m;
	m.end1 = 0;m.end2=0;m.end3=0;m.start1=0;m.start2=0;m.start3=0;

	//start1
	int k = ceilf(dataCount/frameSize);
	for(int i=0;i<k;i++)
	{
		if((2*(energyPerFrame[i] ))<=(energyPerFrame[i+1]))
		{
			m.start1=i;
			if(energyPerFrame[i+1] > 5000)
			{
				m.start1=i+1;
				break;
			}
		}
		else
			m.start1 = 30;	//hardcoded
	}

	//end1
	m.end1 = m.start1+int(7000/320);
	for(int i = m.end1; i>m.start1; i--)
	{
		if((1.5*(energyPerFrame[i]))<=(energyPerFrame[i-1]))
		{
			m.end1 =i;
			if(energyPerFrame[i-1] > 5000)
				break;
		}
		else
			m.end1 = m.start1 + int(7000/320);
	}

	//end3
	k = 0;
	int end=ceilf(dataCount/frameSize)-1;
	for(int i=end;i>=k;i--)
	{
		if((1.5*(energyPerFrame[i]))<=(energyPerFrame[i-1]))
		{
			m.end3 =i;
			if(energyPerFrame[i-1] > 5000)
				break;
		}
		else
			m.end3 = ceilf(dataCount/frameSize);
	}

	//start3
	m.start3=m.end3-int(7000/320);
	for(int i = m.start3; i<m.end3; i++)
	{
		if((2*(energyPerFrame[i]))<=(energyPerFrame[i+1]))
		{
			m.start3 =i;
			if(energyPerFrame[i+1] > 5000)

				break;
		}
		else
			m.start3 = m.end3 - int(7000/320);
	}

	//middle part
	//start2
	m.start2 = m.end1;
	for(int i = m.start2; i<m.start3; i++)
	{
		if((2*(energyPerFrame[i]))<=(energyPerFrame[i+1]))
		{
			m.start2 =i;
			if(energyPerFrame[i+1] > 5000)
				break;
		}
		else
			m.start2 = m.end1;
	}

	//end2
	m.end2 =m.start3;
	for(int i = m.end2; i>m.start2; i--)
	{
		if((1.5*(energyPerFrame[i]))<=(energyPerFrame[i-1]))
		{
			m.end2 =i;
			if(energyPerFrame[i-1] > 5000)
			{
				m.end2 =i;
				break;
			}
		}
		else
			m.end2 = m.start3;
	}
	return m;
} 

void trimSignal(markers m)
{
	char * fileNameOfOperand_1 = "operand1.txt";
	char * fileNameOfOperand_2 = "operand2.txt"; 
	char * fileNameOfOperator = "operator.txt";

	int dcOperand1= 0, dcOperand2 = 0, dcOperator=0;
	long double temp[100000];

	//for operator1
	for(int i =0; i<100000; i++)
		temp[i]=0;
	for(int i=m.start1*frameSize; i<(m.end1 + 1)*frameSize; i++)
	{
		temp[dcOperand1]=data[i];
		dcOperand1++;
	}
	for(int i=0; i<dcOperand1; i++)
		operand1[i] = temp[i];
	for(int i = dcOperand1 ; i< 100000; i++)
		operand1[i]=0;

	// writing operand data into file
	FILE * f = fopen(fileNameOfOperand_1, "w");
	for(int i = 0; i < MAX_DATA_LIMIT; i++) {
		fprintf(f,"%Lf\n", operand1[i]);
	}
	fclose(f);


	//for operand
	for(int i =0; i<100000; i++)
		temp[i]=0;
	for(int i=m.start2*frameSize; i<(m.end2 + 1)*frameSize; i++)
	{
		temp[dcOperator]=data[i];
		dcOperator++;
	}
	for(int i=0; i<dcOperator; i++)
		operator1[i] = temp[i];
	for(int i = dcOperator ; i< 100000; i++)
		operator1[i]=0;

	// writing operator data into file
	f = fopen(fileNameOfOperator, "w");
	for(int i = 0; i < MAX_DATA_LIMIT; i++) {
		fprintf(f,"%Lf\n", operator1[i]);
	}
	fclose(f);
	//for operator2
	for(int i =0; i<100000; i++)
		temp[i]=0;
	for(int i=m.start3*frameSize; i<(m.end3 + 1)*frameSize; i++)
	{
		temp[dcOperand2]=data[i];
		dcOperand2++;
	}
	for(int i=0; i<dcOperand2; i++)
		operand2[i] = temp[i];
	for(int i = dcOperand2 ; i< 100000; i++)
		operand2[i]=0;

	// writing operand data into file
	f = fopen(fileNameOfOperand_2, "w");
	for(int i = 0; i < MAX_DATA_LIMIT; i++) {
		fprintf(f,"%Lf\n", operand2[i]);
	}
	fclose(f);
}

void step5()
{
	for(int i=0; i<1; i++)
	{
		initialization();
		
		f1.open("data.txt",ios::in);
		fetchData();
		f1.close();

		//finding energy and triming the signal
		int startFrameNo = 0, endFrameNo = 0;
		findEnergy();
		markers m = setMarkers();
		trimSignal(m);
	}

}

