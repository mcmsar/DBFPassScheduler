/********************************************************************
*	Module:			DBFPointingAngles.cpp 
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

	0.0	2010/15/12		rcr		start

********************************************************************/
#include "DBFPointingAngles.h"
#include "emsDBFtypes2.h"
#include "emsconst.h"
#include "emsvectr.h"
#include "emscoord.h"
#include <new>

#include "emsatinf.h"

#include "emslinfo.h"
#include "emstleconv.h"
#include "emssun.h"
#include "emsclock.h"
#include "convutility.h"
#include <iostream>
#include <sstream>
#include "DBFPSConfig.h"

const CEMSCoordSystem c_coordSys;

/********************************************************************/



CDBFPointingAngles::CDBFPointingAngles() : m_ulChannels(0), m_ulSatellites(0),
								m_lpOrbit(NULL), m_pafBiasOffset(NULL)
{
	m_bInitialized = FALSE;
	m_bSun = false;
	m_dDlFrequency = 0.0;

	m_dPlateMinElevation = cdPlateMinElevationDefault;
	m_dPlateMinAzimuth   = cdPlateMinAzimuthDefault;
	m_dPlateMaxAzimuth   = cdPlateMaxAzimuthDefault;
}

CDBFPointingAngles::CDBFPointingAngles( const CDBFPointingAngles& x ) : m_ulChannels( x.m_ulChannels ),
							m_ulSatellites(x.m_ulSatellites),
							m_lpOrbit(NULL)
{
	// Orbit object is not shared across copies; it will be created on demand
	// via Initialize().
	m_bInitialized = FALSE;

	m_dPlateMinElevation = x.m_dPlateMinElevation;
	m_dPlateMinAzimuth   = x.m_dPlateMinAzimuth;
	m_dPlateMaxAzimuth   = x.m_dPlateMaxAzimuth;
}

CDBFPointingAngles::~CDBFPointingAngles( void )
{
	m_ulChannels = 0;
	m_ulSatellites = 0;
		
	_Reset();
}

//---------------------------------------------------------------------------

void
CDBFPointingAngles::_Reset( )
{
	if ( m_lpOrbit )
	{
		delete m_lpOrbit;
		m_lpOrbit = 0;
	}

	if ( m_bInitialized )
	{
		m_bInitialized = FALSE;
	}
	return;
}

//---------------------------------------------------------------------------

void 
CDBFPointingAngles::SetPlate( EMSDBFARRAY *aDBFplate )
{
	memset(&m_aDBFplate, 0, sizeof(EMSDBFARRAY));
	memcpy( &m_aDBFplate, aDBFplate, sizeof(EMSDBFARRAY) );
}
//---------------------------------------------------------------------------

void
CDBFPointingAngles::SetPlateLimits( double dMinElevation, double dMinAzimuth, double dMaxAzimuth )
{
	m_dPlateMinElevation = dMinElevation;
	m_dPlateMinAzimuth   = dMinAzimuth;
	m_dPlateMaxAzimuth   = dMaxAzimuth;
}
//---------------------------------------------------------------------------

// True when a satellite at dSatAzimuth (degrees north, site frame) falls inside the
// plate's azimuth acceptance window. The window is expressed as absolute azimuths
// rather than an offset from the plate face, so a plate facing 180 with 90 degrees of
// coverage either side is configured as 90..270.
bool
CDBFPointingAngles::_InPlateAzimuthWindow( const double dSatAzimuth ) const
{
	// A window covering the whole circle accepts everything. Checked before the
	// wrap handling below, because the default 0..360 normalises to 0..0 and would
	// otherwise be read as an empty window.
	if ( ( m_dPlateMaxAzimuth - m_dPlateMinAzimuth ) >= 360.0 )
	{
		return true;
	}

	double dAz  = fmod( fmod( dSatAzimuth, 360.0 ) + 360.0, 360.0 );
	double dMin = fmod( fmod( m_dPlateMinAzimuth, 360.0 ) + 360.0, 360.0 );
	double dMax = fmod( fmod( m_dPlateMaxAzimuth, 360.0 ) + 360.0, 360.0 );

	if ( dMin <= dMax )
	{
		// Ordinary window, e.g. 90..270 for a plate facing south.
		return ( dAz >= dMin ) && ( dAz <= dMax );
	}

	// Window wraps through north, e.g. 280..100 for a plate facing 010.
	return ( dAz >= dMin ) || ( dAz <= dMax );
}
//---------------------------------------------------------------------------

EMS_RESULT
CDBFPointingAngles::Initialize( std::vector<EMSDBFPASATCFG> &ulVectAllSats, EMSDBFPASITELOCATIONCFG &siteLocation )
{
	EMS_RESULT hr = EMS_UNKNOWN_ERROR;

	m_ulChannels   = DBF_MAX_CHANNELS;
	m_ulSatellites = DBF_MAX_SATELLITES;

	m_dDlFrequency = 1544.1e6;
	m_dWaveLength  = (c_dVelocityOfLight / m_dDlFrequency) * 10000; // in centimetres 

	m_lpOrbit = 0;
	m_ulLutID = 0;
	m_ulValidSatID = 0;
	
	memset( &m_atleData, 0, culMaxSats*sizeof( EMSTLEDATA ) );
	memset( m_aulSatID, 0, culMaxSats );

	DWORD dwResult = 0;

	if ( !m_bInitialized )
	{
		m_lpOrbit = new (std::nothrow) CDBFOrbitCalc();
		hr = m_lpOrbit ? m_lpOrbit->Init() : EMS_UNKNOWN_ERROR;

		if( EMS_OK == hr)
		{
			if(ulVectAllSats.size() > 0)
			{
				m_ulLutID = siteLocation.ulLutId;
				memcpy(&m_locnAntenna, &siteLocation.locnAntenna, sizeof(EMSLOCATION));

				hr = _GetValidSatellites( ulVectAllSats );
			}
			else
				hr = EMS_FALSE;
		}

		m_bInitialized = ( hr == EMS_OK );
	}
			
	if( EMS_OK == hr )
	{
		if ( (m_ulChannels>0) && (m_ulSatellites>0) && (m_atleData) )
		{
			hr = EMS_OK;
		}
		else
		{
			hr = EMS_UNKNOWN_ERROR;
		}
	}

	return hr;
}


//---------------------------------------------------------------------------

