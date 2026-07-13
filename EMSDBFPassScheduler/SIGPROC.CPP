/********************************************************************
*	Module:			sigproc.cpp
*	Process ID:
*	S/W Platforms:
*	H/W Platforms:
*	Compiler:
*	Description: 	 Signal processing lib interface
*
*
*	Usage:
*	Entry Point:
*	Input Files:
*	Output Files:
*	Comments:
*
*********************************************************************
*	Copyright (c) 2001-2005 by EMS Technologies, Inc.,
*					All rights reserved
*	This program is unpublished software and contains the trade secrets
*	and confidential information of EMS Technologies, Inc.  It may not be
*	reproduced in whole or in part, in any form or by any means whatsoever
*	without the express written permission of EMS Technologies, Inc.
*
********************************************************************/

/*******************************************************************

  Revision Record

	Rev	Date			Auth			Changes
	===	====			=============	=======

	0.0	2002/05/16		RVW				Start
	0.1 2006/11/20		RVW				added Intel PP support

********************************************************************/
#include <stdio.h>

#include "sigproc.h"
#include "emsexcpt.h"
#include "LogMsgParam.h"

const int c_emsNoScaleFactor = 0;

BOOL CEMSSignalProcLib::m_bInitialized = FALSE;

// ippStsAacSfValErr was removed in oneAPI IPP 2022+
#ifndef ippStsAacSfValErr
#define ippStsAacSfValErr -164
#endif

// ippStsSingularErr was in ippm which was removed in oneAPI IPP
#ifndef ippStsSingularErr
#define ippStsSingularErr -195
#endif

#if defined (EMS_USE_INTEL_SPL)

void emssbCopy( const float  *lpfSrc, float  *lpfDest, int nCount )
{
	if ( nCount > 0 )
		nspsbCopy( lpfSrc, lpfDest, nCount );
	else
		throw CEMSException( EMS_SBCOPY_ZERO_LENGTH );
}

void emscbCopy( const EMSCOMPLEX  *lpfSrc, EMSCOMPLEX  *lpfDest, int nCount )
{
	if ( nCount > 0 )
		nspcbCopy( lpfSrc, lpfDest, nCount );
	else
		throw CEMSException( EMS_CBCOPY_ZERO_LENGTH );
}

void emswbCopy( const short *lpfSrc, short *lpfDest, int nCount )
{
	if ( nCount > 0 )
		nspwbCopy( lpfSrc, lpfDest, nCount );
	else
		throw CEMSException( EMS_WBCOPY_ZERO_LENGTH );
}

static NSPStatus __stdcall _nspErrorCallback( NSPStatus status, const char* lpcszFunc, const char *lpcszContext, const char *file, int line )
{
	if ( status == NSP_StsOk )
	{
		return status;
	}
	else
	{
		EMS_RESULT hrSubcode = EMS_UNKNOWN_ERROR;

		switch ( status )
		{
			case NSP_StsBackTrace: //     -1         /* pseudo error for back trace     */
				break;

			case NSP_StsInternal: //      -3         /* internal error (bad state)      */
				break;

			case NSP_StsNoMem: //         -4         /* out of memory                   */
				hrSubcode = EMS_NO_MEMORY;
				break;

			case NSP_StsBadArg: //        -5         /* function arg/param is bad       */
				hrSubcode = EMS_BAD_PARAM;
				break;

			case NSP_StsBadFunc: //       -6         /* unsupported function            */
				break;

			case NSP_StsNoConv: //        -7         /* iter. didn't converge           */
				break;

			case NSP_StsAutoTrace: //     -8         /* Tracing through nsptrace.h      */
				break;

			case NSP_StsError: //         -2         /* unknown /unspecified error      */
			default:
				break;
		}
		nspSetErrStatus( status );

		CEMSLogMsgParam	param;
		param.Add( lpcszFunc );
		param.Add( lpcszContext );
		throw CEMSException( __LINE__, TEXT(__FILE__), EMS_EXCEPTION_CONTEXT, param, hrSubcode );

//		TCHAR szComment[ 256 ];
//		sprintf( szComment, TEXT( "%s : %s" ), lpcszFunc, lpcszContext );
//		throw CEMSException( szComment, (EMS_RESULT)0x300, hrSubcode );
	}

}

