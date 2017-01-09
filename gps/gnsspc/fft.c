#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "gps_pc_mode.h"
#include <cutils/log.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define  LOG_TAG  "LIBGPS_ENGPC"
#define  E(...)   ALOGE(__VA_ARGS__)
#define  D(...)   ALOGD(__VA_ARGS__)

#define A0 (0.21557895)
#define A1 (0.41663158)
#define A2 (0.277263158)
#define A3 (0.083578947)
#define A4 (0.006947368)


#define FS (4092000)
#define nDataCount (16384)  //4096
#define FIN (200000)

// The following line must be defined before including math.h to correctly define M_PI
#define PI M_PI /* pi to machine precision, defined in math.h */
#define TWOPI (2.0*PI)

char data_capture_flag = 0;
static int dataCount = 0;
static int lineCount = 0;
double arrIQData[2 * nDataCount + 1] = {0.0};
double arrWinAdd[nDataCount] = {0.0};
double fMaxMag = 0.0;
int MaxIndex = 0;
int CN0 = 0;

/*
FFT/IFFT routine. (see pages 507-508 of Numerical Recipes in C)
Inputs:
data[] : array of complex* data points of size 2*NFFT+1.
data[0] is unused,
* the n'th complex number x(n), for 0 <= n <= length(x)-1, is stored as:
data[2*n+1] = real(x(n))
data[2*n+2] = imag(x(n))
if length(Nx) < NFFT, the remainder of the array must be padded with zeros
nn : FFT order NFFT. This MUST be a power of 2 and >= length(x).
isign: if set to 1,
computes the forward FFT
if set to -1,
computes Inverse FFT - in this case the output values have
to be manually normalized by multiplying with 1/NFFT.
Outputs:
data[] : The FFT or IFFT results are stored in data, overwriting the input.
*/
void FFT(double data[], int nn, int isign)
{
	int n, mmax, m, j, istep, i;
	double wtemp, wr, wpr, wpi, wi, theta;
	double tempr, tempi;
	n = nn << 1;
	j = 1;

	for (i = 1; i < n; i += 2)
	{
		if (j > i)
		{
			tempr = data[j]; data[j] = data[i]; data[i] = tempr;
			tempr = data[j+1]; data[j+1] = data[i+1]; data[i+1] = tempr;
		}
		m = n >> 1;
		while (m >= 2 && j > m)
		{
			j -= m;
			m >>= 1;
		}
		j += m;
	}
	mmax = 2;
	while (n > mmax)
	{
		istep = 2*mmax;
		theta = TWOPI/(isign*mmax);
		wtemp = sin(0.5*theta);
		wpr = -2.0*wtemp*wtemp;
		wpi = sin(theta);
		wr = 1.0;
		wi = 0.0;
		for (m = 1; m < mmax; m += 2)
		{
			for (i = m; i <= n; i += istep)
			{
				j =i + mmax;
				tempr = wr*data[j] - wi*data[j+1];
				tempi = wr*data[j+1] + wi*data[j];
				data[j] = data[i] - tempr;
				data[j+1] = data[i+1] - tempi;
				data[i] += tempr;
				data[i+1] += tempi;
			}
			wr = (wtemp = wr)*wpr - wi*wpi + wr;
			wi = wi*wpr + wtemp*wpi + wi;
		}
		mmax = istep;
	}
}

void CalMag(double data[], double arryFFTMagData[])
{
	double fMax = 0.0;
	int nIndex = 0;
	double fCur = 0.0;
	int i;

	for(i = 0; i < nDataCount; i++)
	{
		fCur = sqrt(data[2*i+1]*data[2*i+1]+data[2*i+2]*data[2*i+2]) / nDataCount / 1024.0;
		arryFFTMagData[i] = fCur;
		if(fCur > fMax)
		{
			fMax = fCur;
			nIndex = i;
		}
	}

	fMaxMag = fMax;
	MaxIndex = nIndex;
}

void DataSwap(double data[], int count)
{
	double temp = 0.0;
	int i;

	for(i = 0; i < count / 2; i++)
	{
		temp = data[i];
		data[i] = data[i + count / 2];
		data[i + count / 2] = temp;
	}
}