EMS_RESULT 
CDBFPointingAngles::_GetValidSatellites( std::vector<EMSDBFPASATCFG> &ulVectAllSats )
{
	EMS_RESULT hr = EMS_BAD_PARAM;
	
	ULONG ulSatVectSize = ulVectAllSats.size();

	FILE* lpTraceFILE =  CDBFPSConfig::Instance().OpenTraceLog();


	if( ( ulSatVectSize> 0) && m_lpOrbit )
	{
		EMSTLEDATA tleData;
		EMSTIME timeNow;
		CEMSSystemClock     oTime;

		timeNow = oTime.GetTime();

		memset( &tleData, 0, sizeof(EMSTLEDATA) );

		m_ulValidSatID = 0;
		ULONG ulSatID = 0;
		ULONG ulPriority = 0;

		for( ULONG l = 0; (l < ulSatVectSize) && (m_ulValidSatID < culMaxSats); l++ )
		{
			try
			{
				
				ulSatID = ulVectAllSats[l].ulSatId;
				ulPriority = ulVectAllSats[l].ulPriority;

				if( (ulSatID < 700) && (ulPriority > 0) )
				{

					hr = m_lpOrbit->GetSatelliteTLE( ulSatID, timeNow, &tleData );

					if( EMS_OK == hr )
					{
						m_aulSatID[ m_ulValidSatID ] = ulSatID;
						m_aulSatPriority[ m_ulValidSatID ] = ulPriority;
						memcpy( &m_atleData[ m_ulValidSatID ], &tleData, sizeof(EMSTLEDATA) );
						m_ulValidSatID++;
					}
					if (lpTraceFILE) fprintf(lpTraceFILE,"%03d : %d (%d) hr = %d : %I64d, %d, %10.5f,%10.5f,%10.5f,%10.5f,%10.5f,%10.5f\n",
						ulSatID, m_ulValidSatID, ulSatVectSize, hr,
						tleData.timeEpoch.intTime,
						tleData.nOrbitNumber,
						tleData.elem.var.fArgPerigee,
						tleData.elem.var.fEccentricity,
						tleData.elem.var.fInclination,
						tleData.elem.var.fMeanAnomaly,
						tleData.elem.var.fMeanMotion,
						tleData.elem.var.fRightAscNode);
				}
			}
			catch(...)
			{
				// error.
			}
		}

	}
	
	if (lpTraceFILE) fclose( lpTraceFILE);

	return hr;
}

//---------------------------------------------------------------------------

int
CDBFPointingAngles::GetConstellationType( ULONG ulSatID )
{
	int nSatType;

	nSatType = ulSatID / 100;

	return nSatType;
}

//---------------------------------------------------------------------------

EMSTLEDATA
CDBFPointingAngles::GetTLE( ULONG ulSatID )
{
	EMS_RESULT hr = EMS_OK;
	
	EMSTLEDATA aTLEdata;

	memset( &aTLEdata, 0, sizeof(EMSTLEDATA) );

	for ( int i = 0; i < culMaxSats; i++ )
	{
		if ( ulSatID == m_aulSatID[i] )
		{
			memcpy( &aTLEdata, &m_atleData[i], sizeof(EMSTLEDATA) );
			break;
		}

	}
	return aTLEdata;
}

//---------------------------------------------------------------------------

EMS_RESULT
CDBFPointingAngles::GetConstellationAZELs( EMSDBFAZEL* aAzEls, ULONG* pulActual )
{
	EMS_RESULT hr = EMS_OK;
	ULONG ulRecords = 0;

	if( !pulActual )
	{
		hr = EMS_INVALID_PTR;
	}
	else
	{
		ulRecords = *pulActual;
		*pulActual = 0;
	}

	if( EMS_OK == hr )
	{

		// Determine satellite constellation counts

		int anSatCount[8];
		memset( anSatCount, 0, sizeof(int) * 8 );

		int nSatType  = -1;
		ULONG i;

		for( i = 0; i < ulRecords; i++ )
		{
			nSatType = GetConstellationType( aAzEls[i].ulSatID );

			if ( (nSatType > -1) && (nSatType < 8) )
			//if ( (nSatType > 3) && (nSatType < 6) )
			{
				anSatCount[nSatType] += aAzEls[i].ulSatPriority;
			}
		}

		// Choose constellation with maximum number of visible satellites

		int nSatTypeX = -1;
		int nMaxCount = 0;

		//for ( i = 0; i < 4; i++ )
		for ( i = 0; i < 8; i++ )
		{
			if ( anSatCount[i] > nMaxCount )
			{
				nMaxCount = anSatCount[i];
				nSatTypeX = i;
			}
		}

		for ( i = 0; i < ulRecords; i++ )
		{

			nSatType = GetConstellationType( aAzEls[i].ulSatID );

			if (  nSatType == nSatTypeX || aAzEls[i].ulSatID == 999) //include sun vector
			{
				aAzEls[*pulActual] = aAzEls[i];
				(*pulActual)++;
			}
		}

	}

	return hr;
}

//---------------------------------------------------------------------------

EMSVECTORD
CDBFPointingAngles::_Convert2UnitVector( const double dAzimuth, const double dElevation )
{
	EMSVECTORD vUnitVector;
		
	// Compute unit vector pointing vectors
	vUnitVector.dX = cos( dAzimuth * c_dDegToRad ) * cos( dElevation * c_dDegToRad );
	vUnitVector.dY = sin( dAzimuth * c_dDegToRad ) * cos( dElevation * c_dDegToRad );
	vUnitVector.dZ = sin( dElevation * c_dDegToRad );

	return vUnitVector;
}
//---------------------------------------------------------------------------

