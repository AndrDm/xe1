//==============================================================================
//
// Title:		MP Helper
// Purpose:		Helper DLL for Multiscale Pyramids.
//
// Created on:	30.04.2023 at 18:08:18 by AD.
//
//==============================================================================

#include "MP Helper.h"

inline double fastPow(double a, double b) {
    union {
        double d;
        int x[2];
    } u = { a };
    u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
    u.x[0] = 0;
    return u.d;
}

void ApplyTransform(NIImageHandle SrcImage, int StartLine, int EndLine, double Divider, double Power, double Multiplier,  LVErrorCluster *ErrorCluster)
{
	Image *ImgSrc;
	float *LVImagePtrSrcSGL;
 	int LVWidth, LVHeight,	LVLineWidthSrc, x, y;

	CHECK_ERROR_IN(ErrorCluster);
	LV_IS_NOT_IMAGE(SrcImage, ErrorCluster);

	//ImgSrc = ADV_LVDTToAddress(SrcImage);
	LV_SetThreadCore(1); //must be called prior to LV_LVDTToGRImage
	LV_LVDTToGRImage(SrcImage, &ImgSrc);
	
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

	register double temp0, temp1, temp2;
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

void ApplyPower(NIImageHandle SrcImage, double Power, LVErrorCluster *ErrorCluster)
{
	Image *ImgSrc;
	float *LVImagePtrSrcSGL;
 	int LVWidth, LVHeight,	LVLineWidthSrc, x, y;

	CHECK_ERROR_IN(ErrorCluster);
	LV_IS_NOT_IMAGE(SrcImage, ErrorCluster);

	//ImgSrc = ADV_LVDTToAddress(SrcImage);
	LV_SetThreadCore(1); //must be called prior to LV_LVDTToGRImage
	LV_LVDTToGRImage(SrcImage, &ImgSrc);
	
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
