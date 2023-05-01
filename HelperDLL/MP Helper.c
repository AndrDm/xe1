//==============================================================================
//
// Title:		MP Helper
// Purpose:		Helper DLL for Multiscale Pyramids.
//
// Created on:	30.04.2023 at 18:08:18 by AD.
// Copyright:	AD. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include "MP Helper.h"

static CmtThreadPoolHandle PoolHandle;

inline double fastPow(double a, double b) {
    union {
        double d;
        int x[2];
    } u = { a };
    u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
    u.x[0] = 0;
    return u.d;
}

int PrepareThreadPool(int NumThreads)
{
	int maximumNumberOfThreads, numberOfThreadsCreated;
	maximumNumberOfThreads = (NumThreads)?NumThreads:4;
	CmtNewThreadPool(maximumNumberOfThreads, &PoolHandle);
	// incurs the overhead of creating threads.
	CmtPreAllocThreadPoolThreads(PoolHandle, maximumNumberOfThreads, &numberOfThreadsCreated);
	
	return numberOfThreadsCreated; //Shall be equal to request
}

int UnprepareThreadPool()
{
	CmtDiscardThreadPool(PoolHandle);
	
	return 0;
}


int CVICALLBACK TransformThreadFunction1 (void *functionData);
int CVICALLBACK TransformThreadFunction2 (void *functionData);
static 	float *LVImagePtrSrcSGLGLB1, *LVImagePtrSrcSGLGLB2;
static	int LVWidthGLB, LVHeightGLB, LVLineWidthSrcGLB;
double DividerGLB, PowerGLB, MultiplierGLB;

void ApplyTransformL(void *SrcImage, int StartLine, int EndLine, double Divider, double Power, double Multiplier,  LVErrorCluster *ErrorCluster)
{
	Image *ImgSrc;
	float *LVImagePtrSrcSGL;
 	int LVWidth, LVHeight,	LVLineWidthSrc, x, y;

	CHECK_ERROR_IN(ErrorCluster);
	LV_IS_NOT_IMAGE(SrcImage, ErrorCluster);

	ImgSrc = ADV_LVDTToAddress(SrcImage);
	LV_IS_NOT_IMAGE(ImgSrc, ErrorCluster);

 	LVWidthGLB = LVWidth = ((ImageInfo *)ImgSrc)->xRes;
	LVHeightGLB = LVHeight = ((ImageInfo *)ImgSrc)->yRes;
	LVLineWidthSrcGLB = LVLineWidthSrc = ((ImageInfo *)ImgSrc)->pixelsPerLine;
	DividerGLB = Divider; PowerGLB = Power; MultiplierGLB = Multiplier;
	
	switch (((ImageInfo *)ImgSrc)->imageType){
		case IMAQ_IMAGE_SGL:
			LVImagePtrSrcSGLGLB1 = LVImagePtrSrcSGLGLB2 = LVImagePtrSrcSGL = (float *)((ImageInfo *)ImgSrc)->imageStart;
			LV_IS_NOT_IMAGE(LVImagePtrSrcSGL, ErrorCluster);
			break;
		default:
			ADV_SetLVError(ERR_INVALID_IMAGE_TYPE, __func__, ErrorCluster);
			return;
	}

	double temp0, temp1, temp2;
	LVImagePtrSrcSGL += (StartLine * LVLineWidthSrc);
	for (y = StartLine; y < EndLine; y++){
		for (x = 0; x < LVWidth; x++){
			temp0 = *LVImagePtrSrcSGL;
			temp1 = abs(*LVImagePtrSrcSGL) / Divider;
			if (temp1){
				temp2 = fastPow(temp1, Power);
				*LVImagePtrSrcSGL = temp2 * Multiplier;
			}
			else *LVImagePtrSrcSGL = 0.0;
			if (temp0 < 0) *LVImagePtrSrcSGL = *LVImagePtrSrcSGL * -1.0;
			
			LVImagePtrSrcSGL++;
		}
		LVImagePtrSrcSGL += (LVLineWidthSrc - LVWidth);
	}
}