void
CDBFPointingAngles::RotationMatrix( )
{
	EMSVECTORD vPlateX, vPlateY, vPlateZ;
	EMSVECTORD X, Y, Z;
	double dCangle1,dCangle2, dCangle3;

	double dPlateAzimuth   = m_aDBFplate.fPlateFaceAzimuth;
	double dPlateElevation = m_aDBFplate.fPlateFaceElevation;

	m_vRotationRow1.dX = -sin(dPlateAzimuth  * c_dDegToRad);
	m_vRotationRow1.dY = cos(dPlateAzimuth  * c_dDegToRad);
	m_vRotationRow1.dZ = 0.0;

	m_vRotationRow2.dX = -sin(dPlateElevation  * c_dDegToRad) * cos(dPlateAzimuth  * c_dDegToRad);
	m_vRotationRow2.dY = -sin(dPlateElevation  * c_dDegToRad) * sin(dPlateAzimuth  * c_dDegToRad);
	m_vRotationRow2.dZ = cos(dPlateElevation  * c_dDegToRad);

	m_vRotationRow3.dX = cos(dPlateElevation  * c_dDegToRad) * cos(dPlateAzimuth  * c_dDegToRad);
	m_vRotationRow3.dY = cos(dPlateElevation  * c_dDegToRad) * sin(dPlateAzimuth  * c_dDegToRad);
	m_vRotationRow3.dZ = sin(dPlateElevation  * c_dDegToRad);

	// Validiation of matrix

	vPlateX = _Convert2UnitVector( dPlateAzimuth+90.0, 0.0 );
	vPlateY = _Convert2UnitVector( dPlateAzimuth-180, 90.0-dPlateElevation );
	vPlateZ = _Convert2UnitVector( dPlateAzimuth, dPlateElevation );

	dCangle1 = CEMSVector::DotProduct( vPlateX, vPlateY );
	dCangle2 = CEMSVector::DotProduct( vPlateY, vPlateZ );
	dCangle3 = CEMSVector::DotProduct( vPlateZ, vPlateX );

	X.dX =  CEMSVector::DotProduct( m_vRotationRow1, vPlateX );
	X.dY =  CEMSVector::DotProduct( m_vRotationRow2, vPlateX );
	X.dZ =  CEMSVector::DotProduct( m_vRotationRow3, vPlateX );

	Y.dX =  CEMSVector::DotProduct( m_vRotationRow1, vPlateY );
	Y.dY =  CEMSVector::DotProduct( m_vRotationRow2, vPlateY );
	Y.dZ =  CEMSVector::DotProduct( m_vRotationRow3, vPlateY );

	Z.dX =  CEMSVector::DotProduct( m_vRotationRow1, vPlateZ );
	Z.dY =  CEMSVector::DotProduct( m_vRotationRow2, vPlateZ );
	Z.dZ =  CEMSVector::DotProduct( m_vRotationRow3, vPlateZ );

	return;
}


//---------------------------------------------------------------------------

EMS_RESULT
CDBFPointingAngles::_Satellite2PlateRotation( const double dSatAzimuth, double dSatElevation,
											double *dDBFazimuth, double *dDBFelevation )
{
	EMS_RESULT hr = EMS_BAD_PARAM;

	if (dSatElevation > 0)
	{

		EMSVECTORD vSat, vPlate;
		EMSVECTORD v1, v2, v3;

		double dCangle;
		double dAz, dEl;

		double dPlateAzimuth   = m_aDBFplate.fPlateFaceAzimuth;
		double dPlateElevation = m_aDBFplate.fPlateFaceElevation;
		double dPlateRotation  = m_aDBFplate.fPlateFaceRotation;

		// <platefaceelevation> is the tilt of the face away from the ZENITH, not an
		// elevation above the horizon: 0 points the boresight straight up, 90 lays it
		// flat against the horizon. This is what makes a plate configured at azimuth 0
		// and elevation 0 report each satellite's own azimuth and elevation unchanged,
		// which is the natural identity check for the transform. 45 degrees means the
		// same thing under either reading, which is why every tilted system agreed and
		// the difference only showed up on a plate configured at 0.
		double dBoresightElevation = 90.0 - dPlateElevation;

		// Compute unit vector pointing vectors
		vSat   = _Convert2UnitVector(dSatAzimuth, dSatElevation);
		vPlate = _Convert2UnitVector(dPlateAzimuth, dBoresightElevation);

		// Compute satellite elevation in DBF plate coordinates
		dCangle = CEMSVector::DotProduct(vSat, vPlate);
		dEl = asin(dCangle) * c_dRadToDeg;

		// Compute Axes of Plate surface. The degenerate case is a boresight straight up
		// the Z axis, which is now <platefaceelevation> 0 rather than 90.
		if (abs(dPlateElevation) > 0.01)
		{
			// Cross products

			// v1 perpendicular to plate vector and Z axis (0,0,1)
			v1.dX = -vPlate.dY;
			v1.dY = vPlate.dX;
			v1.dZ = 0.0;
			v1 = CEMSVector::Normalize(v1);

			// v2  perpendicular to plate vector and v1
			v2.dX = v1.dY * vPlate.dZ - v1.dZ * vPlate.dY;
			v2.dY = v1.dZ * vPlate.dX - v1.dX * vPlate.dZ;
			v2.dZ = v1.dX * vPlate.dY - v1.dY * vPlate.dX;
			v2 = CEMSVector::Normalize(v2);

			// Projection of the satellite vector onto the plate surface, as its
			// components along the two in-plane axes. This used to multiply vSat
			// element-wise by (v1 + v2), which is not a projection onto anything: it
			// mixes the two axes together and leaves the result in world coordinates,
			// so the atan2 below was reading axes that were never separated. With the
			// face tilted the answer was merely wrong; with a face at zero elevation
			// v1 + v2 is axis aligned, v3.dX collapses to exactly zero for every
			// satellite, and the azimuth can only come out as 90 or 270.
			v3.dX = CEMSVector::DotProduct( vSat, v1 );
			v3.dY = CEMSVector::DotProduct( vSat, v2 );
			v3.dZ = 0.0;

			// Measured from v2 (the in-plane "up" axis) toward v1 (transverse), which
			// is what makes this agree with the face-at-zenith case below: as the face
			// tilts up to 90 degrees v2 approaches the plate azimuth direction and this
			// converges on dSatAzimuth - dPlateAzimuth. Taking the arguments the other
			// way round measures from v1 instead, which is the same angle reflected and
			// offset by 90 degrees, and left the two branches disagreeing across the
			// boundary.
			dAz = atan2(v3.dX, v3.dY) * c_dRadToDeg;
		}
		else
		{
			// Face points at the zenith, where v1 above degenerates: vPlate is (0,0,1),
			// so the vector being normalised is zero to within rounding. Use the limit
			// of the branch above instead. The plate azimuth still counts here -- it
			// orients the face about the vertical -- so it is subtracted rather than
			// ignored, which together with dEl below makes this the identity transform
			// when the plate is left at azimuth 0.
			dAz = dSatAzimuth - dPlateAzimuth;
		}

		// Normalise into [0,360) rather than nudging once by 360: dPlateRotation can
		// push the sum past either end, and the single correction below could leave an
		// out-of-range azimuth when it did.
		dAz = fmod( dAz + dPlateRotation, 360.0 );
		if (dAz < 0.0 ) dAz = dAz + 360.0;

		*dDBFazimuth   = dAz;
		*dDBFelevation = dEl;

		hr = EMS_OK;

	}
	return hr;
}
//---------------------------------------------------------------------------
// Zeinab's function
//---------------------------------------------------------------------------