void CEMSSignalProcLib::Initialize( void )
{
	if ( !m_bInitialized )
	{
		nspRedirectError( _nspErrorCallback );

		m_bInitialized = TRUE;
	}
}

void CEMSSignalProcLib::Release( void )
{
	if ( m_bInitialized )
	{
		nspRedirectError( 0 );

		m_bInitialized = FALSE;
	}
}

#elif defined (EMS_USE_INTEL_PP )

#include <math.h>

void _emsIppException( IppStatus retCode, int nLine, LPCTSTR lpcszFile )
{
	switch ( retCode )
	{
		case ippStsNoErr:
			break;

		default:
			throw CEMSException( nLine, lpcszFile );
	}
}

void emssbCopy( const float  *lpfSrc, float  *lpfDest, int nCount )
{
	if ( nCount > 0 )
		ippsCopy_32f( lpfSrc, lpfDest, nCount );
	else
		throw CEMSException( EMS_SBCOPY_ZERO_LENGTH );
}

void emscbCopy( const EMSCOMPLEX  *lpfSrc, EMSCOMPLEX  *lpfDest, int nCount )
{
	if ( nCount > 0 )
		ippsCopy_32fc( lpfSrc, lpfDest, nCount );
	else
		throw CEMSException( EMS_CBCOPY_ZERO_LENGTH );
}

void emswbCopy( const short *lpfSrc, short *lpfDest, int nCount )
{
	if ( nCount > 0 )
		ippsCopy_16s( lpfSrc, lpfDest, nCount );
	else
		throw CEMSException( EMS_WBCOPY_ZERO_LENGTH );
}


void emssbZero( const float  *src, int n )
{
	IppStatus retCode = ippsZero_32f( (float*)src, n );

	if ( ippStsNoErr != retCode )
		_emsIppException( retCode, __LINE__, TEXT(__FILE__) );
}

void emscbZero( const EMSCOMPLEX  *src, int n )
{
	IppStatus retCode = ippsZero_32fc( (EMSCOMPLEX*)src, n );

	if ( ippStsNoErr != retCode )
		_emsIppException( retCode, __LINE__, TEXT(__FILE__) );
}

void emswbZero( const short *src, int n )
{
	IppStatus retCode = ippsZero_16s( (short*)src, n );

	if ( ippStsNoErr != retCode )
		_emsIppException( retCode, __LINE__, TEXT(__FILE__) );
}

void CEMSSignalProcLib::Initialize( void )
{
	if ( !m_bInitialized )
	{
//		nspRedirectError( _nspErrorCallback );

		m_bInitialized = TRUE;
	}
}

void CEMSSignalProcLib::Release( void )
{
	if ( m_bInitialized )
	{
//		nspRedirectError( 0 );

		m_bInitialized = FALSE;
	}
}

