/********************************************************************
*	Module:			DBFNullPointing.h
*	Process ID:
*	S/W Platforms:
*	H/W Platforms:
*	Compiler:
*	Description: 	
*
*	Usage:
*	Entry Point:
*	Input Files:
*	Output Files:
*	Comments:
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
#ifndef INC_DBFNULLPOINTING
#define INC_DBFNULLPOINTING

#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>

#include "emstypes.h"
#include "emserror.h"
#include "emsDBFtypes2.h"
#include "sigproc.h"

//#define MAX_PATTERNS (5)
//#define DBF_MAX_CHANNELS    (64)		// maximum number of input channels (based upon ADC board capabilities)
//#define DBF_MAX_SATELLITES  (5)			// maximum number of DBF tracked satellites per face

class CDBFNullPointing
{
public:

	~CDBFNullPointing( void );
	CDBFNullPointing();

	CDBFNullPointing( const CDBFNullPointing& x ) ;

	EMS_RESULT Initialize();

	EMS_RESULT GetNullPhase( EMSDBFAZEL* aAzEls, ULONG* pulActual );

	EMS_RESULT _GetNullComplexMatrix( EMSDBFAZEL* aAzEls, ULONG ulChannels, ULONG ulSatellites,
									  EMSCOMPLEX* acMatrix, ULONG ulIndex );

	EMS_RESULT _ComplexMatrixInverse( EMSCOMPLEX* acMatrix, EMSCOMPLEX* acMatrixInverse, ULONG ulMSize );
	
	EMS_RESULT _ComplexMatrixMultiply( const EMSCOMPLEX* acMatrixA, const EMSCOMPLEX* acMatrixB, EMSCOMPLEX* acMatrixOUT, ULONG ulMSize );
	
	bool CDBFNullPointing::_MissingChannel( EMSDBFAZEL aAzEls, ULONG ulChannel );

	void _Reset( );
	
	EMSCOMPLEX _CDivide ( EMSCOMPLEX cData);
	EMSCOMPLEX _CMultiply ( EMSCOMPLEX cData1, EMSCOMPLEX cData2 );
	EMS_RESULT _CMultipy2x2( EMSCOMPLEX* cM1, EMSCOMPLEX* cM2, EMSCOMPLEX* cM3);
	EMSCOMPLEX _CAdd ( EMSCOMPLEX cData1, EMSCOMPLEX cData2 );
	EMSCOMPLEX _CSub ( EMSCOMPLEX cData1, EMSCOMPLEX cData2 );
	EMS_RESULT _ComplexMatrix2x2Inverse( EMSCOMPLEX* acMatrix, EMSCOMPLEX* acMatrixInverse  );
	EMS_RESULT _ComplexMatrix4x4Inverse( EMSCOMPLEX* acMatrix, EMSCOMPLEX* acMatrixInverse  );
	EMS_RESULT _ComplexMatrixNxNInverse( EMSCOMPLEX* acMatrix, EMSCOMPLEX* acMatrixInverse, ULONG ulMSize  );
	float      _FMatrixMultiply( float* afMatrix1, ULONG n1, ULONG m1, float* afMatrix2, ULONG n2, ULONG m2  );

//protected:

//private: // methods

private: // data

	BOOL		m_bInitialized;
	ULONG		m_ulChannels;
	ULONG		m_ulSatellites;



	EMSCOMPLEX	m_acA[DBF_MAX_SATELLITES * DBF_MAX_CHANNELS];
	EMSCOMPLEX	m_acAAh[DBF_MAX_SATELLITES * DBF_MAX_SATELLITES];
	EMSCOMPLEX	m_acAh[DBF_MAX_CHANNELS * DBF_MAX_SATELLITES];
	EMSCOMPLEX	m_acInvAAh[DBF_MAX_SATELLITES * DBF_MAX_SATELLITES];
	EMSCOMPLEX	m_acInvAAh_A[DBF_MAX_SATELLITES * DBF_MAX_CHANNELS];
	EMSCOMPLEX	m_acP[DBF_MAX_CHANNELS * DBF_MAX_CHANNELS];


	EMSCOMPLEX	m_acMA[DBF_MAX_SATELLITES][DBF_MAX_CHANNELS];
	EMSCOMPLEX	m_acMAAh[DBF_MAX_SATELLITES][DBF_MAX_SATELLITES];
	EMSCOMPLEX	m_acMAh[DBF_MAX_CHANNELS][DBF_MAX_SATELLITES];
	EMSCOMPLEX	m_acMInvAAh[DBF_MAX_SATELLITES][DBF_MAX_SATELLITES];
	EMSCOMPLEX	m_acMInvAAh_A[DBF_MAX_SATELLITES][DBF_MAX_CHANNELS];
	EMSCOMPLEX	m_acMP[DBF_MAX_CHANNELS][DBF_MAX_CHANNELS];

	EMSDBFAZEL	m_acAngles[DBF_MAX_SATELLITES];


//private: // constants

};


#endif // INC_DBFNULLPOINTING