EMS_RESULT
CDBFPointingAngles::_Satellite2PlateRotation_V2( const double dSatAzimuth, double dSatElevation,
												 double *dDBFazimuth, double *dDBFelevation )
{
	EMS_RESULT hr = EMS_BAD_PARAM;

	if( dSatElevation > 0 )
	{

		EMSVECTORD vSat, vPlateX, vPlateY, vPlateZ;
		EMSVECTORD vS;
		
		double dAz = 0;
		double dEl = 0;
		

		double dPlateAzimuth   = m_aDBFplate.fPlateFaceAzimuth;
		double dPlateElevation = -m_aDBFplate.fPlateFaceElevation;

		// Compute unit vector pointing vectors
		vSat    = _Convert2UnitVector( dSatAzimuth, dSatElevation );


		/*vPlateX.dX = -sin(dPlateAzimuth * c_dDegToRad);
		vPlateX.dY = cos(dPlateAzimuth * c_dDegToRad);
                vPlateX.dZ = 0.0 ;

		vPlateY.dX = -sin(dPlateElevation * c_dDegToRad) * cos(dPlateAzimuth * c_dDegToRad);
		vPlateY.dY = -sin(dPlateElevation * c_dDegToRad) * sin(dPlateAzimuth * c_dDegToRad);
                vPlateY.dZ = cos(dPlateElevation * c_dDegToRad) ;

		vPlateZ.dX = cos(dPlateElevation * c_dDegToRad) * cos(dPlateAzimuth * c_dDegToRad);
		vPlateZ.dY = cos(dPlateElevation * c_dDegToRad) * sin(dPlateAzimuth * c_dDegToRad);
                vPlateZ.dZ = sin(dPlateElevation * c_dDegToRad) ;*/
		
		vPlateX.dX =  cos(dPlateAzimuth * c_dDegToRad);
		vPlateX.dY =  sin(dPlateAzimuth * c_dDegToRad);
		vPlateX.dZ =  0.0 ;

		vPlateY.dX =  -cos(dPlateElevation * c_dDegToRad) * sin(dPlateAzimuth * c_dDegToRad);
		vPlateY.dY =  cos(dPlateElevation * c_dDegToRad) * cos(dPlateAzimuth * c_dDegToRad);
		vPlateY.dZ =  sin(dPlateElevation * c_dDegToRad) ;

		vPlateZ.dX =  sin(dPlateElevation * c_dDegToRad) * sin(dPlateAzimuth * c_dDegToRad);
		vPlateZ.dY =  -sin(dPlateElevation * c_dDegToRad) * cos(dPlateAzimuth * c_dDegToRad);
		vPlateZ.dZ =  cos(dPlateElevation * c_dDegToRad) ;

		vS.dX = CEMSVector::DotProduct( vSat, vPlateX );
		vS.dY = CEMSVector::DotProduct( vSat, vPlateY );
		vS.dZ = CEMSVector::DotProduct( vSat, vPlateZ );	
		
		
		dEl = asin( vS.dZ ) * c_dRadToDeg;

		// Compute azimuth angle
		if ( dEl <= 89.95 )
		{
			dAz = atan2( vS.dY , vS.dX ) * c_dRadToDeg ;

			if ( dAz > 180)
			{
				dAz = dAz - 360;
			}
			else
			{
				dAz = dAz;
			}

		}

		*dDBFazimuth   = fmod( dAz + 360.0, 360.0 );
		*dDBFelevation = dEl;

		hr = EMS_OK;

	}
	return hr;
}

//---------------------------------------------------------------------------

void
CDBFPointingAngles::_NearFieldCorrections( EMSDBFAZEL *aAzEl , int index, bool bAdd )
{
	EMS_RESULT hr = EMS_BAD_PARAM;

	for ( int i = 1; i < DBF_MAX_CHANNELS; i++ )
	{
		if ( bAdd )
		{
			m_aDBFplate.emsDBFCells[i].fRHCP += m_aDBFplate.emsFixedPoints[index].fPhaseAdjust[i];
			m_aDBFplate.emsDBFCells[i].fLHCP += m_aDBFplate.emsFixedPoints[index].fPhaseAdjust[i];
		}
		else
		{
			m_aDBFplate.emsDBFCells[i].fRHCP -= m_aDBFplate.emsFixedPoints[index].fPhaseAdjust[i];
			m_aDBFplate.emsDBFCells[i].fLHCP -= m_aDBFplate.emsFixedPoints[index].fPhaseAdjust[i];
		}
	}
	return;
}
//---------------------------------------------------------------------------

void
CDBFPointingAngles::_ComputeNearFieldCorrections( EMSDBFAZEL *aAzEl , int index )
{
	EMS_RESULT hr = EMS_BAD_PARAM;

	if( aAzEl > 0 )
	{

		EMSVECTORD vSat;
		vSat = _Convert2UnitVector( aAzEl[0].degPlateAzimuth, aAzEl[0].degPlateElevation );

		// Note: Azimuth is measured from the Y axis of the plate. Therefore there is a 90 degree switch in the vSat vector.

		double dSatDist = m_aDBFplate.emsFixedPoints[index].fFixedPointDistance;
		double dCosAngle = 0.0;
		double dAngle = 0.0;
		double dCellRadius = 0.0;
		double dActualDistance = 0.0;  // distance from element to transmitter (in cm)
		double dExtraDistance = 0.0;


		// Perform loop for number of cells NOT number of channels
		for( int i = 1; i < DBF_MAX_CHANNELS; i++ )
		{
			
			//dCosAngle = m_aDBFplate.emsDBFCells[i].fXcoord * vSat.dX * dSatDist  + m_aDBFplate.emsDBFCells[i].fYcoord * vSat.dY * dSatDist;
			dCosAngle = m_aDBFplate.emsDBFCells[i].fXcoord * vSat.dY * dSatDist  + m_aDBFplate.emsDBFCells[i].fYcoord * vSat.dX * dSatDist;

			dCellRadius = sqrt( m_aDBFplate.emsDBFCells[i].fXcoord * m_aDBFplate.emsDBFCells[i].fXcoord
							  + m_aDBFplate.emsDBFCells[i].fYcoord * m_aDBFplate.emsDBFCells[i].fYcoord );


			dActualDistance = sqrt( dSatDist*dSatDist + dCellRadius*dCellRadius - 2.0*dCosAngle);

			dAngle = acos( dCosAngle/dCellRadius/dSatDist );

			dExtraDistance = dCellRadius * sin( 180 - dAngle );

			m_aDBFplate.emsFixedPoints[index].fPhaseAdjust[i] = (float) (( dActualDistance - dSatDist - dExtraDistance ) / m_dWaveLength * 360.0 );

		}
	}

	//m_aDBFplate;
}

