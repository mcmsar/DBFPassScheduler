/********************************************************************
*	Module:			DBFNullPointing.cpp 
*	Process ID:
*	S/W Platforms:
*	H/W Platforms:
*	Compiler:
*	Description: 	
*						
*
*	Usage:
*	Entry Point:
*	Input Files:
*	Output Files:
*	Comments:	
*
*
*********************************************************************
*	              Copyright (c) 2011 by EMS Technologies, Inc.,
*										All rights reserved
*	This program is unpublished software and contains the trade secrets
*	and confidential information of EMS Technologies, Inc.  It may not be 
* reproduced in whole or in part, in any form or by any means whatsoever 
* without the express written permission of EMS Technologies, Inc.
*
********************************************************************/

/*******************************************************************

  Revision Record

	Rev	Date			Auth	Changes
	===	====			====	=======

	0.0	2011/01/02		rcr		start

********************************************************************/
#include "sigproc.h"
#include "DBFNullPointing.h"
#include "emsDBFtypes2.h"
#include "emsconst.h"

/********************************************************************/


CDBFNullPointing::CDBFNullPointing() : m_ulChannels(0), m_ulSatellites(0)
{
	m_bInitialized = FALSE;
}

CDBFNullPointing::CDBFNullPointing( const CDBFNullPointing& x ) : m_ulChannels( x.m_ulChannels ),
							m_ulSatellites(x.m_ulSatellites)
{
	m_bInitialized = FALSE;
}

CDBFNullPointing::~CDBFNullPointing( void )
{
	m_ulChannels = 0;
	m_ulSatellites = 0;
		
	_Reset();

}

//---------------------------------------------------------------------------

void 
CDBFNullPointing::_Reset( )
{

	if ( m_bInitialized )
	{
		m_bInitialized = FALSE;
	}
	return;
}

//---------------------------------------------------------------------------

EMS_RESULT
CDBFNullPointing::Initialize()
{
	EMS_RESULT hr = EMS_UNKNOWN_ERROR;

	m_ulChannels   = DBF_MAX_CHANNELS;
	m_ulSatellites = DBF_MAX_SATELLITES;

	m_bInitialized = true;
			
	hr = EMS_OK;

	return hr;
}