void CastImageL(void *SrcImage, void *DstImage, int StartLine, int EndLine, double Multiplier, LVErrorCluster *ErrorCluster)
{
	Image *ImgSrc, *ImgDst;
	float *LVImagePtrSrcSGL;
	unsigned short int *LVImagePtrDstU16;
 	int LVWidth, LVHeight,	LVLineWidthSrc, LVLineWidthDst, x, y;

	CHECK_ERROR_IN(ErrorCluster);
	LV_IS_NOT_IMAGE(SrcImage, ErrorCluster);
	LV_IS_NOT_IMAGE(DstImage, ErrorCluster);
	
	ImgSrc = ADV_LVDTToAddress(SrcImage);	LV_IS_NOT_IMAGE(ImgSrc, ErrorCluster);
	ImgDst = ADV_LVDTToAddress(DstImage);	LV_IS_NOT_IMAGE(ImgDst, ErrorCluster);


 	LVWidth = ((ImageInfo *)ImgSrc)->xRes;
	LVHeight = ((ImageInfo *)ImgSrc)->yRes;
	LVLineWidthSrc = ((ImageInfo *)ImgSrc)->pixelsPerLine;
	LVLineWidthDst = ((ImageInfo *)ImgDst)->pixelsPerLine;
	
	switch (((ImageInfo *)ImgSrc)->imageType){
		case IMAQ_IMAGE_SGL:
			LVImagePtrSrcSGL = (float *)((ImageInfo *)ImgSrc)->imageStart;
			LV_IS_NOT_IMAGE(LVImagePtrSrcSGL, ErrorCluster);
			break;
		default:
			ADV_SetLVError(ERR_INVALID_IMAGE_TYPE, __func__, ErrorCluster);
			return;
	}
	
	switch (((ImageInfo *)ImgDst)->imageType){
		case IMAQ_IMAGE_U16:
			LVImagePtrDstU16 = (unsigned short *)((ImageInfo *)ImgDst)->imageStart;
			LV_IS_NOT_IMAGE(LVImagePtrDstU16, ErrorCluster);
			break;
		default:
			ADV_SetLVError(ERR_INVALID_IMAGE_TYPE, __func__, ErrorCluster);
			return;
	}

	LVImagePtrSrcSGL += (StartLine * LVLineWidthSrc);
	LVImagePtrDstU16 += (StartLine * LVLineWidthDst);
	
	for (y = StartLine; y < EndLine; y++){
		for (x = 0; x < LVWidth; x++){
			*LVImagePtrDstU16 = *LVImagePtrSrcSGL;
			LVImagePtrDstU16++;
			LVImagePtrSrcSGL++;
		}
		LVImagePtrSrcSGL += (LVLineWidthSrc - LVWidth);
		LVImagePtrDstU16 += (LVLineWidthDst - LVWidth);
	}
}


void CastImageL1(void *SrcImage, void *DstImage, int StartLine, int EndLine, double Multiplier, LVErrorCluster *ErrorCluster)
{
	Image *ImgSrc, *ImgDst;
	float *LVImagePtrSrcSGL;
	unsigned short int *LVImagePtrDstU16;
 	int LVWidth, LVHeight,	LVLineWidthSrc, LVLineWidthDst, x, y;

	CHECK_ERROR_IN(ErrorCluster);
	LV_IS_NOT_IMAGE(SrcImage, ErrorCluster);
	LV_IS_NOT_IMAGE(DstImage, ErrorCluster);
	
	ImgSrc = ADV_LVDTToAddress(SrcImage);	LV_IS_NOT_IMAGE(ImgSrc, ErrorCluster);
	ImgDst = ADV_LVDTToAddress(DstImage);	LV_IS_NOT_IMAGE(ImgDst, ErrorCluster);


 	LVWidth = ((ImageInfo *)ImgSrc)->xRes;
	LVHeight = ((ImageInfo *)ImgSrc)->yRes;
	LVLineWidthSrc = ((ImageInfo *)ImgSrc)->pixelsPerLine;
	LVLineWidthDst = ((ImageInfo *)ImgDst)->pixelsPerLine;
	
	switch (((ImageInfo *)ImgSrc)->imageType){
		case IMAQ_IMAGE_SGL:
			LVImagePtrSrcSGL = (float *)((ImageInfo *)ImgSrc)->imageStart;
			LV_IS_NOT_IMAGE(LVImagePtrSrcSGL, ErrorCluster);
			break;
		default:
			ADV_SetLVError(ERR_INVALID_IMAGE_TYPE, __func__, ErrorCluster);
			return;
	}
	
	switch (((ImageInfo *)ImgDst)->imageType){
		case IMAQ_IMAGE_U16:
			LVImagePtrDstU16 = (unsigned short *)((ImageInfo *)ImgDst)->imageStart;
			LV_IS_NOT_IMAGE(LVImagePtrDstU16, ErrorCluster);
			break;
		default:
			ADV_SetLVError(ERR_INVALID_IMAGE_TYPE, __func__, ErrorCluster);
			return;
	}

	LVImagePtrSrcSGL += (StartLine * LVLineWidthSrc);
	LVImagePtrDstU16 += (StartLine * LVLineWidthDst);
	
        union convert
        {
                float f;
                unsigned short int i;
        } pixel;	
	
	for (y = StartLine; y < EndLine; y++){
		for (x = 0; x < LVWidth; x++){
			pixel.f = *LVImagePtrSrcSGL;
			*LVImagePtrDstU16 = pixel.i;
			LVImagePtrDstU16++;
			LVImagePtrSrcSGL++;
		}
		LVImagePtrSrcSGL += (LVLineWidthSrc - LVWidth);
		LVImagePtrDstU16 += (LVLineWidthDst - LVWidth);
	}
}