EMS_RESULT
CDBFPointingAngles::CellPhaseAngles( EMSDBFAZEL *aAzEl )
{
	EMS_RESULT hr = EMS_BAD_PARAM;


	if( aAzEl > 0 )
	{

		EMSVECTORD vSat;
		
		// Note: Azimuth is measured from the Y axis of the plate. Therefore there is a 90 degree switch in the vSat vector.
		vSat = _Convert2UnitVector( (90.0-aAzEl[0].degPlateAzimuth), aAzEl[0].degPlateElevation );
		
		double dPhase = 0.0;
		double dCellRadius = 0.0;
		double dDelayDistance = 0.0;  // delay distance from centre of array (in metres)

		// Perform loop for number of cells NOT number of channels
		for( int i = 1; i < DBF_MAX_CHANNELS; i++ )
		{
			//dCellRadius = sqrt( m_aDBFplate.fXcoord[i] * m_aDBFplate.fXcoord[i]
			//				  + m_aDBFplate.fYcoord[i] * m_aDBFplate.fYcoord[i] );

			//dDelayDistance = vSat.dX * m_aDBFplate.fXcoord[i] + vSat.dY * m_aDBFplate.fYcoord[i];
			dCellRadius = sqrt( m_aDBFplate.emsDBFCells[i].fXcoord * m_aDBFplate.emsDBFCells[i].fXcoord
							  + m_aDBFplate.emsDBFCells[i].fYcoord * m_aDBFplate.emsDBFCells[i].fYcoord );


			dDelayDistance = vSat.dX * m_aDBFplate.emsDBFCells[i].fXcoord + vSat.dY * m_aDBFplate.emsDBFCells[i].fYcoord;
			//dDelayDistance = vSat.dY * m_aDBFplate.emsDBFCells[i].fXcoord + vSat.dX * m_aDBFplate.emsDBFCells[i].fYcoord;

			GetConstellation( aAzEl->ulSatID, &m_dWaveLength, &m_dDlFrequency );

			dPhase = dDelayDistance / m_dWaveLength;
			
			//Question if it is should be applied now or later 
			//if ( m_aDBFplate.wPlatePolarization )
				//dPhase += (m_aDBFplate.emsDBFCells[i].fRHCP * c_dTwoPI/360.00);
			//else
				//dPhase += (m_aDBFplate.emsDBFCells[i].fLHCP * c_dTwoPI/360.00);

			dPhase = fmod(dPhase * c_dTwoPI, c_dTwoPI );

			// khalid's changes, Zeinab's change
			//aAzEl[0].acmplxPhase[m_aDBFplate.wCellID[i]].re = (float) cos( dPhase );
			//aAzEl[0].acmplxPhase[m_aDBFplate.wCellID[i]].im = (float) sin( dPhase );
		
			aAzEl[0].acmplxPhase[m_aDBFplate.emsDBFCells[i].wChannelID].re = (float) cos( dPhase );
			aAzEl[0].acmplxPhase[m_aDBFplate.emsDBFCells[i].wChannelID].im = (float) sin( dPhase );
			
			aAzEl[0].acmplxPhase[m_aDBFplate.emsDBFCells[i].wChannelID].im *= -1.0;;

		}

		hr = EMS_OK;
	}
	return hr;
}



//---------------------------------------------------------------------------

EMS_RESULT
CDBFPointingAngles::GetPlateAZELs( EMSDBFAZEL* aAzEls, ULONG* pulActual )
{
	EMS_RESULT hr = EMS_OK;
	ULONG ulRecords = 0;

	if( !pulActual )
	{
		hr = EMS_INVALID_PTR;
	}
	else
	{
		ulRecords = *pulActual;
		*pulActual = 0;
	}

	if( EMS_OK == hr )
	{
	
		double dDBFazimuth;
		double dDBFelevation;

		for ( ULONG l = 0; l < ulRecords; l++ )
		{
			hr = _Satellite2PlateRotation( aAzEls[l].degSatAzimuth, aAzEls[l].degSatElevation,
										&dDBFazimuth, &dDBFelevation );
			// ZA use the new function
			//hr = _Satellite2PlateRotation_V2( aAzEls[l].degSatAzimuth, aAzEls[l].degSatElevation,
			//								&dDBFazimuth, &dDBFelevation );

			// dDBFelevation is the angle off the plate boresight, so on its own it only
			// describes a cone about the face normal. With the face tilted up, that cone
			// reaches well past the azimuths the antenna can actually see, so the
			// configured azimuth window is applied as a separate constraint on the
			// satellite's true azimuth.
			if ( (EMS_OK == hr)
				&& ( dDBFelevation > m_dPlateMinElevation )
				&& _InPlateAzimuthWindow( aAzEls[l].degSatAzimuth ) )
			{
				
				aAzEls[*pulActual] = aAzEls[l];
				aAzEls[*pulActual].degPlateAzimuth   = dDBFazimuth;
				aAzEls[*pulActual].degPlateElevation = dDBFelevation;

				(*pulActual)++;
			}

		}

	}

	return hr;
}
//---------------------------------------------------------------------------


void
CDBFPointingAngles::GetFixedPointSched( EMSDBFAZEL* aAzEls, ULONG* pulActual )
{
	*pulActual = 1;
	int iCount = 0;

	for ( int i = 0; i < m_aDBFplate.wFixedPointNumber; i++ )
	{
		if ( m_aDBFplate.emsFixedPoints[i].bEnabled )
		{
			aAzEls[iCount].degSatAzimuth = m_aDBFplate.emsFixedPoints[i].fFixPointAzimuth;
			aAzEls[iCount].degSatElevation = m_aDBFplate.emsFixedPoints[i].fFixPointElevation;
			GetPlateAZELs( &aAzEls[iCount], pulActual );
			_ComputeNearFieldCorrections( &aAzEls[iCount], i );
			_NearFieldCorrections( &aAzEls[iCount], i, true );
			CellPhaseAngles( &aAzEls[iCount] );
			_NearFieldCorrections( &aAzEls[iCount], i, false );

			iCount++;
		}
	}
	*pulActual = iCount;
	//GetNullPhase( aAzEls, &ulActual );
	return;
}

EMS_RESULT
CDBFPointingAngles::CheckLEOMEO( EMSDBFAZEL* aAzEls, ULONG* pulActual )
{
	EMS_RESULT hr = EMS_OK;
	
	ULONG ulCount = 0;
	bool bLEO = false;

	if( !pulActual )
	{
		hr = EMS_INVALID_PTR;
	}
	for( ULONG i = 0; i < *pulActual; i++ )
	{
		if( aAzEls[i].ulSatID < 200 ) bLEO = true;
	}

	if( bLEO )
	{
		for( ULONG i = 0; i < *pulActual; i++ )
		{
			if( aAzEls[i].ulSatID < 200 )
			{
				memcpy( &aAzEls[ulCount], &aAzEls[i], sizeof(EMSDBFAZEL) );
				ulCount++;
			}
		}
		*pulActual = ulCount;
	}
	return hr;
}

