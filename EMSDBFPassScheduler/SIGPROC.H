/********************************************************************
*	Module:			sigproc.h
*	Process ID:
*	S/W Platforms:
*	H/W Platforms:
*	Compiler:
*	Description: 	 Signal processing interface
*
*					 
*	Usage:
*	Entry Point:
*	Input Files:
*	Output Files:
*	Comments:
*
*********************************************************************
*	              Copyright (c) 2001 by EMS Technologies, Inc.,
*										All rights reserved
*	This program is unpublished software and contains the trade secrets
*	and confidential information of EMS Technologies, Inc.  It may not be 
* reproduced in whole or in part, in any form or by any means whatsoever 
* without the express written permission of EMS Technologies, Inc.
*
********************************************************************/

/*******************************************************************

  Revision Record

	Rev	Date			Auth				Changes
	===	====			=============	=======

	0.0	2001/09/07		RVW				Start
	0.1 2006/11/20		RVW				added Intel PP support

********************************************************************/
#ifndef INC_SIGPROC
#define INC_SIGPROC

#include "emstypes.h"

#ifndef EMS_USE_INTEL_PP
#define EMS_USE_INTEL_SPL
#endif


#if defined (EMS_USE_INTEL_SPL)
	#define nsp_UsesAll
	#include "nsp.h"

	#ifndef M_PI
	#define M_PI NSP_PI
	#endif

	#define	EMS_SPL_INV NSP_Inv
	#define EMS_SPL_FWD NSP_Forw

	typedef SCplx EMSCOMPLEX;
	typedef NSPStatus EMSSIGPROCSTATUS;

	#define emscbConj1 			nspcbConj1
	#define emssbIntToFloat	nspsbIntToFloat
	#define emscbMpy1 			nspcbMpy1
	#define emssbMpy1	    	nspsbMpy1
		#define emswbMpy1		  	nspwbMpy1
	#define emssbMpy2     		nspsbMpy2
	#define emssbMpy3 			nspsbMpy3 
	#define emscbMpy3 			nspcbMpy3 
	#define emssbAbs1 			nspsbAbs1 
	#define	emssRealFftNip 	nspsRealFftNip
	#define emscFft 			nspcFft
	#define emscFftNip 			nspcFftNip
	#define	emscbPowerSpectr 	nspcbPowerSpectr
	#define emswbZero 			nspwbZero
	#define emssbZero 			nspsbZero
	#define emscbZero 			nspcbZero
	#define emswMinExt  		nspwMinExt
	#define emswbSub1   		nspwbSub1
	#define emscbMag            nspcbMag
	#define emswMean	 			nspwMean
	#define emssMean				nspsMean
	#define	emssStdDev 			nspsStdDev
	#define	emsvbReal 			nspvbReal
	#define emsvbImag 			nspvbImag
	#define emswDotProd 	 	nspwDotProd
	#define emssDotProd			nspsDotProd
	#define emssbAdd2 			nspsbAdd2
	#define emssbSub3  			nspsbSub3
	#define emssbAdd3  			nspsbAdd3
	#define emssbInvThresh1 	nspsbInvThresh1 
	#define emssbThresh1 		nspsbThresh1
	#define emssbThresh2 		nspsbThresh2
	#define emssbLn2  			nspsbLn2 
	#define	emswbrPhase  		nspwbrPhase
	//#define emswbCopy  			nspwbCopy
	//#define emscbCopy  			nspcbCopy
	//#define emssbCopy 			nspsbCopy
	#define	emscFirInit 	 	nspcFirInit
	#define	emsFirFree 	 	nspFirFree
	#define emssbSet 			nspsbSet
	#define emscbFir 			nspcbFir
	#define emswDownSample		nspwDownSample

	#define emssMax				nspsMax
	#define emssbSub1			nspsbSub1
	#define emssbFloatToInt		nspsbFloatToInt
	#define emssbAdd1			nspsbAdd1

	#define emssCcsFftNip       nspsCcsFftNip
	#define emssMaxExt          nspsMaxExt
	#define emscbConj2          nspcbConj2
	#define emscbMpy2           nspcbMpy2
	#define emscbPhase          nspcbPhase
	#define emscDotProd			nspcDotProd
	#define emsdFirBandPass		nspdFirBandpass