void emswbSub1( const short val, short *dst, int n, int nScaleMode, int *lpnScaleFactor )
{
	IppStatus status = ippStsAacSfValErr;  // unsupported Scale Factor

	switch ( nScaleMode & 0xFFFE )
	{
		case EMS_SPL_NO_SCALE:
			status = ippsSubC_16s_I( val, dst, n );
			break;

		case EMS_SPL_FIXED_SCALE:
			status = ippsSubC_16s_ISfs( val, dst, n , *lpnScaleFactor );
			break;

		case EMS_SPL_AUTO_SCALE:
			break;

		default:
			break;
	}

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emssbSub1( const float val, float *dst, int n )
{
	IppStatus status = ippsSubC_32f_I(val, dst, n );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emssbSub3( const float *src, const float *val, float *dst, int n )
{
	IppStatus status = ippsSub_32f( src, val, dst, n );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


void emssbAdd1( const float val, float *dst, int n )
{
	IppStatus status = ippsAddC_32f_I(val, dst, n );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emssbAdd2( const float *src, float *dst, int n )
{
	IppStatus status = ippsAdd_32f_I( src, dst, n );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emssbAdd3( const float *srcA, const float *srcB, float *dst, int n )
{
	IppStatus status = ippsAdd_32f( srcA, srcB, dst, n );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


void emscbMpy1( const EMSCOMPLEX val, EMSCOMPLEX *dst, int n )
{
	IppStatus status = ippsMulC_32fc_I( val, dst, n );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


void emssbMpy1( const float val, float *dst, int n )
{
	IppStatus status = ippsMulC_32f_I( val, dst, n );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emswbMpy1( const short val, short *dst, int n, int nScaleMode, int *lpnScaleFactor )
{
	IppStatus status = ippStsAacSfValErr;  // unsupported Scale Factor

	switch ( nScaleMode & 0xFFFE )
	{
		case EMS_SPL_NO_SCALE:
			status = ippsMulC_16s_I( val, dst, n );
			break;

		case EMS_SPL_FIXED_SCALE:
			status = ippsMulC_16s_ISfs( val, dst, n , *lpnScaleFactor );
			break;

		case EMS_SPL_AUTO_SCALE:
			break;

		default:
			break;
	}

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emssbMpy2( const float *src, float *dst, int n)
{
	IppStatus status = ippsMul_32f_I( src, dst, n );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emssbMpy3( const float *srcA, const float *srcB, float *dst, int n)
{
	IppStatus status = ippsMul_32f( srcA, srcB, dst, n );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emscbMpy2( const EMSCOMPLEX *src, EMSCOMPLEX *dst, int n)
{
	IppStatus status = ippsMul_32fc_I( src, dst, n );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emscbMpy3( const EMSCOMPLEX *srcA, const EMSCOMPLEX *srcB, EMSCOMPLEX *dst, int n)
{
	IppStatus status = ippsMul_32fc( srcA, srcB, dst, n );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


EMSCOMPLEX emscDotProd(const EMSCOMPLEX *srcA, const EMSCOMPLEX *srcB, int len)
{
	EMSCOMPLEX result;
	IppStatus status = ippsDotProd_32fc( srcA, srcB, len, &result );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );

	return result;
}

short emswDotProd(const short  *srcA, const short  *srcB, int len,
                                   int nScaleMode, int *lpnScaleFactor)
{
	short wResult=0;
	Ipp32s result32 = 0;
	IppStatus status = ippStsAacSfValErr;  // unsupported Scale Factor

	switch ( nScaleMode & 0xFFFE )
	{
		case EMS_SPL_NO_SCALE:
			status = ippsDotProd_16s32s_Sfs( srcA, srcB, len, &result32, c_emsNoScaleFactor );
			wResult = (short)result32;
			break;

		case EMS_SPL_FIXED_SCALE:
			status = ippsDotProd_16s32s_Sfs( srcA, srcB, len, &result32, *lpnScaleFactor );
			wResult = (short)result32;
			break;

		case EMS_SPL_AUTO_SCALE:
			break;

		default:
			break;
	}

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );

	return wResult;
}

float emssDotProd(const float  *srcA, const float  *srcB, int len)
{
	float fResult = 0.0;

	IppStatus status = ippsDotProd_32f( srcA, srcB, len, &fResult );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );

	return fResult;
}

void emssbAbs1( float *vec, int len)
{
	IppStatus status = ippsAbs_32f_I( vec, len );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


void emsvbReal(const EMSWCOMPLEX *src,short  *dst,int length)
{
	IppStatus status = ippsReal_16sc( src, dst, length );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emsvbImag(const EMSWCOMPLEX *src,short  *dst,int length)
{
	IppStatus status = ippsImag_16sc( src, dst, length );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


void emscbConj1( EMSCOMPLEX *vec, int n)
{
	IppStatus status = ippsConj_32fc_I( vec, n);

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emscbConj2( const EMSCOMPLEX *src, EMSCOMPLEX *dst, int n)
{
	IppStatus status = ippsConj_32fc( src, dst, n );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


void emssbFloatToInt(const float  *src, void *dst, int len, int wordSize, int flags)
{
	IppStatus status = ippStsNotSupportedModeErr;
	IppRoundMode rndMode = ( flags & EMS_SPL_TruncZero ) ? ippRndZero : ippRndNear;
	bool bUnsigned = EMS_SPL_Unsigned == ( flags & EMS_SPL_Unsigned );

	switch ( wordSize )
	{
		case 8:
			if ( bUnsigned )
				status = ippsConvert_32f8u_Sfs( src, (unsigned char*)dst, len, rndMode, c_emsNoScaleFactor );
			else
				status = ippsConvert_32f8s_Sfs( src, (signed char *)dst, len, rndMode, c_emsNoScaleFactor );
			break;

		case 16:
			if ( bUnsigned )
				status = ippsConvert_32f16u_Sfs( src, (unsigned short*)dst, len, rndMode, c_emsNoScaleFactor );
			else
				status = ippsConvert_32f16s_Sfs( src, (short *)dst, len, rndMode, c_emsNoScaleFactor );
			break;

		case 32:
			status = ippsConvert_32f32s_Sfs( src, (int *)dst, len, rndMode, c_emsNoScaleFactor );
			break;

		default:
			break;
	}


	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


void emssbIntToFloat(const void *src, float  *dst, int len, int wordSize, int flags)
{
	IppStatus status = ippStsNotSupportedModeErr;
	bool bUnsigned = EMS_SPL_Unsigned == ( flags & EMS_SPL_Unsigned );

	switch ( wordSize )
	{
		case 8:
			if ( bUnsigned )
				status = ippsConvert_8u32f( (unsigned char *)src, dst, len );
			else
				status = ippsConvert_8s32f( (signed char *)src, dst, len );
			break;

		case 16:
			if ( bUnsigned )
				status = ippsConvert_16u32f( (unsigned short *)src, dst, len );
			else
				status = ippsConvert_16s32f( (short *)src, dst, len ) ;
			break;

		case 32:
			status = ippsConvert_32s32f( (int*)src, dst, len );
			break;

		default:
			break;
	}


	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


float emssMax(const float *vec, int len)
{
	float fResult = 0.0;
	IppStatus status = ippsMax_32f( vec, len, &fResult );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );

	return fResult;
}


float emssMaxExt(const float  *vec, int len, int* index)
{
	float fResult = 0.0;
	IppStatus status = ippsMaxIndx_32f( vec, len, &fResult, index );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );

	return fResult;
}


float emssMinExt(const float  *vec, int len, int* index)

{
	float fResult = 0.0;
	IppStatus status = ippsMinIndx_32f( vec, len, &fResult, index );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );

	return fResult;
}

void emscbMag(const EMSCOMPLEX *src,float *mag,int length)
{
	IppStatus status = ippsMagnitude_32fc( src, mag, length );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


void emscbPowerSpectr(const EMSCOMPLEX *src, float *spectr,int length)
{
	IppStatus status = ippsPowerSpectr_32fc( src, spectr, length );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


float emssMean(const float *vec, int len)
{
	float fResult = 0.0;
	IppStatus status = ippsMean_32f( vec, len, &fResult, ippAlgHintNone );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );

	return fResult;
}


short emswMean(const short *vec, int len)
{
	short nResult = 0;
	IppStatus status = ippsMean_16s_Sfs( vec, len, &nResult, c_emsNoScaleFactor );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );

	return nResult;
}

float emssStdDev(const float *vec, int len)
{
	float fResult = 0.0;
	IppStatus status = ippsStdDev_32f( vec, len, &fResult, ippAlgHintNone );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );

	return fResult;
}

void emssbSet(float val, float *dst, int n)
{
	IppStatus status = ippsSet_32f( val, dst, n );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emssbLn2(const float  *src, float  *dst, int len)
{
	IppStatus status = ippsLn_32f( src, dst, len );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emswbrPhase(const short  *srcReal,const short *srcImag, short *phase, int length)
{
	IppStatus status = ippsPhase_16s_Sfs( srcReal, srcImag, phase, length, c_emsNoScaleFactor );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


void emscbPhase(const EMSCOMPLEX *src,float  *phase,int length)
{
	IppStatus status = ippsPhase_32fc( src, phase, length );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


void emssbThresh1(float  *vec, int len, float  thresh, int relOp)
{
	IppStatus status =  ippStsNotSupportedModeErr;

	switch ( relOp )
	{
	case EMS_SPL_GT:
		status = ippsThreshold_GT_32f_I( vec, len, thresh );
		break;

	case EMS_SPL_LT:
		status = ippsThreshold_LT_32f_I( vec, len, thresh );
		break;

	default:
		break;
	}

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}

void emssbThresh2(const float  *src,    float  *dst,int len,
                                float   thresh, int     relOp)
{
		IppStatus status =  ippStsNotSupportedModeErr;

	switch ( relOp )
	{
	case EMS_SPL_GT:
		status = ippsThreshold_GT_32f( src, dst, len, thresh );
		break;

	case EMS_SPL_LT:
		status = ippsThreshold_LT_32f( src, dst, len, thresh );
		break;

	default:
		break;
	}

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


void emssbInvThresh1(float  *vec, int len, float  thresh)
{
	IppStatus status = ippsThreshold_LTInv_32f_I( vec, len, thresh );

	if ( ippStsNoErr != status )
		_emsIppException( status, __LINE__, TEXT(__FILE__) );
}


void emscFirInit(const EMSCOMPLEX *tapVals, int          tapsLen,
                          const EMSCOMPLEX *dlyVals, NSPFirState *statePtr)
{
	if ( statePtr )
	{
		memset( statePtr, 0, sizeof( NSPFirState ) );

		int specSize = 0, bufSize = 0;
		IppStatus status = ippsFIRSRGetSize( tapsLen, ipp32fc, &specSize, &bufSize );

		if ( ippStsNoErr == status )
		{
			statePtr->pSpec = (IppsFIRSpec_32fc*)ippsMalloc_8u( specSize );
			statePtr->pBuf = ippsMalloc_8u( bufSize );
			statePtr->tapsLen = tapsLen;

			// Allocate and initialize delay line buffers
			int dlyLen = tapsLen - 1;
			statePtr->pDlySrc = ippsMalloc_32fc( dlyLen > 0 ? dlyLen : 1 );
			statePtr->pDlyDst = ippsMalloc_32fc( dlyLen > 0 ? dlyLen : 1 );

			if ( dlyVals && dlyLen > 0 )
				ippsCopy_32fc( dlyVals, statePtr->pDlySrc, dlyLen );
			else if ( dlyLen > 0 )
				ippsZero_32fc( statePtr->pDlySrc, dlyLen );

			if ( dlyLen > 0 )
				ippsZero_32fc( statePtr->pDlyDst, dlyLen );

			status = ippsFIRSRInit_32fc( tapVals, tapsLen, ippAlgDirect, statePtr->pSpec );

			if ( ippStsNoErr == status )
				statePtr->wFirType = 0x32FC;
			else
				_emsIppException( status, __LINE__, TEXT(__FILE__) );
		}
		else
			_emsIppException( status, __LINE__, TEXT(__FILE__) );
	}
	else
		_emsIppException( ippStsNotSupportedModeErr, __LINE__, TEXT(__FILE__) );
}


void emscbFir(NSPFirState *statePtr,const EMSCOMPLEX *inSamps,
                       EMSCOMPLEX *outSamps, int numIters)
{
	if ( statePtr && ( 0x32FC == statePtr->wFirType ) )
	{
		IppStatus status = ippsFIRSR_32fc( inSamps, outSamps, numIters,
			statePtr->pSpec, statePtr->pDlySrc, statePtr->pDlyDst, statePtr->pBuf );

		// Carry delay line forward for next call
		if ( statePtr->tapsLen > 1 )
			ippsCopy_32fc( statePtr->pDlyDst, statePtr->pDlySrc, statePtr->tapsLen - 1 );

		if ( ippStsNoErr != status )
			_emsIppException( status, __LINE__, TEXT(__FILE__) );
	}
	else
		_emsIppException( ippStsNotSupportedModeErr, __LINE__, TEXT(__FILE__) );
}

void emsFirFree(NSPFirState *statePtr )
{
	if ( statePtr )
	{
		switch ( statePtr->wFirType )
		{
			case 0x32FC:
				if ( statePtr->pSpec )   ippsFree( statePtr->pSpec );
				if ( statePtr->pBuf )    ippsFree( statePtr->pBuf );
				if ( statePtr->pDlySrc ) ippsFree( statePtr->pDlySrc );
				if ( statePtr->pDlyDst ) ippsFree( statePtr->pDlyDst );
				statePtr->pSpec = 0;
				statePtr->pBuf = 0;
				statePtr->pDlySrc = 0;
				statePtr->pDlyDst = 0;
				break;
		}
		statePtr->wFirType = 0;
	}
}

void emssCcsFftNip(const EMSCOMPLEX *inSamps, float  *outSamps,
                                 int    order,   int     flags)
{
	// ****** IMPLEMENT ********
	// ****** IMPLEMENT ********
	// ****** IMPLEMENT ********

	_emsIppException( ippStsNotSupportedModeErr, __LINE__, TEXT(__FILE__) );
}


void emswDownSample(const short* src, const int srcLen,
                           short  *dst, int* dstLen, const int factor, int* phase)
{

	// ****** IMPLEMENT ********
	// ****** IMPLEMENT ********
	// ****** IMPLEMENT ********

	_emsIppException( ippStsNotSupportedModeErr, __LINE__, TEXT(__FILE__) );
}


void emscFft(EMSCOMPLEX *samps, int order, int flags)
{
	IppStatus status = ippStsNoErr;

	int nFlags = IPP_FFT_NODIV_BY_ANY;

	if( EMS_SPL_FWD == flags )
	{
//		nFlags = IPP_FFT_DIV_FWD_BY_N;
	}
	else if( EMS_SPL_INV == flags )
	{
//		nFlags = IPP_FFT_DIV_FWD_BY_N;
	}

	// Get buffer sizes for FFT spec
	int specSize = 0, specBufSize = 0, bufSize = 0;
	status = ippsFFTGetSize_C_32fc( order, nFlags, ippAlgHintNone, &specSize, &specBufSize, &bufSize );

	if ( ippStsNoErr == status )
	{
		Ipp8u* pSpecBuf = ippsMalloc_8u( specSize );
		Ipp8u* pInitBuf = ( specBufSize > 0 ) ? ippsMalloc_8u( specBufSize ) : NULL;
		Ipp8u* pWorkBuf = ( bufSize > 0 ) ? ippsMalloc_8u( bufSize ) : NULL;

		IppsFFTSpec_C_32fc *pSpec = 0;
		status = ippsFFTInit_C_32fc( &pSpec, order, nFlags, ippAlgHintNone, pSpecBuf, pInitBuf );

		if ( ippStsNoErr == status )
		{
			if( EMS_SPL_FWD == flags )
			{
				status = ippsFFTFwd_CToC_32fc_I(samps, pSpec, pWorkBuf);
			}
			else if( EMS_SPL_INV == flags )
			{
				status = ippsFFTInv_CToC_32fc_I(samps, pSpec, pWorkBuf);
			}
			else
			{
				_emsIppException( ippStsNotSupportedModeErr, __LINE__, TEXT(__FILE__) );
			}

			if ( ippStsNoErr != status )
				_emsIppException( status, __LINE__, TEXT(__FILE__) );
		}

		if ( pWorkBuf ) ippsFree( pWorkBuf );
		if ( pInitBuf ) ippsFree( pInitBuf );
		if ( pSpecBuf ) ippsFree( pSpecBuf );
	}
}


void emssRealFftNip(const float  *inSamps,  EMSCOMPLEX *outSamps,
                                  int     order,    int    flags)
{
	IppStatus status = ippStsNoErr;

	int nFlags = IPP_FFT_NODIV_BY_ANY;

	if( EMS_SPL_FWD == flags )
	{
//		nFlags = IPP_FFT_DIV_FWD_BY_N;
	}
	else if( EMS_SPL_INV == flags )
	{
//		nFlags = IPP_FFT_DIV_INV_BY_N;
	}

	// Get buffer sizes for FFT spec
	int specSize = 0, specBufSize = 0, bufSize = 0;
	status = ippsFFTGetSize_R_32f( order, nFlags, ippAlgHintNone, &specSize, &specBufSize, &bufSize );

	if ( ippStsNoErr == status )
	{
		Ipp8u* pSpecBuf = ippsMalloc_8u( specSize );
		Ipp8u* pInitBuf = ( specBufSize > 0 ) ? ippsMalloc_8u( specBufSize ) : NULL;
		Ipp8u* pWorkBuf = ( bufSize > 0 ) ? ippsMalloc_8u( bufSize ) : NULL;

		IppsFFTSpec_R_32f *pSpec = NULL;
		status = ippsFFTInit_R_32f( &pSpec, order, nFlags, ippAlgHintNone, pSpecBuf, pInitBuf );

		if ( ippStsNoErr == status )
		{
			if( EMS_SPL_FWD == flags )
			{
				status = ippsFFTFwd_RToCCS_32f( inSamps, (Ipp32f*) outSamps, pSpec, pWorkBuf );
			}
			else if( EMS_SPL_INV == flags )
			{
				status = ippsFFTInv_CCSToR_32f( inSamps, (Ipp32f*) outSamps, pSpec, pWorkBuf );
			}
			else
			{
				_emsIppException( ippStsNotSupportedModeErr, __LINE__, TEXT(__FILE__) );
			}

			if ( ippStsNoErr != status )
				_emsIppException( status, __LINE__, TEXT(__FILE__) );
		}

		if ( pWorkBuf ) ippsFree( pWorkBuf );
		if ( pInitBuf ) ippsFree( pInitBuf );
		if ( pSpecBuf ) ippsFree( pSpecBuf );
	}
}

void emscFftNip(const EMSCOMPLEX *inSamps,  EMSCOMPLEX *outSamps,
                              int    order,    int     flags)
{
	IppStatus status = ippStsNoErr;

	int nFlags = IPP_FFT_NODIV_BY_ANY;

	if( EMS_SPL_FWD == flags )
	{
//		nFlags = IPP_FFT_DIV_FWD_BY_N;
	}
	else if( EMS_SPL_INV == flags )
	{
		//nFlags = IPP_FFT_DIV_FWD_BY_N;
//		nFlags = IPP_FFT_DIV_BY_SQRTN;
	}


	// Get buffer sizes for FFT spec
	int specSize = 0, specBufSize = 0, bufSize = 0;
	status = ippsFFTGetSize_C_32fc( order, nFlags, ippAlgHintNone, &specSize, &specBufSize, &bufSize );

	if ( ippStsNoErr == status )
	{
		Ipp8u* pSpecBuf = ippsMalloc_8u( specSize );
		Ipp8u* pInitBuf = ( specBufSize > 0 ) ? ippsMalloc_8u( specBufSize ) : NULL;
		Ipp8u* pWorkBuf = ( bufSize > 0 ) ? ippsMalloc_8u( bufSize ) : NULL;

		IppsFFTSpec_C_32fc *pSpec = 0;
		status = ippsFFTInit_C_32fc( &pSpec, order, nFlags, ippAlgHintNone, pSpecBuf, pInitBuf );

		if ( ippStsNoErr == status )
		{
			if( EMS_SPL_FWD == flags )
			{
				status = ippsFFTFwd_CToC_32fc( inSamps, outSamps, pSpec, pWorkBuf );
			}
			else if( EMS_SPL_INV == flags )
			{
				status = ippsFFTInv_CToC_32fc( inSamps, outSamps, pSpec, pWorkBuf );
			}
			else
			{
				_emsIppException( ippStsNotSupportedModeErr, __LINE__, TEXT(__FILE__) );
			}

			if ( ippStsNoErr != status )
				_emsIppException( status, __LINE__, TEXT(__FILE__) );
		}

		if ( pWorkBuf ) ippsFree( pWorkBuf );
		if ( pInitBuf ) ippsFree( pInitBuf );
		if ( pSpecBuf ) ippsFree( pSpecBuf );
	}
}


#endif