EMS_RESULT
CDBFPointingAngles::GetSatelliteAZELs( EMSTIME timeNow, EMSDBFAZEL* aAzEls, ULONG* pulActual )
{
	EMSAZELVECTOR azelVector;
	EMS_RESULT hr = EMS_OK;

	if( !pulActual )
	{
		hr = EMS_INVALID_PTR;
	}
	else
		*pulActual = 0;

	ULONG ulActual = 0;

	if( EMS_OK == hr )
	{
		memset( &azelVector, 0, sizeof( azelVector ) );

		if( m_lpOrbit )
		{
			EMSTIMECOORD timeCoord;
			EMSTIMECOORD timeCoordEF;
			ULONG ulCount = 0;

			EMSTLEDATA tleData;
			memset( &tleData, 0, sizeof(EMSTLEDATA) );

			for( ULONG l = 0; (l < m_ulValidSatID) && ( EMS_OK == hr ); l++ )
			{

				hr = m_lpOrbit->GetSatelliteTLE( m_aulSatID[l], timeNow, &tleData );

				if( EMS_OK == hr  )
				{
					hr = m_lpOrbit->GetOrbitNumber( m_aulSatID[l], timeNow, &m_aulPassID[l] );

					hr = m_lpOrbit->InertialOrbitTLE( &tleData, &timeNow, 1, &timeCoord, &ulCount);
					hr = m_lpOrbit->EarthFixedOrbitTLE( &tleData, &timeNow, 1, &timeCoordEF, &ulCount);

					if ( EMS_OK == hr )
					{
						EMSVECTOR vDiff, vObserver;
						EMSESUVECTOR esuVector;
						
						vObserver = c_coordSys.ToInertialPosition( m_locnAntenna, timeNow );

						vDiff.fX = timeCoord.coord.radius.fX - vObserver.fX;
						vDiff.fY = timeCoord.coord.radius.fY - vObserver.fY;
						vDiff.fZ = timeCoord.coord.radius.fZ - vObserver.fZ;

						esuVector = c_coordSys.ToTopocentricHorizon( vDiff, m_locnAntenna, timeNow );

						azelVector = c_coordSys.ToAzimuthAndElevation( esuVector );

						if ( azelVector.azel.degElevation > cdSiteMinElevation )
						{
							aAzEls[ulActual].ulSatID = m_aulSatID[l];
							aAzEls[ulActual].ulPassID = m_aulPassID[l];
							aAzEls[ulActual].ulSatPriority = m_aulSatPriority[l];
							aAzEls[ulActual].degSatAzimuth = fmod(azelVector.azel.degAzimuth +360.0, 360.0);
							aAzEls[ulActual].degSatElevation = azelVector.azel.degElevation;
							memcpy( &aAzEls[ulActual].timeCoord, &timeCoordEF, sizeof(EMSTIMECOORD) );
							ulActual++;
						}
					}
				}
				else
				{
					//printf("\nError finding satellite %d.",m_aulSatID[l] );
//					hr = EMS_FALSE;
				}

			}
		}
		else
		{
			hr = EMS_NO_ORBIT_OBJ;
		}

		*pulActual = ulActual;
		if(m_bSun)
		{
			// Add sun vector
			EMSAZELVECTOR aSun;
			
			aSun = CEMSSun::SunPoint( timeNow, m_locnAntenna );

			if ( aSun.azel.degElevation > cdSiteMinElevation )
			{
				aAzEls[*pulActual].ulSatID = 999;
				aAzEls[*pulActual].ulSatPriority = 100;
				aAzEls[*pulActual].degSatAzimuth = fmod( aSun.azel.degAzimuth + 360.0, 360.0 );
				aAzEls[*pulActual].degSatElevation = aSun.azel.degElevation;
				(*pulActual)++;
			}

		}
	}

	return hr;
}

//---------------------------------------------------------------------------

EMS_RESULT
CDBFPointingAngles::ApplyPhaseBias( EMSDBFAZEL* aAzEls, ULONG* pulActual )
{
	EMS_RESULT hr = EMS_OK;

	if( !pulActual || !aAzEls )
	{
		hr = EMS_INVALID_PTR;
	}

	if ( EMS_OK == hr )
	{
	
		ULONG i,j;
		double dPhase;
		double dCosPhaseOffset, dSinPhaseOffset;
		double dCosPhase, dSinPhase;


		for( i = 0; i < *pulActual; i++ )
		{ 
			for( j = 0; j < DBF_MAX_CHANNELS; j++ )
			{
				if ( m_aDBFplate.wPlatePolarization )
					dPhase = -m_aDBFplate.emsDBFCells[j].fRHCP * c_dTwoPI/360.00;
				else
					dPhase = -m_aDBFplate.emsDBFCells[j].fLHCP * c_dTwoPI/360.00;

				dCosPhaseOffset = cos(dPhase);
				dSinPhaseOffset = sin(dPhase);
				//dSinPhaseOffset = -sin(dPhase);

				dCosPhase = aAzEls[i].acmplxPhase[m_aDBFplate.emsDBFCells[j].wChannelID].re;
				dSinPhase = aAzEls[i].acmplxPhase[m_aDBFplate.emsDBFCells[j].wChannelID].im;

				aAzEls[i].acmplxPhase[m_aDBFplate.emsDBFCells[j].wChannelID].re = dCosPhase*dCosPhaseOffset - dSinPhase*dSinPhaseOffset;
				aAzEls[i].acmplxPhase[m_aDBFplate.emsDBFCells[j].wChannelID].im = dCosPhase*dSinPhaseOffset + dSinPhase*dCosPhaseOffset;
			}
		}
	}

	return hr;
}

EMS_RESULT
CDBFPointingAngles::SetBiasOffset( const float* biasOffset)
{
	EMS_RESULT hr = EMS_OK;
	//memcpy( m_pafBiasOffset, biasOffset, 32 * sizeof(float) );
	m_pafBiasOffset = const_cast< float * > (biasOffset);
	return hr;
}