//---------------------------------------------------------------------------
EMS_RESULT
CDBFNullPointing:: GetNullPhase( EMSDBFAZEL* aAzEls, ULONG* pulActual )
{
	EMS_RESULT hr = EMS_OK;
	ULONG ulSize0 = 0;
	ULONG ulSize1 = 0;

	if( !pulActual )
	{
		hr = EMS_INVALID_PTR;
	}
	else
	{
		ulSize0 = *pulActual;
		if ( ulSize0 > 9 ) ulSize0 = 9;
		if ( ulSize0 > 1 ) ulSize1 = ulSize0 - 1;
		*pulActual = ulSize0;
	}

	if( (EMS_OK == hr) && (ulSize1 > 0) )
	{
	
		EMSCOMPLEX cValue;

		ULONG i, j, k;

		EMSDBFAZEL*	acAngles = NULL;

		acAngles = new EMSDBFAZEL[ulSize0];
		memset( acAngles, 0, ulSize0 * sizeof( EMSDBFAZEL ) );

		EMSCOMPLEX cV[DBF_MAX_CHANNELS];
		EMSCOMPLEX cA[DBF_MAX_SATELLITES * DBF_MAX_CHANNELS];
		EMSCOMPLEX cAh[DBF_MAX_SATELLITES * DBF_MAX_CHANNELS];
		EMSCOMPLEX cAAh[DBF_MAX_SATELLITES * DBF_MAX_SATELLITES];
		EMSCOMPLEX cInvAAh[DBF_MAX_SATELLITES * DBF_MAX_SATELLITES];
		EMSCOMPLEX cInvAAhA[DBF_MAX_SATELLITES * DBF_MAX_CHANNELS];
		EMSCOMPLEX cP[DBF_MAX_CHANNELS * DBF_MAX_CHANNELS];
		EMSCOMPLEX cPh[DBF_MAX_CHANNELS * DBF_MAX_CHANNELS];
		EMSCOMPLEX cI[DBF_MAX_CHANNELS * DBF_MAX_CHANNELS];

		EMSCOMPLEX cTest[DBF_MAX_CHANNELS];
		EMSCOMPLEX cTest1[DBF_MAX_CHANNELS];
		double dNorm;
		//double dTest[DBF_MAX_SATELLITES * DBF_MAX_SATELLITES];

		memset( cV, 0, DBF_MAX_CHANNELS * sizeof( EMSCOMPLEX) );
		memset( cA, 0, DBF_MAX_SATELLITES * DBF_MAX_CHANNELS * sizeof( EMSCOMPLEX) );
		memset( cAh, 0, DBF_MAX_SATELLITES * DBF_MAX_CHANNELS * sizeof( EMSCOMPLEX) );
		memset( cAAh, 0, DBF_MAX_SATELLITES * DBF_MAX_SATELLITES * sizeof( EMSCOMPLEX) );
		memset( cInvAAh, 0, DBF_MAX_SATELLITES * DBF_MAX_SATELLITES * sizeof( EMSCOMPLEX) );
		memset( cInvAAhA, 0, DBF_MAX_SATELLITES * DBF_MAX_CHANNELS * sizeof( EMSCOMPLEX) );
		memset( cP, 0, DBF_MAX_CHANNELS * DBF_MAX_CHANNELS * sizeof( EMSCOMPLEX) );
		memset( cPh, 0, DBF_MAX_CHANNELS * DBF_MAX_CHANNELS * sizeof( EMSCOMPLEX) );
		memset( cI, 0, DBF_MAX_CHANNELS * DBF_MAX_CHANNELS * sizeof( EMSCOMPLEX) );

		memset( cTest, 0, DBF_MAX_CHANNELS * sizeof( EMSCOMPLEX) );
		memset( cTest1, 0, DBF_MAX_CHANNELS * sizeof( EMSCOMPLEX) );
		//memset( dTest, 0, DBF_MAX_SATELLITES * DBF_MAX_SATELLITES * sizeof( double) );


		ULONG ulActualChannels;

		// RR - nulling reduction factor
		double dNullfactor = 1.0;

		for ( i = 0; i < ulSize0; i++ )
		{
			
			ulActualChannels = m_ulChannels;
			k = 0;

			for ( j = 0; j < m_ulChannels; j++ )
			{
				if (!_MissingChannel( aAzEls[i], j ) )
				{
					cV[k].re = aAzEls[i].acmplxPhase[j].re;
					cV[k].im = aAzEls[i].acmplxPhase[j].im;
					k++;
				}
				else
				{
					ulActualChannels--;
				}

			}

			// Compute A matrix of null pointing vectors
			// A is an (#Channels) x (#Satellites-1) matrix
			
			hr = _GetNullComplexMatrix( aAzEls, m_ulChannels, ulSize0, cA, i );
			
//			// Compute conjugate matrix A'
			emscbMatrixTranspose( cA, cAh, ulActualChannels, ulSize1 );
			emscbConj1( cAh, ulActualChannels * ulSize1 );

			// Compute AhA

			// Note: The output matrix AhA is Hermitian :  AhA = conj(transpose AhA)
			//emscbMatrixMuliply( cAh, ulSize1, ulActualChannels, cA, ulActualChannels, ulSize1, cAAh );
			emscbMatrixMuliply( cA, ulSize1, ulActualChannels, cAh, ulActualChannels, ulSize1, cAAh );

		
			// Compute projection matrix
			// P = I - A * inverse( A' * A) * A'

			hr = _ComplexMatrixInverse( cAAh, cInvAAh, ulSize1 );

			// Test Matrix Inverse
			emscbMatrixMuliply( cInvAAh, ulSize1, ulSize1, cAAh, ulSize1, ulSize1, cI );
			

			//emscbMatrixMuliply( cInvAAh, ulSize1, ulSize1, cAh, ulSize1, ulActualChannels, cInvAAhA );
			emscbMatrixMuliply( cAh, ulActualChannels, ulSize1, cInvAAh, ulSize1, ulSize1, cInvAAhA );

			//emscbMatrixMuliply( cA, ulActualChannels, ulSize1, cInvAAhA, ulSize1, ulActualChannels, cP ); 
			emscbMatrixMuliply( cInvAAhA, ulActualChannels, ulSize1, cA, ulSize1, ulActualChannels, cP ); 

			for ( j = 0; j < ulActualChannels * ulActualChannels ; j++ )
			{
				cP[j].re *= -dNullfactor;
				cP[j].im *= -dNullfactor;

				if ( j % (ulActualChannels+1) == 0)
				{
					cP[j].re += 1.0;
				}
			}

			emscbMatrixTranspose( cP, cPh, ulActualChannels, ulActualChannels );

			//emscbMatrixMuliply( cP, ulActualChannels, ulActualChannels, cV, ulActualChannels, 1, cTest ); 
			emscbMatrixMuliply( cV, 1, ulActualChannels, cPh, ulActualChannels, ulActualChannels, cTest ); 
			

			
			//printf("%d", i);
			//for ( j = 0; j < ulActualChannels; j++ )
			//{
			//	//dNorm = sqrt( cTest[j].re * cTest[j].re + cTest[j].im * cTest[j].im );
			//	//if ( dNorm < 1e-10 ) dNorm = 1e-10;
			//	//cTest[j].re /= dNorm;
			//	//cTest[j].im /= dNorm;

			//	printf(", %f, %f", cV[j].re, cV[j].im );
			//}
			//printf("\n");

			// Restore actual channel data
			for ( j = 0, k = 0; j < m_ulChannels; j++ )
			{
				if ( !_MissingChannel( aAzEls[i], j ) )
				{
					acAngles[i].acmplxPhase[j].re = cTest[k].re;
					acAngles[i].acmplxPhase[j].im = cTest[k].im;
					k++;
				}
			}

		}

		// Test output
		int ii = 0;
		for ( i = 0; i < ulSize0; i++ )
		{
			for ( int k = 0; k < m_ulChannels; k++ )
			{
				cTest1[k].re = acAngles[i].acmplxPhase[k].re;
				cTest1[k].im = acAngles[i].acmplxPhase[k].im;
			}
			
			for ( j = 0; j < ulSize0; j++ )
			{
				cValue = emscDotProd( cTest1, acAngles[j].acmplxPhase, m_ulChannels);
				//dTest[ii++] = sqrt(cValue.re * cValue.re + cValue.im * cValue.im);
			}
			for ( j = 0; j < m_ulChannels; j++ )
			{
				aAzEls[i].acmplxPhase[j].re = acAngles[i].acmplxPhase[j].re;
				aAzEls[i].acmplxPhase[j].im = acAngles[i].acmplxPhase[j].im;
			}
		}

		if ( acAngles )
		{
			delete  [] acAngles;
			acAngles = NULL;
		}

	}


	return hr;
}