#elif defined (EMS_USE_INTEL_PP)

	#include "ipp.h"

	typedef IppStatus EMSSIGPROCSTATUS;
	typedef Ipp32fc EMSCOMPLEX;
	typedef Ipp16sc EMSWCOMPLEX;

	#define EMS_SPL_FWD 1
	#define	EMS_SPL_INV 2

	typedef struct _tagNSPFirState
	{
		WORD wFirType;
		IppsFIRSpec_32fc* pSpec;
		Ipp8u* pBuf;
		Ipp32fc* pDlySrc;
		Ipp32fc* pDlyDst;
		int tapsLen;
	} NSPFirState;

	#ifndef M_PI
	#define M_PI IPP_PI
	#endif

#else
	#pragma message("*** must specify one of EMS_USE_INTEL_SPL or EMS_USE_INTEL_PP ****")
#endif

#ifndef NSP_NO_SCALE
	#define NSP_NO_SCALE 0x0
#endif // NSP_NO_SCALE

#ifndef NSP_FIXED_SCALE
	#define NSP_FIXED_SCALE 0x2
#endif // NSP_FIXED_SCALE

#ifndef NSP_AUTO_SCALE
	#define NSP_AUTO_SCALE 0x4
#endif // NSP_AUTO_SCALE

#ifndef NSP_LT
#define NSP_LT  0
#endif 

#ifndef NSP_GT
#define NSP_GT 1
#endif 


#ifndef NSP_Noflags
#define NSP_Noflags   0x0000         /* Specifies an absence of all flags       */
#endif

#ifndef NSP_Round
#define NSP_Round     0x0080       /* Specifies that floating-point values    */
#endif
                                /* should be rounded to the nearest integer*/
#ifndef NSP_TruncZero
#define NSP_TruncZero 0x0100      /* Specifies that floating-point values    */
#endif
                                /* should be truncated toward zero         */
#ifndef NSP_TruncNeg
#define NSP_TruncNeg  0x0200      /* Specifies that floating-point values    */
#endif
                                /* should be truncated toward negative     */
                                /* infinity                                */
#ifndef NSP_TruncPos
#define NSP_TruncPos  0x0400      /* Specifies that floating-point values    */
#endif
                                /* should be truncated toward positive     */
                                /* infinity                                */
#ifndef NSP_Unsigned
#define NSP_Unsigned  0x0800      /* Specifies that integer values are       */
#endif
                                /* unsigned                                */
#ifndef NSP_Clip
#define NSP_Clip      0x1000     /* Specifies that floating-point values    */
#endif
                                /* outside the allowable integer range     */
                                /* integer value                           */
#ifndef NSP_OvfErr
#define NSP_OvfErr    0x2000     /* Specifies that an overflow error should */
#endif

#define EMS_SPL_Noflags   NSP_Noflags   
#define EMS_SPL_Round     NSP_Round     
#define EMS_SPL_TruncZero NSP_TruncZero 
#define EMS_SPL_TruncNeg  NSP_TruncNeg  
#define EMS_SPL_TruncPos  NSP_TruncPos  
#define EMS_SPL_Unsigned  NSP_Unsigned  
#define EMS_SPL_Clip      NSP_Clip      
#define EMS_SPL_OvfErr    NSP_OvfErr    

#define EMS_SPL_NO_SCALE 		NSP_NO_SCALE
#define EMS_SPL_FIXED_SCALE 	NSP_FIXED_SCALE
#define EMS_SPL_AUTO_SCALE 	NSP_AUTO_SCALE

#define EMS_SPL_GT			NSP_GT
#define EMS_SPL_LT			NSP_LT

#ifdef __cplusplus

class CEMSSignalProcLib
{
protected:	
	CEMSSignalProcLib() { }
	~CEMSSignalProcLib() { }

public:
	static void Initialize( void );
	static void Release( void );

private:
	static BOOL m_bInitialized;	
};