EMS_RESULT
CDBFPointingAngles::ApplyPhaseBias1( EMSDBFAZEL* aAzEls, ULONG* pulActual )
{
	EMS_RESULT hr = EMS_OK;

	if( !pulActual || !aAzEls )
	{
		hr = EMS_INVALID_PTR;
	}

	if ( EMS_OK == hr )
	{
	
		ULONG i,j;
		double dPhase;
		double dCosPhaseOffset, dSinPhaseOffset;
		double dCosPhase, dSinPhase;
		double dMag;


		for( i = 0; i < *pulActual; i++ )
		{ 
			for( j = 0; j < DBF_MAX_CHANNELS; j++ )
			{
				if ( m_aDBFplate.wPlatePolarization )
					dPhase = m_aDBFplate.emsDBFCells[j].fRHCP * c_dTwoPI/360.00;
				else if( m_pafBiasOffset )
					//dPhase = m_pafBiasOffset[j] *-1.0;
					dPhase = m_pafBiasOffset[j];
				else
					dPhase = m_aDBFplate.emsDBFCells[j].fLHCP * c_dTwoPI/360.00;


				dCosPhaseOffset = cos(dPhase);
				dSinPhaseOffset = sin(dPhase);
				//dSinPhaseOffset = -sin(dPhase);

				dCosPhase = aAzEls[i].acmplxPhase[m_aDBFplate.emsDBFCells[j].wChannelID].re;
				dSinPhase = aAzEls[i].acmplxPhase[m_aDBFplate.emsDBFCells[j].wChannelID].im;

				dMag = sqrt( dCosPhase*dCosPhase + dSinPhase*dSinPhase );
				if (dMag < 1.0e-6 ) dMag = 1.0e-6;
				dCosPhase /= dMag;
				dSinPhase /= dMag;


				aAzEls[i].acmplxPhase[m_aDBFplate.emsDBFCells[j].wChannelID].re = dCosPhase*dCosPhaseOffset - dSinPhase*dSinPhaseOffset;
				aAzEls[i].acmplxPhase[m_aDBFplate.emsDBFCells[j].wChannelID].im = dCosPhase*dSinPhaseOffset + dSinPhase*dCosPhaseOffset;

				//aAzEls[i].acmplxPhase[m_aDBFplate.emsDBFCells[j].wChannelID].im *= -1.0;
			}
		}
	}

	return hr;
}
//---------------------------------------------------------------------------

EMS_RESULT
CDBFPointingAngles::GetDBFPhase( EMSDBFAZEL* aAzEls, ULONG* pulActual )
{
	EMS_RESULT hr = EMS_OK;

	if( !pulActual || !aAzEls )
	{
		hr = EMS_INVALID_PTR;
	}

	if ( EMS_OK == hr )
	{
	
		ULONG i;

		for( i = 0; i < *pulActual; i++ )
		{ 
			hr = CellPhaseAngles( &aAzEls[i]);
			//hr = _CellPhaseAngles( (aAzEls+i));
		}
	}

	return hr;
}

EMS_RESULT
CDBFPointingAngles::GetSeparationAngles( EMSDBFAZEL* aAzEls, ULONG* pulActual )
{
	EMS_RESULT hr = EMS_OK;

	if( !pulActual )
	{
		hr = EMS_INVALID_PTR;
	}

	if( EMS_OK == hr )
	{

		ULONG i, j;
		EMSVECTORD vSat1, vSat2;
		double dMinSA, dSA;

		for( i = 0; i < *pulActual; i++ )
		{ 
			vSat1 = _Convert2UnitVector( aAzEls[i].degPlateAzimuth, aAzEls[i].degPlateElevation );

			dMinSA = 90.0;
			for( j = 0; j < *pulActual; j++ )
			{ 
				if ( !(i == j) )
				{
				
					vSat2 = _Convert2UnitVector( aAzEls[j].degPlateAzimuth, aAzEls[j].degPlateElevation );
					dSA = acos( vSat1.dX * vSat2.dX + vSat1.dY * vSat2.dY +vSat1.dZ * vSat2.dZ );
					dSA *= c_dRadToDeg;
					if ( dSA > 0 && dSA < dMinSA )
					{
						dMinSA = dSA;
					}
				}
			}
			aAzEls[i].degMinSeparation = dMinSA;

		}

	}

	return hr;
}

//