void FindMax(double data[], int count)
{
	double maxValue = data[0];
	int maxIndex = 0;
	int i;

	for(i = 1; i < count; i++)
	{
		if(data[i] > maxValue)
		{
			maxValue = data[i];
			maxIndex = i;
		}
	}

	fMaxMag = maxValue;
	MaxIndex = maxIndex;
}

double CalSigPower(double arryFFTMagData[], int nSigBins)
{
	double fPowerSum = 0.0;
	double fSigPower = 0.0;
	int arryIndex[5] = {0};
	int i = 0;
	int count = 0;
	int k;

	switch(nSigBins)
	{
		case 0:
			fPowerSum = arryFFTMagData[nDataCount - 2] * arryFFTMagData[nDataCount - 2]
						+ arryFFTMagData[nDataCount - 1] * arryFFTMagData[nDataCount - 1]
						+ arryFFTMagData[0] * arryFFTMagData[0]
						+ arryFFTMagData[1] * arryFFTMagData[1]
						+ arryFFTMagData[2] * arryFFTMagData[2];

			arryIndex[0] = nDataCount - 2;
			arryIndex[1] = nDataCount - 1;
			arryIndex[2] = 0;
			arryIndex[3] = 1;
			arryIndex[4] = 2;
			break;

		case 1:
			fPowerSum = arryFFTMagData[nDataCount - 1] * arryFFTMagData[nDataCount - 1]
						+ arryFFTMagData[0] * arryFFTMagData[0]
						+ arryFFTMagData[1] * arryFFTMagData[1]
						+ arryFFTMagData[2] * arryFFTMagData[2]
						+ arryFFTMagData[3] * arryFFTMagData[3];
			arryIndex[0] = nDataCount - 1;
			arryIndex[1] = 0;
			arryIndex[2] = 1;
			arryIndex[3] = 2;
			arryIndex[4] = 3;
			break;

		case 16383:
			fPowerSum = arryFFTMagData[nDataCount - 3] * arryFFTMagData[nDataCount -3]
						+ arryFFTMagData[nDataCount -2] * arryFFTMagData[nDataCount - 2]
						+ arryFFTMagData[nDataCount - 1] * arryFFTMagData[nDataCount - 1]
						+ arryFFTMagData[0] * arryFFTMagData[0]
						+ arryFFTMagData[1] * arryFFTMagData[1];
			arryIndex[0] = nDataCount -3;
			arryIndex[1] = nDataCount -2;
			arryIndex[2] = nDataCount -1;
			arryIndex[3] = 0;
			arryIndex[4] = 1;
			break;

		case 16382:
			fPowerSum = arryFFTMagData[nDataCount -4] * arryFFTMagData[nDataCount -4]
						+ arryFFTMagData[nDataCount -3] * arryFFTMagData[nDataCount -3]
						+ arryFFTMagData[nDataCount -2] * arryFFTMagData[nDataCount -2]
						+ arryFFTMagData[nDataCount -1] * arryFFTMagData[nDataCount -1]
						+ arryFFTMagData[0] * arryFFTMagData[0];
			arryIndex[0] = nDataCount -4;
			arryIndex[1] = nDataCount -3;
			arryIndex[2] = nDataCount -2;
			arryIndex[3] = nDataCount -1;
			arryIndex[4] = 0;
			break;

		default:
			k = nSigBins - 2;

			for(i = 0; k <= nSigBins + 2; k++, count++, i++)
			{
				fPowerSum += arryFFTMagData[k] * arryFFTMagData[k];
				arryIndex[count] = k;
			}
	}

	for(i=0;i<5;i++)
	{
		arryFFTMagData[arryIndex[i]] = 0.0;
	}
	fSigPower = sqrt((double)fPowerSum);

	return fSigPower;
}

double CalNoisePower(double arryFFTMagData[])
{
	int k = 0;
	double fNoisePower = 0.0;
	double fSum = 0.0;
	double fNoiseBinLow = 0.0, fNoiseBinHigh = 0.0;
	double nFrqBin = FS / (double)nDataCount;

	fNoiseBinLow = nDataCount / 2 - floor(1000000.0 / nFrqBin) - 1;
	fNoiseBinHigh = nDataCount / 2 + floor(1000000.0 / nFrqBin) - 1;

	for(k = (int)fNoiseBinLow; k <= (int)fNoiseBinHigh; k++)
	{
		fSum += arryFFTMagData[k] * arryFFTMagData[k];
	}

	fNoisePower = sqrt((double)fSum);
	return fNoisePower;
}