extern "C" {

#endif 

void emssbCopy( const float  *src, float  *dst, int n );
void emscbCopy( const EMSCOMPLEX  *src, EMSCOMPLEX  *dst, int n );
void emswbCopy( const short *src, short *dst, int n );

#if defined( EMS_USE_INTEL_PP )
	void emssbZero( const float  *src, int n );
	void emscbZero( const EMSCOMPLEX  *dst, int n );
	void emswbZero( const short *dst, int n );

	void emswbSub1( const short val, short *dst, int n, int nScaleMode, int *lpnScaleFactor );
	void emssbSub1( const float val, float *dst, int n );
	void emssbSub3( const float *src, const float *val, float *dst, int n );

	void emssbAdd1( const float val, float *dst, int n );
	void emssbAdd2( const float *src, float *dst, int n );
	void emssbAdd3( const float *srcA, const float *srcB, float *dst, int n );

	void emscbMpy1( const EMSCOMPLEX val, EMSCOMPLEX *dst, int n );
	void emssbMpy1( const float val, float *dst, int n );
	void emswbMpy1( const short val, short *dst, int n, int nScaleMode, int* pnScaleFactor );

	void emssbMpy2( const float *src, float *dst, int n);
	void emssbMpy3( const float *srcA, const float *srcB, float *dst, int n);
	void emscbMpy2( const EMSCOMPLEX *src, EMSCOMPLEX *dst, int n);
	void emscbMpy3( const EMSCOMPLEX *srcA, const EMSCOMPLEX *srcB, EMSCOMPLEX *dst, int n);


	EMSCOMPLEX emscDotProd(const EMSCOMPLEX *srcA, const EMSCOMPLEX *srcB, int len);
	short emswDotProd(const short  *srcA, const short  *srcB, int len,
                                   	int nScaleMode, int *lpnScaleFactor);
	float emssDotProd(const float  *srcA, const float  *srcB, int len);

	void emssbAbs1( float *vec, int len);

	void emsvbReal(const EMSWCOMPLEX *src,short  *dst,int length);
	void emsvbImag(const EMSWCOMPLEX *src,short  *dst,int length);

	void emscbConj1( EMSCOMPLEX *vec,             int n);
	void emscbConj2( const EMSCOMPLEX *src, EMSCOMPLEX *dst, int n);


	void emssbFloatToInt(const float  *src, void *dst, int len, int wordSize, int flags);

	void emssbIntToFloat(const void *src, float  *dst, int len, int wordSize, int flags);


	float emssMax(const float *vec, int len);

	float emssMaxExt(const float  *vec, int len, int* index);
	float emssMinExt(const float  *vec, int len, int* index);


	void emscbMag(const EMSCOMPLEX *src,float *mag,int length);

	void emscbPowerSpectr(const EMSCOMPLEX *src, float *spectr,int length);

	float emssMean(const float  *vec, int len);

	short emswMean(const short  *vec, int len);

	float emssStdDev(const float *vec, int len);

	void emssbSet(float val, float *dst, int n);

	void emssbLn2(const float  *src, float  *dst, int len);

	void emswbrPhase(const short  *srcReal,const short *srcImag, short *phase, int length);

	void emscbPhase(const EMSCOMPLEX *src,float  *phase,int length);

	void emssbThresh1(float  *vec, int len, float  thresh, int relOp);
	void emssbThresh2(const float  *src,    float  *dst,int len,
                                	float   thresh, int     relOp);

	void emssbInvThresh1(float  *vec, int len, float  thresh);

	void emswDownSample(const short* src, const int srcLen,
                           	short  *dst, int* dstLen, const int factor, int* phase);


	void emscFft(EMSCOMPLEX *samps, int order, int flags);

	void emssRealFftNip(const float  *inSamps,  EMSCOMPLEX *outSamps,
                                  	int     order,    int    flags);


	void emscFftNip(const EMSCOMPLEX *inSamps,  EMSCOMPLEX *outSamps,
                              	int    order,    int     flags);

	void emscFirInit(const EMSCOMPLEX *tapVals, int          tapsLen,
                          	const EMSCOMPLEX *dlyVals, NSPFirState *statePtr);

	void emscbFir(NSPFirState *statePtr,const EMSCOMPLEX *inSamps,
                       	EMSCOMPLEX       *outSamps,      int    numIters);

	void emsFirFree(NSPFirState *statePtr );

	void emssCcsFftNip(const EMSCOMPLEX *inSamps, float  *outSamps,
                                 	int    order,   int     flags);

	void emssbMatrixInverse( const float *src, float *dst, int n );
	void emssbMatrixInverseD( const double *src, double *dst, int n );
	void emscbMatrixTranspose( const EMSCOMPLEX *src, EMSCOMPLEX *dst, int n, int m );
	void emssbMatrixTranspose( const float *src, float *dst, int n, int m );
	void emscbMatrixMuliply( const EMSCOMPLEX *src1, int m1, int n1,
	                         const EMSCOMPLEX *src2, int m2, int n2,
	                         EMSCOMPLEX *dst );
	void emsbMatrixMuliply( const float *src1, int m1, int n1,
	                        const float *src2, int m2, int n2, float *dst );

#endif

#if defined(__cplusplus)
}
#endif


#endif // INC_SIGPROC	  