EMS_RESULT
CDBFPointingAngles::TestPhaseOutputPower( EMSTIME truetime, EMSDBFAZEL* aAzEls1, EMSDBFAZEL* aAzEls, ULONG* pulActual )
{
	EMS_RESULT hr = EMS_OK;

	if( !pulActual )
	{
		hr = EMS_INVALID_PTR;
	}


	if( EMS_OK == hr )
	{
		if ( *pulActual > 1 )
		{
			CEMSTime  oTime = truetime;
			EMSTIMEFIELDS tmFields;
			oTime.GetTime(&tmFields);

			double dTestPower;
			double dTest;
			EMSCOMPLEX cTest;
			EMSVECTORD vSat1, vSat2;
			double dSA, dSAmin;
			EMSCOMPLEX cValue[DBF_MAX_CHANNELS];
			EMSCOMPLEX cValue1[DBF_MAX_CHANNELS];

			long lGainEL[10] = {0,10,20,30,40,50,60,70,80,90};
			double dGainDB[10] = {-6.0,-3.0,0.0,2.0,3.5,4.5,5.0,5.5,5.95,5.98};
			long lElInt = 0;

			FILE* m_lpNull = fopen(CDBFPSConfig::Instance().NullPointingFile().c_str(),"at");
	
			for ( int i = 0; i < *pulActual; i++ )
			{
				vSat1 = _Convert2UnitVector( aAzEls[i].degPlateAzimuth, aAzEls[i].degPlateElevation );
				if (m_lpNull) fprintf( m_lpNull,"\n0, %04d/%02d/%02d,%02d:%02d:%02d",
				tmFields.nYear,
				tmFields.nMonth,
				tmFields.nDay,
				tmFields.nHour,
				tmFields.nMinute,
				tmFields.nSecond );
				dSAmin = 1000.0;

				printf( "0, %04d/%02d/%02d,%02d:%02d:%02d",
				tmFields.nYear,
				tmFields.nMonth,
				tmFields.nDay,
				tmFields.nHour,
				tmFields.nMinute,
				tmFields.nSecond );

				if (m_lpNull) fprintf(m_lpNull,", %3d, %3d, %7.2f, %7.2f", *pulActual, aAzEls[i].ulSatID, aAzEls[i].degPlateAzimuth, aAzEls[i].degPlateElevation );
				printf(", %3d, %3d, %7.2f, %7.2f **", *pulActual, aAzEls[i].ulSatID,aAzEls[i].degPlateAzimuth, aAzEls[i].degPlateElevation );
				
				for ( int j = 0; j < *pulActual; j++ )
				{
					emscbConj2 (aAzEls1[i].acmplxPhase, cValue, DBF_MAX_CHANNELS);
					emscbConj2 (aAzEls[j].acmplxPhase, cValue1, DBF_MAX_CHANNELS);
					cTest = emscDotProd( cValue, aAzEls[j].acmplxPhase, DBF_MAX_CHANNELS);
					cTest = emscDotProd( aAzEls1[i].acmplxPhase, aAzEls[j].acmplxPhase, DBF_MAX_CHANNELS);
					//cTest = emscDotProd( cValue, cValue1, DBF_MAX_CHANNELS);
					dTest = sqrt(cTest.re * cTest.re + cTest.im * cTest.im);

					if ( i == j )
					{
						dSA = 0.0;
					}
					else
					{
						vSat2 = _Convert2UnitVector( aAzEls[j].degPlateAzimuth, aAzEls[j].degPlateElevation );
						dSA = acos( vSat1.dX * vSat2.dX + vSat1.dY * vSat2.dY +vSat1.dZ * vSat2.dZ );
						dSA *= c_dRadToDeg;
						if ( dSA < dSAmin ) dSAmin = dSA;
					}
					//fprintf(m_lpNull,",  %8.4f",dTest );
	
					if (i==j)
					{
						lElInt = long(aAzEls[i].degPlateElevation/10.0);
						dTestPower = log10(dTest)*20 + dGainDB[lElInt] ;
						//printf(",  %8.4f",log10(dTest)*20 + dGainDB[lElInt] );
						printf(",  %8.4f",dTestPower );
						aAzEls[i].dGain =  dTestPower;
					}
					else
						printf(",  %8.4f",dTest );

				}

				aAzEls[i].degMinSeparation = dSAmin;
				//fprintf(m_lpNull,", %8.2f", dSAmin );
				printf(",** %8.2f\n", dSAmin );

				if (m_lpNull) fprintf(m_lpNull, "\n1, %04d/%02d/%02d,%02d:%02d:%02d",
				tmFields.nYear,
				tmFields.nMonth,
				tmFields.nDay,
				tmFields.nHour,
				tmFields.nMinute,
				tmFields.nSecond );
				if (m_lpNull) fprintf(m_lpNull,", %3d, %7.2f, %7.2f", aAzEls1[i].ulSatID,aAzEls1[i].degPlateAzimuth, aAzEls1[i].degPlateElevation );
				for ( int j = 0; j < 32; j++ )
				{
					if (m_lpNull) fprintf(m_lpNull,",  %9.5f, %9.5f",aAzEls1[i].acmplxPhase[j].re, aAzEls1[i].acmplxPhase[j].im );
				}

				if (m_lpNull) fprintf(m_lpNull, "\n2, %04d/%02d/%02d,%02d:%02d:%02d",
				tmFields.nYear,
				tmFields.nMonth,
				tmFields.nDay,
				tmFields.nHour,
				tmFields.nMinute,
				tmFields.nSecond );
				if (m_lpNull) fprintf(m_lpNull,", %3d, %7.2f, %7.2f", aAzEls[i].ulSatID,aAzEls[i].degPlateAzimuth, aAzEls[i].degPlateElevation );
				for ( int j = 0; j < 32; j++ )
				{
					if (m_lpNull) fprintf(m_lpNull,", %9.5f, %9.5f",aAzEls[i].acmplxPhase[j].re, aAzEls[i].acmplxPhase[j].im );
				}

			}
			if (m_lpNull) { fflush( m_lpNull ); fclose( m_lpNull ); m_lpNull = NULL; }
		}
	}
	return hr;
}

EMS_RESULT 
CDBFPointingAngles::PriorityOrder( EMSDBFAZEL* aAzEls, ULONG* pulActual )
{
	EMS_RESULT hr = EMS_OK;

	if( !pulActual )
	{
		hr = EMS_INVALID_PTR;
	}
	if( EMS_OK == hr )
	{
		
		EMSDBFAZEL aAzElTemp;
		if ( *pulActual > 1 )
		{
			for ( int i = 0; i < *pulActual-1; i++ )
			{
				for ( int j = i+1; j < *pulActual; j++ )
				{
					if ( aAzEls[i].dGain < aAzEls[j].dGain )
					{
						memcpy( &aAzElTemp, &aAzEls[i], sizeof(EMSDBFAZEL) );
						memcpy(  &aAzEls[i],  &aAzEls[j], sizeof(EMSDBFAZEL) );
						memcpy( &aAzEls[j], &aAzElTemp, sizeof(EMSDBFAZEL) );
					}
				}
			}
		}
		// RR: Remove satellites with low priorities
		ULONG ulCount = 0;
		double dMinGain = 5;
		for ( int i = 0; i < *pulActual; i++ )
		{
			if ( aAzEls[i].dGain > dMinGain )
				ulCount++;

		}
		*pulActual = ulCount;

	}
	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EMS_RESULT
CDBFPointingAngles::GetSatelliteVectorEF( EMSTIME timeNow, ULONG ulSatID, EMSTIMECOORD* timeCoord )
{
	EMS_RESULT hr = EMS_OK;
	if ( m_lpOrbit )
	{
		ULONG ulCount;
		EMSTLEDATA tleData;
		EMSTIMECOORD timeCoord1;
		memset( &tleData, 0, sizeof(EMSTLEDATA) );

		hr = m_lpOrbit->GetSatelliteTLE( ulSatID, timeNow, &tleData );

		if( EMS_OK == hr  )
		{
			hr = m_lpOrbit->EarthFixedOrbitTLE( &tleData, &timeNow, 1, &timeCoord1, &ulCount);
			timeCoord->coordSystem = timeCoord1.coordSystem;
			timeCoord->vTime.intTime = timeCoord1.vTime.intTime;
			memcpy( &timeCoord->coord.radius, &timeCoord1.coord.radius, sizeof(EMSVECTOR) );
			memcpy( &timeCoord->coord.velocity, &timeCoord1.coord.velocity, sizeof(EMSVECTOR) );
		}


	}
	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EMS_RESULT
CDBFPointingAngles::GetConstellation( ULONG ulSatID, double* dWaveLength, double* dDownFrequency )
{
	EMS_RESULT	hr = EMS_BAD_PARAM;
	double dFrequency = 0.0;

	if (ulSatID > 400 && ulSatID < 500 ) dFrequency = 1544.10e6;
	if (ulSatID > 500 && ulSatID < 600 ) dFrequency = 1544.90e6;
	if (ulSatID > 600 && ulSatID < 700 ) dFrequency = 1544.21e6;
	if (ulSatID > 700 && ulSatID < 800 ) dFrequency = 1544.80e6;
	
	if ( dFrequency > 0.0 )
	{
		*dWaveLength = (c_dVelocityOfLight / dFrequency) * 10000;
		*dDownFrequency = dFrequency;
		hr = EMS_OK;
	}
	else
	{
		*dWaveLength = 0.0;
		*dDownFrequency = 0.0;
		hr = EMS_BAD_PARAM;
	}
	return hr;
}