//---------------------------------------------------------------------------


EMS_RESULT
CDBFNullPointing::_GetNullComplexMatrix( EMSDBFAZEL* aAzEls, ULONG ulChannels, ULONG ulSatellites,
										EMSCOMPLEX* acMatrix, ULONG ulIndex )
{
	EMS_RESULT hr = EMS_OK;
	ULONG l = 0;

	for ( ULONG i = 0; i < ulSatellites; i++ )
	{
		if ( !( i == ulIndex ) )
		{
			for ( ULONG j = 0; j < ulChannels; j++ )
			{
				if (!_MissingChannel( aAzEls[i], j ) )
				{
					acMatrix[l++] = aAzEls[i].acmplxPhase[j];
				}
			}
		}
	}

	return hr;
}

//---------------------------------------------------------------------------

bool
CDBFNullPointing::_MissingChannel( EMSDBFAZEL aAzEls1, ULONG ulChannel )
{
	bool bOK = false;

	double dNorm = sqrt( aAzEls1.acmplxPhase[ulChannel].re * aAzEls1.acmplxPhase[ulChannel].re
					   + aAzEls1.acmplxPhase[ulChannel].im * aAzEls1.acmplxPhase[ulChannel].im );
	if ( dNorm < 1e-10 ) bOK = true;
	//if ( ulChannel>5 ) bOK = true;
	return bOK;
}
//---------------------------------------------------------------------------
EMSCOMPLEX
CDBFNullPointing::_CDivide ( EMSCOMPLEX cData)
{
	EMSCOMPLEX cOutput;
	double dMagnitude = cData.re * cData.re + cData.im * cData.im;
	cOutput.re = cData.re/dMagnitude;
	cOutput.im = -cData.im/dMagnitude;
	return cOutput;
}
EMSCOMPLEX
CDBFNullPointing::_CMultiply ( EMSCOMPLEX cData1, EMSCOMPLEX cData2 )
{
	EMSCOMPLEX cOutput;
	cOutput.re = cData1.re * cData2.re - cData1.im * cData2.im;
	cOutput.im = cData1.re * cData2.im + cData1.im * cData2.re;;
	return cOutput;
}
EMSCOMPLEX
CDBFNullPointing::_CAdd ( EMSCOMPLEX cData1, EMSCOMPLEX cData2 )
{
	EMSCOMPLEX cOutput;
	cOutput.re = cData1.re + cData2.re ;
	cOutput.im = cData1.im + cData2.im;
	return cOutput;
}
EMSCOMPLEX
CDBFNullPointing::_CSub ( EMSCOMPLEX cData1, EMSCOMPLEX cData2 )
{
	EMSCOMPLEX cOutput;
	cOutput.re = cData1.re - cData2.re ;
	cOutput.im = cData1.im - cData2.im;
	return cOutput;
}
EMS_RESULT
CDBFNullPointing::_ComplexMatrix2x2Inverse( EMSCOMPLEX* acMatrix, EMSCOMPLEX* acMatrixInverse )
{
	EMS_RESULT hr = EMS_OK;
	EMSCOMPLEX cDATA1 = _CMultiply (  acMatrix[0],  acMatrix[3] );
	EMSCOMPLEX cDATA2 = _CMultiply (  acMatrix[1],  acMatrix[2] );
	cDATA1 = _CSub( cDATA1, cDATA2 );
	cDATA1 = _CDivide( cDATA1 );
	cDATA2.re = -cDATA1.re;
	cDATA2.im = -cDATA1.im;
	acMatrixInverse[0] = _CMultiply(acMatrix[3], cDATA1 );
	acMatrixInverse[1] = _CMultiply(acMatrix[1], cDATA2 );
	acMatrixInverse[2] = _CMultiply(acMatrix[2], cDATA2 );
	acMatrixInverse[3] = _CMultiply(acMatrix[0], cDATA1 );
	return hr;
}