double CalSNR(double arryFFTMagData[], double fMaxValue, int nMaxIndex)
{
	double power = 0.0, noise = 0.0, SNR = 0.0;

	double nFrqBin = FS / (double)nDataCount;
	int nSigBins1 = (int)floor(nDataCount / 2.0 + FIN / nFrqBin + 1);

	int nSigBins = (fMaxValue > arryFFTMagData[nSigBins1]) ? nMaxIndex : nSigBins1;

	power = CalSigPower(arryFFTMagData, nSigBins);
	noise = CalNoisePower(arryFFTMagData);

	SNR = 20 * log10(power / noise);

	return SNR;
}

void cw_data_capture(const char* nmea, int length)
{
	int i = 0;
	char byte1[3] = {0};
	char byte2[3] = {0};
	char byte3[3] = {0};
	char byte4[3] = {0};
	char data1 = 0, data2 = 0, data3 = 0, data4 = 0;
	long IHexVal = 0;
	long QHexVal = 0;
	double snr = 0.0;
	double arrFFTMagData[nDataCount];
	int n = 0;

	if(NULL == nmea)
	{
		return;
	}

	if(data_capture_flag == 1)
	{
		for(i = 0; i < length; i++)
		{
			if(!memcmp(&nmea[i],"DATA_CAPTURE_END",strlen("DATA_CAPTURE_END")))
			{
				D("####data capture end.");
				data_capture_flag = 0;
				dataCount = 0;
				lineCount = 0;
				break;
			}
		}
	}

	if(data_capture_flag == 1)
	{
		//save data
		lineCount++;
		if(lineCount % 5 != 0)
		{
			dataCount++;
			sscanf(nmea, "%s%s%s%s", &byte1, &byte2, &byte3, &byte4);

			data1 = strtol(byte1, 0, 16);
			data2 = strtol(byte2, 0, 16);
			data3 = strtol(byte3, 0, 16);
			data4 = strtol(byte4, 0, 16);
			IHexVal = (data2 << 8) + data1;
			QHexVal = (data4 << 8) + data3;
			IHexVal = IHexVal > 32768 ? IHexVal - 65535 : IHexVal;
			QHexVal = QHexVal > 32768 ? QHexVal - 65535 : QHexVal;

			arrIQData[2*(dataCount - 1) + 1] = (double)IHexVal;
			arrIQData[2*(dataCount - 1) + 2] = (double)QHexVal * -1;

			arrWinAdd[dataCount - 1] = A0 - A1 * cos(2 * PI * (dataCount - 1) / (nDataCount - 1))
										+ A2 * cos(4 * PI * (dataCount - 1) / (nDataCount - 1))
										- A3 * cos(6 * PI * (dataCount - 1) / (nDataCount - 1))
										+ A4 * cos(8 * PI * (dataCount - 1) / (nDataCount - 1));

			arrIQData[2*(dataCount - 1)+1] *= arrWinAdd[dataCount - 1];
			arrIQData[2*(dataCount - 1)+2] *= arrWinAdd[dataCount - 1];
		}

		if(dataCount == nDataCount)
		{
			dataCount = 0;
			memset(arrFFTMagData, 0, sizeof(double) * nDataCount);
			n = (int)(log((double)nDataCount) / log(2.0));
			FFT(arrIQData, nDataCount, -1);

			CalMag(arrIQData, arrFFTMagData);
			DataSwap(arrFFTMagData, nDataCount);
			FindMax(arrFFTMagData, nDataCount);

			snr = CalSNR(arrFFTMagData, fMaxMag, MaxIndex);

			CN0 = (int)(snr + 10 * log10(2e6));
			D("########CN0:%d########\n", CN0);
		}
	}

	if(data_capture_flag == 0)
	{
		for(i = 0; i < length; i++)
		{
			if(!memcmp(&nmea[i],"DATA_CAPTURE_BEGIN",strlen("DATA_CAPTURE_BEGIN")))
			{
				D("####data capture begin.");
				data_capture_flag = 1;
				dataCount = 0;
				lineCount = 0;
				fMaxMag = 0;
				MaxIndex = 0;
				memset(arrIQData, 0, sizeof(double) * (2*nDataCount+1));
				memset(arrWinAdd, 0, sizeof(double) * nDataCount);
				break;
			}
		}
	}
}