void ApplyTransformL1(void *SrcImage, int StartLine, int EndLine, double Divider, double Power, double Multiplier,  LVErrorCluster *ErrorCluster)
{
	Image *ImgSrc;
	float *LVImagePtrSrcSGL;
 	int LVWidth, LVHeight,	LVLineWidthSrc, x, y;

	CHECK_ERROR_IN(ErrorCluster);
	LV_IS_NOT_IMAGE(SrcImage, ErrorCluster);

	ImgSrc = ADV_LVDTToAddress(SrcImage);
	LV_IS_NOT_IMAGE(ImgSrc, ErrorCluster);

 	LVWidthGLB = LVWidth = ((ImageInfo *)ImgSrc)->xRes;
	LVHeightGLB = LVHeight = ((ImageInfo *)ImgSrc)->yRes;
	LVLineWidthSrcGLB = LVLineWidthSrc = ((ImageInfo *)ImgSrc)->pixelsPerLine;
	DividerGLB = Divider; PowerGLB = Power; MultiplierGLB = Multiplier;
	
	switch (((ImageInfo *)ImgSrc)->imageType){
		case IMAQ_IMAGE_SGL:
			LVImagePtrSrcSGLGLB1 = LVImagePtrSrcSGLGLB2 = LVImagePtrSrcSGL = (float *)((ImageInfo *)ImgSrc)->imageStart;
			LV_IS_NOT_IMAGE(LVImagePtrSrcSGL, ErrorCluster);
			break;
		default:
			ADV_SetLVError(ERR_INVALID_IMAGE_TYPE, __func__, ErrorCluster);
			return;
	}

	double temp0, temp1, temp2;
	LVImagePtrSrcSGL += (StartLine * LVLineWidthSrc);
	for (y = StartLine; y < EndLine; y++){
		for (x = 0; x < LVWidth; x++){
			temp0 = *LVImagePtrSrcSGL;
			if (temp0){
				temp1 = abs(temp0) / Divider;
				temp2 = fastPow(temp1, Power);
				*LVImagePtrSrcSGL = temp2 * Multiplier;
			}
			else *LVImagePtrSrcSGL = 0.0;
			if (temp0 < 0) *LVImagePtrSrcSGL = *LVImagePtrSrcSGL * -1.0;
			
			LVImagePtrSrcSGL++;
		}
		LVImagePtrSrcSGL += (LVLineWidthSrc - LVWidth);
	}
}