EMS_RESULT
CDBFNullPointing::_CMultipy2x2( EMSCOMPLEX* cM1, EMSCOMPLEX* cM2, EMSCOMPLEX* cM3)
{
	EMS_RESULT hr = EMS_OK;
	memset( cM3, 0, 4*sizeof(EMSCOMPLEX) );

	cM3[0] = _CAdd( _CMultiply( cM1[0], cM2[0]) , _CMultiply( cM1[1], cM2[2]) );
	cM3[1] = _CAdd( _CMultiply( cM1[0], cM2[1]) , _CMultiply( cM1[1], cM2[3]) );
	cM3[2] = _CAdd( _CMultiply( cM1[2], cM2[0]) , _CMultiply( cM1[3], cM2[2]) );
	cM3[3] = _CAdd( _CMultiply( cM1[2], cM2[1]) , _CMultiply( cM1[3], cM2[3]) );

	return hr;
}


EMS_RESULT
CDBFNullPointing::_ComplexMatrix4x4Inverse( EMSCOMPLEX* acMatrix, EMSCOMPLEX* acMatrixInverse )
{

//
//  M = [ A, B ; C, D]

//  inv(M) = [ W+ZX, -Z; -YX, Y ]

//  W = inv(A)
//  X = CW
//  Y = inv(D-XB)
//  Z = WBY

	EMS_RESULT hr = EMS_OK;
	EMSCOMPLEX cA[4] = { acMatrix[0], acMatrix[1], acMatrix[4], acMatrix[5] };
	EMSCOMPLEX cB[4] = { acMatrix[2], acMatrix[3], acMatrix[6], acMatrix[7] };
	EMSCOMPLEX cC[4] = { acMatrix[8], acMatrix[9], acMatrix[12],acMatrix[13] };
	EMSCOMPLEX cD[4] = { acMatrix[10],acMatrix[11],acMatrix[14],acMatrix[15] };

	EMSCOMPLEX cW[4];
	EMSCOMPLEX cX[4];
	EMSCOMPLEX cY[4];
	EMSCOMPLEX cZ[4];
	EMSCOMPLEX cTemp[4];
	EMSCOMPLEX cOut[16];
	EMSCOMPLEX cMinus;

	int iIndex[16] = { 0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15};
	cMinus.re = -1.0;
	cMinus.im = 0.0;

	hr = _ComplexMatrix2x2Inverse( cA, cW );
	hr = _CMultipy2x2( cC, cW, cX);
	hr = _CMultipy2x2( cX, cB, cTemp);

	for( int i = 0; i < 4; i++ )
	{
		cTemp[i] = _CSub ( cD[i] ,  cTemp[i] );
	}

	hr = _ComplexMatrix2x2Inverse( cTemp, cY );

	hr = _CMultipy2x2( cB, cY, cTemp);
	hr = _CMultipy2x2( cW, cTemp, cZ);

	hr = _CMultipy2x2( cZ, cX, cTemp);

	for( int i = 0; i < 4; i++ )
	{
		cOut[i] = _CAdd ( cW[i] ,  cTemp[i] );
	}

	for( int i = 4; i < 8; i++ )
	{
		cOut[i] = _CMultiply( cMinus, cZ[i-4] );
	}

	hr = _CMultipy2x2( cY, cX, cTemp );

	for( int i = 8; i < 12; i++ )
	{
		cOut[i] = _CMultiply( cMinus, cTemp[i-8]);
	}

	for( int i = 12; i < 16; i++ )
	{
		cOut[i] = cY[i-12];
	}

	for( int i = 0; i < 16; i++ )
	{
		acMatrixInverse[i] = cOut[iIndex[i]];
	}

	emscbMatrixMuliply( acMatrix, 4, 4, acMatrixInverse, 4, 4, cOut );

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EMS_RESULT
CDBFNullPointing::_ComplexMatrixNxNInverse( EMSCOMPLEX* acMatrix, EMSCOMPLEX* acMatrixInverse, ULONG ulMSize  )
{
	
	EMS_RESULT hr = EMS_OK;
	

	if (ulMSize > DBF_MAX_SATELLITES)
	{
		hr = EMS_FALSE;
	}
	else
	{
		// The following equations provide the complex matrix inverse
		//
		// z = A + i*C
		// z inverse = y00 + i*y10

		// Define:
		//
		//	r0 = inv(A)*C
		//  r1 = C*r0
		//  y11 = inv(r1 + A)
		//  y00 = y11;
		//  y01 = r0*y11
		//  y10 = -y01

		float A[DBF_MAX_SATELLITES*DBF_MAX_SATELLITES];
		float C[DBF_MAX_SATELLITES*DBF_MAX_SATELLITES];
		float r0[DBF_MAX_SATELLITES*DBF_MAX_SATELLITES];
		float r1[DBF_MAX_SATELLITES*DBF_MAX_SATELLITES];
		float AI[DBF_MAX_SATELLITES*DBF_MAX_SATELLITES];
		float y00[DBF_MAX_SATELLITES*DBF_MAX_SATELLITES];
		float y10[DBF_MAX_SATELLITES*DBF_MAX_SATELLITES];
		float y01[DBF_MAX_SATELLITES*DBF_MAX_SATELLITES];
		float y11[DBF_MAX_SATELLITES*DBF_MAX_SATELLITES];

		for ( ULONG i = 0; i < ulMSize*ulMSize; i++ )
		{
			A[i] = acMatrix[i].re;
			C[i] = acMatrix[i].im;
		}

		emssbMatrixInverse(  A, AI, ulMSize );

		emsbMatrixMuliply( AI, ulMSize, ulMSize, C, ulMSize, ulMSize, r0 );

		emsbMatrixMuliply( C, ulMSize, ulMSize, r0, ulMSize, ulMSize, r1 );

		emssbAdd3( r1, A, y10, ulMSize*ulMSize );
		emssbMatrixInverse(  y10, y11, ulMSize );

		emsbMatrixMuliply( r0, ulMSize, ulMSize, y11, ulMSize, ulMSize, y01 );

		for ( ULONG i = 0; i < ulMSize*ulMSize; i++ )
		{
			acMatrixInverse[i].re = y11[i];
			acMatrixInverse[i].im = -y01[i];
		}

	}
	return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EMS_RESULT
CDBFNullPointing::_ComplexMatrixInverse( EMSCOMPLEX* acMatrix, EMSCOMPLEX* acMatrixInverse, ULONG ulMSize  )
{
	EMS_RESULT hr = EMS_OK;
	int iIndex[9] = { 0,1,2,4,5,6,8,9,10 };
	EMSCOMPLEX cTest[16];

	switch (ulMSize)
	{
		case 1:
			acMatrixInverse[0] = _CDivide ( acMatrix[0] );
			break;

		case 2:
			_ComplexMatrix2x2Inverse( acMatrix, acMatrixInverse );
			break;

		case 3:
			EMSCOMPLEX cIN[16], cOUT[16];
			memset( cIN, 0, 16*sizeof( EMSCOMPLEX ) );
			memset( cOUT, 0, 16*sizeof( EMSCOMPLEX ) );

			for( int i = 0; i < 9; i++ )
			{
				cIN[iIndex[i]] = acMatrix[i];
			}
			cIN[15].re = 1.0;
			cIN[15].im = 0.0;
		
			hr = _ComplexMatrix4x4Inverse( cIN, cOUT );
		
			for( int i = 0; i < 9; i++ )
			{
				acMatrixInverse[i] = cOUT[iIndex[i]];
			}
			break;

		case 4:
			hr = _ComplexMatrix4x4Inverse( acMatrix, acMatrixInverse );
			return hr;
		
		default:
			hr = _ComplexMatrixNxNInverse( acMatrix, acMatrixInverse, ulMSize );
			break;
	}

	return hr;
}
	//ULONG ulSize = ulMSize * 2;

	//ULONG i, j, k;

	//float *fM = new float [ulSize * ulSize];
	//float *fMinv = new float [ulSize * ulSize];

	//memset( fM, 0, sizeof(float) * ulSize * ulSize );
	//memset( fMinv, 0, sizeof(float) * ulSize * ulSize );
	//
	//k = 0;
	//
	//for ( i = 0; i < ulMSize; i++ )
	//{
	//	for ( j = 0; j < ulMSize; j++ )
	//	{
	//		fM[k] = acMatrix[i+j].re;
	//		fM[k+ulMSize] = acMatrix[i+j].im;
	//		k++;
	//	}
	//	k += ulMSize;
	//}

	//for ( i = 0; i < ulMSize; i++ )
	//{
	//	for ( j = 0; j < ulMSize; j++ )
	//	{
	//		fM[k] = -acMatrix[i+j].im;
	//		fM[k+ulMSize] = acMatrix[i+j].re;
	//		k++;
	//	}
	//	k += ulMSize;
	//}

	//try
	//{
	//	emssbMatrixInverse( &fM[0], &fMinv[0], ulSize );
	//}
	//catch(...)
	//{
	//	printf("Input = ");
	//	for ( i = 0; i < ulSize * ulSize; i++ )
	//		printf("%f,",fM[i]);
	//	printf("\n");

	//}

	//for ( i = 0; i < ulMSize; i++ )
	//{
	//	acMatrixInverse[i].re = fMinv[k];
	//	acMatrixInverse[i].im = fMinv[k + ulMSize * ulMSize];
	//	k++;
	//}

	//if ( fM )
	//{
	//	delete [] fM;
	//	fM = NULL;
	//}
	//if ( fMinv )
	//{
	//	delete [] fMinv;
	//	fMinv = NULL;
	//}

	//return hr;
//}

//---------------------------------------------------------------------------

