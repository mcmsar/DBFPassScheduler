/********************************************************************
*	Module:			DBFPointingAngles.h
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
*	              Copyright (c) 2006 by EMS Technologies, Inc.,
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

	0.0	2010/12/15		rcr		start

********************************************************************/
#ifndef INC_DBFPOINTINGANGLES
#define INC_DBFPOINTINGANGLES

#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>

#include "emstypes.h"
#include "emserror.h"
#include "sigproc.h"
#include "emsDBFtypes2.h"

#include "gpsclock.h"
#include "emstime.h"

#include "emstle.h"
#include "emsorbit.h"
#include "emsatinf.h"
#include "DBFOrbitCalc.h"

#include <vector>
#include "EMSTextFileLog.h"

typedef struct _tagEMSDBFPASATCFG
{
	std::string szType;
	double fFrequency;
	std::string szPolarisation;
	ULONG ulSatId;
	ULONG ulPriority;

}EMSDBFPASATCFG;

typedef struct _tagEMSDBFPASITELOCATIONCFG
{
	ULONG ulLutId;
	EMSLOCATION locnAntenna;

}EMSDBFPASITELOCATIONCFG;

interface IEMSSatelliteInfo12;

const ULONG culMaxSats = 128;
const ULONG culMaxChannels = 63;
const double cdSiteMinElevation  = 5.0;  // minimum elevation angle at site
//const double cdPlateMinElevation = 30.0; // minimum elevation angle at plate
const double cdPlateMinElevationMEO = 5.0; // minimum elevation angle at plate
const double cdPlateMinElevationLEO = 5.0; // minimum elevation angle at plate for LEO


#define MAX_PATTERNS (5)
#define DBF_MAX_CHANNELS    (32)		// maximum number of input channels (based upon ADC board capabilities)
#define DBF_MAX_SATELLITES  (5)			// maximum number of DBF tracked satellites per face

class CDBFPointingAngles
{
public:

	~CDBFPointingAngles( void );
	CDBFPointingAngles();

	CDBFPointingAngles( const CDBFPointingAngles& x ) ;

	EMS_RESULT Initialize( std::vector<EMSDBFPASATCFG> &ulVectAllSats, EMSDBFPASITELOCATIONCFG &siteLocation );

	ULONG GetLutID(){return m_ulLutID;};

	void GetFixedPointSched( EMSDBFAZEL* aAzEls, ULONG* pulActual );

	EMS_RESULT GetSatelliteAZELs( EMSTIME timeNow, EMSDBFAZEL* aAzEls, ULONG* pulActual );

	EMS_RESULT GetPlateAZELs( EMSDBFAZEL* aAzEls, ULONG* pulActual );

	EMS_RESULT GetConstellationAZELs( EMSDBFAZEL* aAzEls, ULONG* pulActual );

	EMS_RESULT GetDBFPhase( EMSDBFAZEL* aAzEls, ULONG* pulActual );
	
	EMS_RESULT GetSeparationAngles( EMSDBFAZEL* aAzEls, ULONG* pulActual );

	int GetConstellationType( ULONG ulSatID );

	void SetPlate( EMSDBFARRAY* aDBFplate );

	EMS_RESULT SetBiasOffset( const float* biasOffset);

	EMS_RESULT ApplyPhaseBias( EMSDBFAZEL* aAzEls, ULONG* pulActual );
	EMS_RESULT ApplyPhaseBias1( EMSDBFAZEL* aAzEls, ULONG* pulActual );

	EMS_RESULT TestPhaseOutputPower( EMSTIME truetime, EMSDBFAZEL* aAzEls1, EMSDBFAZEL* aAzEls, ULONG* pulActual );

	EMS_RESULT PriorityOrder( EMSDBFAZEL* aAzEls, ULONG* pulActual );

	void RotationMatrix( );

	EMS_RESULT CellPhaseAngles( EMSDBFAZEL *aAzEl);

	EMS_RESULT CheckLEOMEO( EMSDBFAZEL* aAzEls, ULONG* pulActual );
	
	EMS_RESULT GetSatelliteVectorEF( EMSTIME timeNow, ULONG ulSatID, EMSTIMECOORD* timeCoord );

	EMS_RESULT GetConstellation( ULONG ulSatID, double* dWaveLength, double* dDownFrequency );

	EMSTLEDATA GetTLE( ULONG ulSatID );


private:

	void _ComputeNearFieldCorrections( EMSDBFAZEL *aAzEl, int index );
	
	void _NearFieldCorrections( EMSDBFAZEL *aAzEl, int index, bool bAdd );

	EMS_RESULT _GetValidSatellites( std::vector<EMSDBFPASATCFG> &ulVectAllSats );

	EMSVECTORD _Convert2UnitVector( const double dAzimuth, const double dElevation );

	EMS_RESULT _DBFcellpositions( double *dCellPositionX,
								  double *dCellPositionY );

	EMS_RESULT _Satellite2PlateRotation( const double dSatAzimuth, double dSatElevation,
										double *dDBFazimuth, double *dDBFelevation );

	EMS_RESULT _Satellite2PlateRotation_V2( const double dSatAzimuth, double dSatElevation,
										double *dDBFazimuth, double *dDBFelevation );


	void _Reset( );
	


//protected:

//private: // methods

private: // data

	BOOL					m_bInitialized;
	ULONG					m_ulChannels;
	ULONG					m_ulSatellites;

	CDBFOrbitCalc			*m_lpOrbit;

	ULONG					m_ulValidSatID;
	ULONG					m_aulSatID[ culMaxSats ];
	ULONG					m_aulPassID[ culMaxSats ];
	ULONG					m_aulSatPriority[ culMaxSats ];
	EMSTLEDATA				m_atleData[ culMaxSats ];

	ULONG					m_ulLutID;
	EMSLOCATION				m_locnAntenna;

	EMSDBFARRAY				m_aDBFplate;

	EMSVECTORD              m_vRotationRow1;
	EMSVECTORD              m_vRotationRow2;
	EMSVECTORD              m_vRotationRow3;

	double					m_dWaveLength;
	double					m_dDlFrequency;

	bool                    m_bSun;
	float					*m_pafBiasOffset;


//private: // constants

};


#endif // INC_DBFPOINTINGANGLES