void ApplyTransformL2(void *SrcImage, int StartLine, int EndLine, double Divider, double Power, double Multiplier,  LVErrorCluster *ErrorCluster)
{
	Image *ImgSrc;
	float *LVImagePtrSrcSGL;
 	int LVWidth, LVHeight,	LVLineWidthSrc, x, y;

	CHECK_ERROR_IN(ErrorCluster);
	LV_IS_NOT_IMAGE(SrcImage, ErrorCluster);

	ImgSrc = ADV_LVDTToAddress(SrcImage);
	LV_IS_NOT_IMAGE(ImgSrc, ErrorCluster);

 	LVWidthGLB = LVWidth = ((ImageInfo *)ImgSrc)->xRes;
	LVHeightGLB = LVHeight = ((ImageInfo *)ImgSrc)->yRes;
	LVLineWidthSrcGLB = LVLineWidthSrc = ((ImageInfo *)ImgSrc)->pixelsPerLine;
	DividerGLB = Divider; PowerGLB = Power; MultiplierGLB = Multiplier;
	
	switch (((ImageInfo *)ImgSrc)->imageType){
		case IMAQ_IMAGE_SGL:
			LVImagePtrSrcSGLGLB1 = LVImagePtrSrcSGLGLB2 = LVImagePtrSrcSGL = (float *)((ImageInfo *)ImgSrc)->imageStart;
			LV_IS_NOT_IMAGE(LVImagePtrSrcSGL, ErrorCluster);
			break;
		default:
			ADV_SetLVError(ERR_INVALID_IMAGE_TYPE, __func__, ErrorCluster);
			return;
	}

	double temp0, temp1, temp2;
	LVImagePtrSrcSGL += (StartLine * LVLineWidthSrc);
	for (y = StartLine; y < EndLine; y++){
		for (x = 0; x < LVWidth; x++){
			temp0 = *LVImagePtrSrcSGL;
			if (temp0){
				temp1 = abs(temp0) / Divider;
				temp2 = fastPow(temp1, Power);
				*LVImagePtrSrcSGL = temp2 * Multiplier;
			}
			else *LVImagePtrSrcSGL = 0.0;
			if (temp0 < 0) *LVImagePtrSrcSGL = *LVImagePtrSrcSGL * -1.0;
			
			LVImagePtrSrcSGL++;
		}
		LVImagePtrSrcSGL += (LVLineWidthSrc - LVWidth);
	}
}

void ApplyTransform(void *SrcImage, double Divider, double Power, double Multiplier,  LVErrorCluster *ErrorCluster)
{
	Image *ImgSrc;
	float *LVImagePtrSrcSGL;
 	int LVWidth, LVHeight,	LVLineWidthSrc, x, y;

	CHECK_ERROR_IN(ErrorCluster);
	LV_IS_NOT_IMAGE(SrcImage, ErrorCluster);

	ImgSrc = ADV_LVDTToAddress(SrcImage);
	LV_IS_NOT_IMAGE(ImgSrc, ErrorCluster);

 	LVWidthGLB = LVWidth = ((ImageInfo *)ImgSrc)->xRes;
	LVHeightGLB = LVHeight = ((ImageInfo *)ImgSrc)->yRes;
	LVLineWidthSrcGLB = LVLineWidthSrc = ((ImageInfo *)ImgSrc)->pixelsPerLine;
	DividerGLB = Divider; PowerGLB = Power; MultiplierGLB = Multiplier;
	
	switch (((ImageInfo *)ImgSrc)->imageType){
		case IMAQ_IMAGE_SGL:
			LVImagePtrSrcSGLGLB1 = LVImagePtrSrcSGLGLB2 = LVImagePtrSrcSGL = (float *)((ImageInfo *)ImgSrc)->imageStart;
			LV_IS_NOT_IMAGE(LVImagePtrSrcSGL, ErrorCluster);
			break;
		default:
			ADV_SetLVError(ERR_INVALID_IMAGE_TYPE, __func__, ErrorCluster);
			return;
	}

	double temp0, temp1, temp2;
	for (y = 0; y < LVHeight; y++){
		for (x = 0; x < LVWidth; x++){
			temp0 = *LVImagePtrSrcSGL;
			if (temp0){
				temp1 = abs(temp0) / Divider;
				temp2 = fastPow(temp1, Power);
				*LVImagePtrSrcSGL = temp2 * Multiplier;
			}
			else *LVImagePtrSrcSGL = 0.0;
			if (temp0 < 0) *LVImagePtrSrcSGL = *LVImagePtrSrcSGL * -1.0;
			
			LVImagePtrSrcSGL++;
		}
		LVImagePtrSrcSGL += (LVLineWidthSrc - LVWidth);
	}
	
}

void ApplyTransformP(void *SrcImage, double Divider, double Power, double Multiplier,  LVErrorCluster *ErrorCluster)
{
	Image *ImgSrc;
	float *LVImagePtrSrcSGL;
 	int LVWidth, LVHeight,	LVLineWidthSrc;
	int functionId1, functionId2;

	CHECK_ERROR_IN(ErrorCluster);
	LV_IS_NOT_IMAGE(SrcImage, ErrorCluster);

	ImgSrc = ADV_LVDTToAddress(SrcImage);
	LV_IS_NOT_IMAGE(ImgSrc, ErrorCluster);

 	LVWidthGLB = LVWidth = ((ImageInfo *)ImgSrc)->xRes;
	LVHeightGLB = LVHeight = ((ImageInfo *)ImgSrc)->yRes;
	LVLineWidthSrcGLB = LVLineWidthSrc = ((ImageInfo *)ImgSrc)->pixelsPerLine;
	DividerGLB = Divider; PowerGLB = Power; MultiplierGLB = Multiplier;
	
	switch (((ImageInfo *)ImgSrc)->imageType){
		case IMAQ_IMAGE_SGL:
			LVImagePtrSrcSGLGLB1 = LVImagePtrSrcSGLGLB2 = LVImagePtrSrcSGL = (float *)((ImageInfo *)ImgSrc)->imageStart;
			LV_IS_NOT_IMAGE(LVImagePtrSrcSGL, ErrorCluster);
			break;
		default:
			ADV_SetLVError(ERR_INVALID_IMAGE_TYPE, __func__, ErrorCluster);
			return;
	}

	CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE, TransformThreadFunction1, NULL, &functionId1);
	CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE, TransformThreadFunction2, NULL, &functionId2);
	//Do the job
	CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE, functionId1, 0);
	CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE, functionId2, 0);
	
}

void ApplyTransformP2(void *SrcImage, double Divider, double Power, double Multiplier,  LVErrorCluster *ErrorCluster)
{
	Image *ImgSrc;
	float *LVImagePtrSrcSGL;
 	int LVWidth, LVHeight,	LVLineWidthSrc;
	int functionId1, functionId2;

	CHECK_ERROR_IN(ErrorCluster);
	LV_IS_NOT_IMAGE(SrcImage, ErrorCluster);

	ImgSrc = ADV_LVDTToAddress(SrcImage);
	LV_IS_NOT_IMAGE(ImgSrc, ErrorCluster);

 	LVWidthGLB = LVWidth = ((ImageInfo *)ImgSrc)->xRes;
	LVHeightGLB = LVHeight = ((ImageInfo *)ImgSrc)->yRes;
	LVLineWidthSrcGLB = LVLineWidthSrc = ((ImageInfo *)ImgSrc)->pixelsPerLine;
	DividerGLB = Divider; PowerGLB = Power; MultiplierGLB = Multiplier;
	
	switch (((ImageInfo *)ImgSrc)->imageType){
		case IMAQ_IMAGE_SGL:
			LVImagePtrSrcSGLGLB1 = LVImagePtrSrcSGLGLB2 = LVImagePtrSrcSGL = (float *)((ImageInfo *)ImgSrc)->imageStart;
			LV_IS_NOT_IMAGE(LVImagePtrSrcSGL, ErrorCluster);
			break;
		default:
			ADV_SetLVError(ERR_INVALID_IMAGE_TYPE, __func__, ErrorCluster);
			return;
	}

	CmtScheduleThreadPoolFunction (PoolHandle, TransformThreadFunction1, NULL, &functionId1); //Still overhead around 150 ms
	CmtScheduleThreadPoolFunction (PoolHandle, TransformThreadFunction2, NULL, &functionId2);
	//Do the job
	Sleep(5);
	CmtWaitForThreadPoolFunctionCompletion (PoolHandle, functionId1, 0);//OPT_TP_PROCESS_EVENTS_WHILE_WAITING 
	CmtWaitForThreadPoolFunctionCompletion (PoolHandle, functionId2, 0);
	CmtReleaseThreadPoolFunctionID(PoolHandle, functionId1);
	CmtReleaseThreadPoolFunctionID(PoolHandle, functionId2);
}


//Not very eleganth, but sufficient to proof concept
//Spoiler - too slow due to overhead
int CVICALLBACK TransformThreadFunction1 (void *functionData)
{
	double temp0, temp1, temp2;
	for (int y = 0; y < LVHeightGLB/2; y++){
		for (int x = 0; x < LVWidthGLB; x++){
			temp0 = *LVImagePtrSrcSGLGLB1;
			temp1 = abs(*LVImagePtrSrcSGLGLB1) / DividerGLB;
			if (temp1){
				temp2 = fastPow(temp1, PowerGLB);
				*LVImagePtrSrcSGLGLB1 = temp2 * MultiplierGLB;
			}
			else *LVImagePtrSrcSGLGLB1 = 0.0;
			if (temp0 < 0) *LVImagePtrSrcSGLGLB1 = *LVImagePtrSrcSGLGLB1 * -1.0;
			
			LVImagePtrSrcSGLGLB1++;
		}
		LVImagePtrSrcSGLGLB1 += (LVLineWidthSrcGLB - LVWidthGLB);
	}

    return 0;
}

int CVICALLBACK TransformThreadFunction2 (void *functionData)
{
	double temp0, temp1, temp2;
	LVImagePtrSrcSGLGLB2 += (LVHeightGLB/2 * LVLineWidthSrcGLB);
	for (int y = LVHeightGLB/2; y < LVHeightGLB; y++){
		for (int x = 0; x < LVWidthGLB; x++){
			temp0 = *LVImagePtrSrcSGLGLB2;
			temp1 = abs(*LVImagePtrSrcSGLGLB2) / DividerGLB;
			if (temp1){
				temp2 = fastPow(temp1, PowerGLB);
				*LVImagePtrSrcSGLGLB2 = temp2 * MultiplierGLB;
			}
			else *LVImagePtrSrcSGLGLB2 = 0.0;
			if (temp0 < 0) *LVImagePtrSrcSGLGLB2 = *LVImagePtrSrcSGLGLB2 * -1.0;
			
			LVImagePtrSrcSGLGLB2++;
		}
		LVImagePtrSrcSGLGLB2 += (LVLineWidthSrcGLB - LVWidthGLB);
	}

    return 0;
}


void ApplyPower(void *SrcImage, double Power, LVErrorCluster *ErrorCluster)
{
	Image *ImgSrc;
	float *LVImagePtrSrcSGL;
 	int LVWidth, LVHeight,	LVLineWidthSrc, x, y;

	CHECK_ERROR_IN(ErrorCluster);
	LV_IS_NOT_IMAGE(SrcImage, ErrorCluster);

	ImgSrc = ADV_LVDTToAddress(SrcImage);
	LV_IS_NOT_IMAGE(ImgSrc, ErrorCluster);

 	LVWidth = ((ImageInfo *)ImgSrc)->xRes;
	LVHeight = ((ImageInfo *)ImgSrc)->yRes;
	LVLineWidthSrc = ((ImageInfo *)ImgSrc)->pixelsPerLine;
	
	switch (((ImageInfo *)ImgSrc)->imageType){
		case IMAQ_IMAGE_SGL:
			LVImagePtrSrcSGL = (float *)((ImageInfo *)ImgSrc)->imageStart;
			LV_IS_NOT_IMAGE(LVImagePtrSrcSGL, ErrorCluster);
			break;
		default:
			ADV_SetLVError(ERR_INVALID_IMAGE_TYPE, __func__, ErrorCluster);
			return;
	}

	double register temp;
	for (y = 0; y < LVHeight; y++){
		for (x = 0; x < LVWidth; x++){
			temp = *LVImagePtrSrcSGL;
			*LVImagePtrSrcSGL++ = fastPow(temp, Power);			
		}
		LVImagePtrSrcSGL += (LVLineWidthSrc - LVWidth);
	}

}





/// HIFN  What does your function do?
/// HIPAR x/What inputs does your function expect?
/// HIRET What does your function return?
int Your_Functions_Here (int x)
{
	return x;
}

//==============================================================================
// DLL main entry-point functions

int __stdcall DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			if (InitCVIRTE (hinstDLL, 0, 0) == 0)
				return 0;	  /* out of memory */
			break;
		case DLL_PROCESS_DETACH:
			CloseCVIRTE ();
			break;
	}
	
	return 1;
}
