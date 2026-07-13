/********************************************************************
*	Module:			OrbitObj.cpp
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
********************************************************************/

/*******************************************************************

  Revision Record

	Rev	Date			Auth	Changes
	===	====			====	=======

  0.0 01/05/10	    rvw 	start

********************************************************************/
#include <stdio.h>
#include <math.h>
#include <io.h>
#include <tchar.h>
#include <ctype.h>

#include "emsvectr.h"
#include "emscom.h"
#include "emstime.h"
#include "emscoord.h"
#include "orbitobj.h"
#include "emsconfg.h"
//#include "cmmlib.h"
#include "emsconst.h"


extern HMODULE g_hDLLModule;

#define SIGN(a)       ((a > 0) ? -1.0 : 1.0)

#include "orbitfns\sgpxstr.h"

#define DMOD(a,b)      ((a) - (long int)((a)/(b)) * (b))

#define MAX_EMS_ORBIT_DATA	(128)	// increased for MEOs, etc
//#define MAX_EMS_ORBIT_DATA	(32)

const TCHAR c_szTLEFileNameFmt[] 	= TEXT("%sEMS-SatTLE-%03ld.dat");
const TCHAR c_szOrbitMemoryName[] 	= TEXT("EMSOrbitData");
const TCHAR c_szOrbitFile[] 			= TEXT("sarsat.txt");
const TCHAR c_szINIField[]			= TEXT("inifile");
const TCHAR c_szMutexName[]  		= TEXT("EMSOrbitMutex");
const int c_nMaxElementNumber 	 	= 9999;
const TCHAR c_szLine1Fmt[ ] = TEXT("1 %05dU EMS600   %02d%012.8lf  .%08ld  %s  %s 0 %4d");
const TCHAR c_szLine2Fmt[ ] = TEXT("2 %05d %8.4lf %8.4lf %07ld %8.4lf %8.4lf %11.8lf%05d");
const TCHAR c_szSharedMemoryName[]=TEXT("Global\\OrbitMappingObject");

static double _EncodeEpochTime( EMSTIME timeEMS )
{
	double dEpoch = 0.0;
	CEMSTime timeEpoch( timeEMS );
	EMSTIMEFIELDS timeFields;
	int nDaysInYear = timeEpoch.IsLeapYear() ? 366 : 365;

	timeEpoch.GetTime( &timeFields );

	if ( timeFields.nJulianDay >= nDaysInYear )
	{
		timeFields.nYear += 1;
		timeFields.nJulianDay -= nDaysInYear;
	}

	if ( timeFields.nYear >= 2000 )
		timeFields.nYear -= 2000;
	else
		timeFields.nYear -= 1900;

	dEpoch = (double)timeFields.nYear * 1000.0;

	dEpoch += ( double )timeFields.nJulianDay;

	if ( timeFields.nHour )
		dEpoch += (double)timeFields.nHour / 24.0;

	if ( timeFields.nMinute )
		dEpoch += ( (double)timeFields.nMinute / 60.0 ) / 24.0;

	if ( timeFields.nSecond )
		dEpoch +=  ( ( (double)timeFields.nSecond / 60.0 ) / 60.0 ) / 24.0;

	if ( timeFields.lNanosecond )
		dEpoch += ( ( ( (double)timeFields.lNanosecond / 1000000000.0 ) / 60.0 ) / 60.0 ) / 24.0;

	return dEpoch;
}

// don't want to expose the TTLEVector data type in the interface
//
static TTLEVector _ConvertTLE( EMSTLEDATA emsTLE )
{
	TTLEVector tleVector;

	memset( &tleVector, 0, sizeof( tleVector ) );

	tleVector.epoch = _EncodeEpochTime( emsTLE.timeEpoch );
	tleVector.xndt2o = emsTLE.fXndt2o; 
	tleVector.xndd6o	= emsTLE.fXndd6o;
	tleVector.iexp = 0;
	tleVector.xincl = emsTLE.elem.var.fInclination;
	tleVector.bstar = emsTLE.fBStar;
	tleVector.ibexp = 0;
	tleVector.xnodeo = emsTLE.elem.var.fRightAscNode;
	tleVector.eo 		= emsTLE.elem.var.fEccentricity;
	tleVector.omegao	= emsTLE.elem.var.fArgPerigee;
	tleVector.xmo		= emsTLE.elem.var.fMeanAnomaly;
	tleVector.xno		= emsTLE.elem.var.fMeanMotion;
	tleVector.norbt	= emsTLE.nOrbitNumber;

	return tleVector;
}

CEMSOrbitObject::CEMSOrbitObject( ) : CApiObjBase("CEMSOrbitObject")
{
	m_lpOrbitData = 0;
	m_bInitialized = FALSE;
	m_lpSatelliteInfo = 0;
	m_lpszFilespec = 0;
	m_lpszPath = 0;

	m_hMapFile = NULL;

	m_mutex.Create( c_szMutexName );
}

CEMSOrbitObject::~CEMSOrbitObject() 
{
	if ( m_lpszPath )
	{
		delete [] m_lpszPath;
		m_lpszPath = 0;
	}

	if ( m_lpszFilespec )
	{
		delete [ ] m_lpszFilespec;
		m_lpszFilespec = 0;
	}

	if ( m_lpOrbitData )
	{
		_DeallocateOrbitData( );

		m_lpOrbitData = 0;
	}

	if ( m_lpSatelliteInfo )
	{
		m_lpSatelliteInfo->Release();
		m_lpSatelliteInfo = 0;
	}

	m_mutex.Close();

	if ( m_bInitialized )
	{
		m_bInitialized = FALSE;
	}
}

HRESULT CEMSOrbitObject::Init( void )
{
	HRESULT hr = EMS_OK;

	if ( !m_bInitialized )
    {

		hr = CoCreateInstance( CLSID_EMSSatelliteInfo, 0, CLSCTX_ALL, IID_IEMSSatelliteInfo, 
							(LPVOID*)&m_lpSatelliteInfo );

		try
		{
			if ( hr == EMS_OK )
			{
				hr = _SetPath();

				if( hr == EMS_OK )
				{
					hr = _AllocateOrbitDataX();
					if ( hr == EMS_OK )
					{
						m_bInitialized = TRUE;
					}
				}
			}
			else
			{
				hr = EMS_NO_SATINFO_OBJ;
			}
		}
		catch( ... )
		{
			hr = EMS_EXCEPTION;
		}
	}

	if( FAILED(hr) && m_lpSatelliteInfo )
	{
		m_lpSatelliteInfo->Release();
		m_lpSatelliteInfo = 0;
	}
	return hr;
}


STDMETHODIMP_(ULONG) CEMSOrbitObject::AddRef()
{
	return IncrRefCount();
}

STDMETHODIMP_(ULONG) CEMSOrbitObject::Release()
{
	if (DecrRefCount() == 0)
	{
		delete this;
		return 0;
	}
	return GetRefCount();
}
	
STDMETHODIMP CEMSOrbitObject::QueryInterface(REFIID riid, LPVOID *lppObj)
{
	EMS_RESULT result = EMS_NOINTERFACE;
	LPUNKNOWN lpUnknown = 0;

	if (lppObj)
		*lppObj = lpUnknown;

	if (IsEqualIID(riid, IID_IEMSOrbit))
	{
		lpUnknown = (LPUNKNOWN)(LPEMSORBIT)this;
	}
	else if (IsEqualIID(riid, IID_IEMSOrbit2))
	{
		lpUnknown = (LPUNKNOWN)(LPEMSORBIT2)this;
	}
	else if (IsEqualIID(riid, IID_IEMSOrbit4))
	{
		lpUnknown = (LPUNKNOWN)(LPEMSORBIT4)this;
	}
	else if (IsEqualIID(riid, IID_IEMSOrbitInit))
	{
		lpUnknown = (LPUNKNOWN)(LPEMSORBITINIT)this;
	}
	else if (IsEqualIID(riid, IID_IUnknown))
	{
		lpUnknown = (LPUNKNOWN)(LPEMSORBIT)this;
	}

	if (lpUnknown)
	{
		if (lppObj)
		{
			*lppObj = lpUnknown;

			lpUnknown->AddRef();
		}

		result = EMS_OK;
	}

	return result;
}

STDMETHODIMP CEMSOrbitObject::EarthFixedOrbit( ULONG ulSatelliteID,
				 const EMSTIME __RPC_FAR *pvTime, ULONG cbCount,
				 EMSTIMECOORD __RPC_FAR *pvCoord, ULONG __RPC_FAR *pcbResult)
{
	CEMSCoordSystem coordSys;

	HRESULT hr = InertialOrbit( ulSatelliteID, pvTime, cbCount, 
						pvCoord, pcbResult );

	if ( (hr == EMS_OK) && pcbResult && ( *pcbResult > 0 ) )
	{
		for ( int i = 0; i < (int)*pcbResult; i++ )
		{
			pvCoord[i] = coordSys.ToEarthFixed( pvCoord[i] );
		}
	}
	return hr;
}

STDMETHODIMP CEMSOrbitObject::EarthFixedOrbitTLE( EMSTLEDATA __RPC_FAR *lpTLE,
				 const EMSTIME __RPC_FAR *pvTime, ULONG cbCount, 
				 EMSTIMECOORD __RPC_FAR *pvCoord, ULONG __RPC_FAR *pcbResult )
{
	CEMSCoordSystem coordSys;

	HRESULT hr = InertialOrbitTLE( lpTLE, pvTime, cbCount, pvCoord, pcbResult );

	if ( (hr == EMS_OK) && pcbResult && ( *pcbResult > 0 ) )
	{
		for ( int i = 0; i < (int)*pcbResult; i++ )
		{
			pvCoord[i] = coordSys.ToEarthFixed( pvCoord[i] );
		}
	}
	return hr;
}

static double _Angle360( double dAngle )
{
	if ( dAngle < 0.0 )
		dAngle += 360.0;

	return dAngle;
}

static ULONG _CalcOrbitNumber( ULONG ulOrigNumber, TCLVector clVectorEpoch,
						TCLVector clVector, double dTimeDifference )
{
 	double dTerm1, dTerm2, dTerm3, dTerm4;
	ULONG ulNewNumber;

 	dTerm1 = (double)ulOrigNumber;

 	dTerm2 = ( dTimeDifference * 60e0 ) / clVectorEpoch.PERIOD;

 	dTerm3 = DMOD ( _Angle360( clVectorEpoch.ANOM ) + _Angle360( clVectorEpoch.PERI ), 360e0) / 360e0;

 	dTerm4 =  DMOD ( _Angle360( clVector.ANOM ) + _Angle360( clVector.PERI ), 360e0) / 360e0 ;

	ulNewNumber = (ULONG)( dTerm1 + dTerm2 + dTerm3 - dTerm4 + 0.5e0 );

	return ulNewNumber;
}

STDMETHODIMP CEMSOrbitObject::GetOrbitNumber( ULONG ulSatelliteID, 
			const EMSTIME eTime, ULONG __RPC_FAR *lpulOrbitNumber )
{
	HRESULT hr = EMS_OK;
	ULONG ulOrbitNumber = 0;

	if ( !m_bInitialized )
		hr = Init();

	if ( hr == EMS_OK )
	{
		int nIndex = _FindOrbitData( (int)ulSatelliteID );

		if ( nIndex != -1 )
		{
			LPORBITDATA lpData = (LPORBITDATA)m_lpOrbitData;
			TTLEVector tleVector = _ConvertTLE( lpData[nIndex].emsTLE.tle );
			TOrbit tempObj( &tleVector, 2 );
			TEFVector efVector;
			CEMSTime epochTime( lpData[nIndex].emsTLE.tle.timeEpoch );
			float fDifference = (float)epochTime.MinutesDifferent( eTime );
			TINVector inVector = tempObj.Predict( fDifference );
	        TCLVector clVector = TConvert::ToCL( &inVector );
			TINVector inVectorEpoch = tempObj.Predict( 0e0 );
	        TCLVector clVectorEpoch = TConvert::ToCL( &inVectorEpoch );

//			float ts = 0.0;
//
//			double dTerm1, dTerm2, dTerm3, dTerm4;
//
//			dTerm1 = (double) lpData[nIndex].emsTLE.tle.nOrbitNumber;
//			dTerm2 = ( fDifference * 60e0 ) / clVectorEpoch.PERIOD;
//			dTerm3 = DMOD ( clVectorEpoch.ANOM + clVectorEpoch.PERI, 360e0) / 360e0;
//			dTerm4 =  DMOD ( clVector.ANOM  + clVector.PERI , 360e0) / 360e0 ;
//
//			ulOrbitNumber = (ULONG)( dTerm1 + dTerm2 + dTerm3 - dTerm4 + 0.5e0 );

			ulOrbitNumber = _CalcOrbitNumber( lpData[nIndex].emsTLE.tle.nOrbitNumber,
										clVectorEpoch, clVector, fDifference );
		}
		else
			hr = EMS_ORBIT_NO_SATELLITE;
	}

	if ( lpulOrbitNumber )
		*lpulOrbitNumber = ulOrbitNumber;

	return hr;
}

STDMETHODIMP CEMSOrbitObject::InertialOrbit( ULONG ulSatelliteID,
				 const EMSTIME __RPC_FAR *pvTime, ULONG cbCount,
				 EMSTIMECOORD __RPC_FAR *pvCoord, ULONG __RPC_FAR *pcbResult)
{
	HRESULT hr = EMS_OK;
	int nIndex = -1;
	ULONG ulReturned = 0;

	if ( !m_bInitialized)
		hr = Init();

	if ( hr == EMS_OK )
	{
		ORBITDATA orbitData;

		if ( _GetOrbitData( (int)ulSatelliteID, &orbitData ) )
		{
			hr = InertialOrbitTLE( &orbitData.emsTLE.tle, pvTime, cbCount,
							pvCoord, &ulReturned );
		}
		else
			hr = EMS_ORBIT_NO_SATELLITE;
	}

	if ( pcbResult )
		*pcbResult = ulReturned;

	return hr;
}
        
STDMETHODIMP CEMSOrbitObject::InertialOrbitTLE( EMSTLEDATA __RPC_FAR *lpTLE,
				 const EMSTIME __RPC_FAR *pvTime, ULONG cbCount, 
            EMSTIMECOORD __RPC_FAR *pvCoord, ULONG __RPC_FAR *pcbResult )
{
	EMS_RESULT hr = EMS_OK;
	ULONG ulReturned = 0;

    if( lpTLE )
	{
		if ( !m_bInitialized )
			hr = Init();

		if ( hr == EMS_OK ) 
		{
			TTLEVector tleVector = _ConvertTLE( *lpTLE );
			TOrbit tempObj( &tleVector, 2 );
			TINVector inVector;
			TEFVector efVector;
			CEMSTime epochTime( lpTLE->timeEpoch );
			float fDifference;

			for ( int i = 0; pvCoord && ( i < (int)cbCount ); i++ )
			{
				fDifference = (float)epochTime.MinutesDifferent( pvTime[i] );

				inVector = tempObj.Predict( fDifference );

				if ( pvCoord )
				{
					pvCoord[i].coordSystem = COORDSYS_Inertial;
					pvCoord[i].vTime = pvTime[i];
					pvCoord[i].coord.radius.fX = (float)inVector.Rx;
					pvCoord[i].coord.radius.fY = (float)inVector.Ry;
					pvCoord[i].coord.radius.fZ = (float)inVector.Rz;
					pvCoord[i].coord.velocity.fX = (float)inVector.Vx;
					pvCoord[i].coord.velocity.fY = (float)inVector.Vy;
					pvCoord[i].coord.velocity.fZ = (float)inVector.Vz;

					ulReturned++;
				}
			}
		}
	}
	else
		hr = EMS_BAD_PARAM;

	if ( pcbResult )
		*pcbResult = ulReturned;

	return hr;
}

        
STDMETHODIMP CEMSOrbitObject::SetSatelliteTLE( ULONG ulSatelliteID,
            const EMSTIME eTime, const EMSTLEDATA __RPC_FAR *lpTLEData )
{
	EMS_RESULT hr = EMS_OK;
	EMSTLEDATA2 tleData2;

	if ( lpTLEData )
	{
		memset( &tleData2, 0, sizeof( tleData2 ) );

		tleData2.tle = *lpTLEData;

		tleData2.timeEffective = eTime;

		tleData2.timeModified = eTime;

		hr = SetSatelliteTLE3( ulSatelliteID, &tleData2, 0 );
	}
	else
		hr = EMS_BAD_PARAM;

	return hr;
}

STDMETHODIMP CEMSOrbitObject::GetSatelliteTLE( ULONG ulSatelliteID,
            const EMSTIME eTime, EMSTLEDATA __RPC_FAR *lpTLEData )
{
	EMS_RESULT hr = EMS_OK;
	EMSTLEDATA2 tleData2;

	memset( &tleData2, 0, sizeof( tleData2 ) );

	hr = GetSatelliteTLE2( ulSatelliteID, eTime, &tleData2 );

	if ( lpTLEData )
		*lpTLEData = tleData2.tle;

	return hr;
}

int CEMSOrbitObject::_FindEmptySlot( void )
{
	int nIndex = -1;

	if ( m_lpOrbitData )
	{
		LPORBITDATA lpData = (LPORBITDATA)m_lpOrbitData;
		int i = 0;

		while ( ( nIndex == -1 ) && ( i < MAX_EMS_ORBIT_DATA ) )
		{
			if ( !lpData[i].bInUse )
				nIndex = i;
			else
				i++;
		}
	}

	return nIndex;
}

BOOL CEMSOrbitObject::_GetOrbitData( int nID, LPORBITDATA lpOrbitData, BOOL bUseNorad )
{
	BOOL bOK = FALSE;

	if ( lpOrbitData )
	{
		if ( m_mutex.Lock() )
		{
			int nIndex = _FindOrbitData( nID, bUseNorad );

			if ( nIndex != -1 )
			{
				LPORBITDATA lpData = (LPORBITDATA)m_lpOrbitData;

				*lpOrbitData = lpData[ nIndex ];

				bOK = TRUE;
			}
			m_mutex.Unlock();
		}
	}

	return bOK;
}

int CEMSOrbitObject::_FindOrbitData( int nID, BOOL bUseNorad /*=FALSE*/ )
{
	int nIndex = -1;

	if ( m_lpOrbitData )
	{
		LPORBITDATA lpData = (LPORBITDATA)m_lpOrbitData;
		int i = 0;

		while ( ( nIndex == -1 ) && ( i < MAX_EMS_ORBIT_DATA ) )
		{
			if ( lpData[i].bInUse )
			{
				if ( ( bUseNorad && ( lpData[i].nNoradNumber == nID ) ) ||
					( !bUseNorad && ( lpData[i].nSatelliteID == nID ) ) )
					nIndex = i;
			}
			i++;
		}
	}

	return nIndex;
}

BOOL CEMSOrbitObject::_ValidName( LPCTSTR lpcszSatName )
{
	BOOL bValid = FALSE;

	if ( lpcszSatName && *lpcszSatName && isalnum( *lpcszSatName ) )
	{	
		bValid = TRUE;
	}

	return bValid;
}

/*
 *  EPOCH	Epoch time (Julian date)
 *  XNDT2O	First time derivative of Mean Motion (rev/day**2)
 *  XNDD6O	Second time derivative of Mean Motion (rev/day**3)
 *  IEXP
 *  BSTAR	Drag term if SGP4 is used, otherwise, radiation
 *              pressure coeeficient
 *  IBEXP
 *  XINCL	Inclination (degs)
 *  XNODEO	Right ascension of the ascending node (degs)
 *  EO		Eccentricity
 *  OMEGAO	Argument of perigee (degs)
 *  XMO		Mean anomaly (degs)
 *  XNO		Mean motion (revs/day)
 */

HRESULT CEMSOrbitObject::_InitDataFromFile( LPCTSTR lpcszFilename )
{
	HRESULT hr = EMS_UNKNOWN_ERROR;

	if ( lpcszFilename && *lpcszFilename )
	{
		FILE *pFile = fopen( lpcszFilename, "rt" );

		try
		{
			if ( pFile )
			{
				char szLine1[256], szLine2[256], szSatelliteName[256];
				EMSTLEDATA tleData;
				int i, nNoradNumber, nIndex, nElementNumber=0;
				double dEpoch;
				LPORBITDATA lpData = (LPORBITDATA)m_lpOrbitData;

				hr = EMS_OK;

				while ( !feof( pFile ) && ( hr == EMS_OK ) )
				{
					if ( fgets( szSatelliteName, sizeof( szSatelliteName ), pFile ) )
					{	
						if ( _ValidName( szSatelliteName ) && fgets( szLine1, sizeof( szLine1 ), pFile ) )
						{	
							TCHAR cCheck1 = _CalcChecksum( szLine1 );

							if ( ( lstrlen( szLine1 ) > 68 ) && ( cCheck1 == szLine1[68] ) )
							{
								if ( fgets( szLine2, sizeof( szLine2 ), pFile ) )
								{	// line 2
									TCHAR cCheck2 = _CalcChecksum( szLine2 );

									if ( ( lstrlen( szLine2 ) > 68 ) && ( cCheck2 == szLine2[68] ) )
									{
										memset( &tleData, 0, sizeof( tleData ) );
															
										for (i = 18; i < 32; i++)
										{
											if (szLine1[i] == ' ')
												szLine1[i] = '0';	/* No blank between year and days */
										}

										sscanf (&szLine1[18], "%14lf", &dEpoch);

										sscanf (&szLine1[33], "%10lf",  &tleData.fXndt2o);

										sscanf (&szLine1[44], "%6lf",   &tleData.fXndd6o);  
										tleData.fXndd6o /= 100000.0;

										int nExp = 0;

										sscanf (&szLine1[50], "%2d",   &nExp  );

										if ( nExp != 0 )
											tleData.fXndd6o *= pow( 10.0 , nExp );

										sscanf (&szLine1[53], "%6lf",   &tleData.fBStar ); 
										tleData.fBStar  /= 100000.0;

										sscanf (&szLine1[59], "%2d",   &nExp );

										if ( nExp != 0 )
											tleData.fBStar *= pow( 10.0 , nExp );

										for ( i = 64; i < 68; i++ )
										{
											if (szLine1[i] == ' ')
  											szLine1[i] = '0';  /* No blank at the beginning of orbit number */
										}
										sscanf( &szLine1[64], "%4d", &nElementNumber );

										szLine2[25] = '.';	/* Decimal point for eccentricity */

										for ( i = 63; i < 68; i++ )
										{
											if (szLine2[i] == ' ')
  											szLine2[i] = '0';  /* No blank at the beginning of orbit number */
										}
										sscanf (&szLine2[ 2], "%5d",   &nNoradNumber );
										sscanf (&szLine2[ 8], "%8lf",   &tleData.elem.var.fInclination );
										sscanf (&szLine2[17], "%8lf",   &tleData.elem.var.fRightAscNode);
										sscanf (&szLine2[25], "%8lf",   &tleData.elem.var.fEccentricity );
										sscanf (&szLine2[34], "%8lf",   &tleData.elem.var.fArgPerigee );
										sscanf (&szLine2[43], "%8lf",   &tleData.elem.var.fMeanAnomaly );
										sscanf (&szLine2[52], "%11lf",  &tleData.elem.var.fMeanMotion );
										sscanf (&szLine2[63], "%5d",   &tleData.nOrbitNumber );

										nIndex = _FindOrbitData( nNoradNumber, TRUE );

										if ( nIndex == - 1)
											nIndex = _FindEmptySlot();

										if ( nIndex != -1 )
										{
											INT nSatID = _SatIDFromNoradNumber( nNoradNumber );

											if ( nSatID > 0 ) // if not in .INI - will be zero
											{
												_RemoveTrailingUnprintables( szSatelliteName );

												lpData[nIndex].bInUse = TRUE;
												lpData[nIndex].nElementNumber = nElementNumber;
												lpData[nIndex].emsTLE.tle = tleData;
												lpData[nIndex].nNoradNumber = nNoradNumber;
												lpData[nIndex].nSatelliteID = nSatID;

												lstrcpyn( lpData[nIndex].szName, szSatelliteName,
																				EMS_MAX_SAT_NAME_LENGTH );

												lpData[nIndex].emsTLE.tle.timeEpoch = _ConvertEpochTime( dEpoch );
											}
										}
									}
									else
									{
										hr = EMS_ORBIT_BAD_DATA;
									}
								}
								
							}
							else
							{
								hr = EMS_ORBIT_BAD_DATA;
							}
						}	
					}
				}
				fclose( pFile );
				pFile = NULL;
			}
			else
			{
				hr = EMS_NO_FILE;
			}

		}
		catch( ... )
		{
			hr = EMS_EXCEPTION;
		}
		// cleanup
		if( pFile )
		{
			fclose( pFile );
			pFile = NULL;
		}
	}
	else
	{
		hr = EMS_ORBIT_INVALID_FILESPEC;
	}


	return hr;
}

EMS_RESULT CEMSOrbitObject::_SetPath( void )
{
	EMS_RESULT hr = EMS_OK;
	TCHAR szPath[ 256 ]="", szFile[ 256 ] = "";
	CEMSRegistryConfig reg;

	reg.SetComponentName( c_szComponentName );

	if ( reg.OpenKey( ) )
	{
		reg.QueryValue( c_szINIField, szFile, sizeof( szFile ) / sizeof( szFile[0] ) );

		reg.CloseKey( );
	}

	if ( szFile[0] == 0 )	// entry not found in the registry, so set it up
	{
		if ( reg.FindFile( szPath, sizeof( szPath ) / sizeof( szPath[0] ),
				(HANDLE)g_hDLLModule, c_szOrbitFile ) == EMS_OK )
		{
			wsprintf( szFile, TEXT("%s%s"), szPath, c_szOrbitFile );

			if ( reg.CreateKey( ) )
			{
				reg.SetValue( c_szINIField, szFile );
				reg.CloseKey();
			}
		}
		else
		{
			hr = EMS_ORBIT_INVALID_FILESPEC;
		}
	}

	if ( SUCCEEDED(hr) && szFile[0] )
	{
		if ( _access( szFile, 0 ) == 0 )
		{
			m_lpszFilespec = new TCHAR[ lstrlen( szFile ) + 1 ];

			if ( m_lpszFilespec )
			{
				TCHAR szDrive[ _MAX_DRIVE ], szTemp[ _MAX_PATH ];

				lstrcpy( m_lpszFilespec, szFile );

				_splitpath( szFile, szDrive, szTemp, 0 , 0 );

				wsprintf( szPath, TEXT("%s%s"), szDrive, szTemp );
			}
			else
			{
				hr = EMS_ORBIT_NO_MEMORY;
			}
		}
		else
		{
			hr = EMS_ORBIT_INVALID_FILESPEC;
		}
	}

	if( SUCCEEDED(hr) && szPath[0] )
    {
		m_lpszPath = new TCHAR [ lstrlen( szPath ) + 1 ];

		if ( m_lpszPath )
			lstrcpy( m_lpszPath, szPath );
		else
			hr = EMS_ORBIT_NO_MEMORY;
	}

	return hr;
}

HRESULT CEMSOrbitObject::_InitData( void )
{
	EMS_RESULT hr = EMS_UNKNOWN_ERROR;

	if ( m_lpOrbitData )
	{
		LPORBITDATA lpData = (LPORBITDATA) m_lpOrbitData;

		memset( m_lpOrbitData, 0, sizeof( ORBITDATA ) * MAX_EMS_ORBIT_DATA );

		if ( m_lpszFilespec )
		{
			hr = _InitDataFromFile( m_lpszFilespec );

			if ( hr == EMS_OK )
			{
				ORBITDATA tempData;

				for ( int i = 0; i < MAX_EMS_ORBIT_DATA; i++ )
				{
					if ( lpData[ i ].bInUse )
					{
						if ( _ReadTLEFromFile( lpData[i].nSatelliteID, &tempData ) == EMS_OK )
						{
							if ( tempData.emsTLE.tle.timeEpoch.intTime > lpData[i].emsTLE.tle.timeEpoch.intTime )
							{
								lpData[i] = tempData;
								lpData[i].bInUse = TRUE;
							}
						}
					}
				}
			}
		}
		else 
			hr = EMS_ORBIT_INVALID_FILESPEC;
	}
	else
		hr = EMS_ORBIT_NO_MEMORY;

	return hr;
}

int CEMSOrbitObject::_SatIDFromNoradNumber( int nNoradNumber )
{
	int nID = 0;

	if ( m_lpSatelliteInfo )
	{
		ULONG ulID = 0, ulNoradID = nNoradNumber;
		HRESULT hr = m_lpSatelliteInfo->GetSatelliteID( ulNoradID, &ulID );

		if ( hr == EMS_OK )
			nID = (int)ulID;
	}

	return nID;
}

int CEMSOrbitObject::_RemoveTrailingUnprintables( LPTSTR lpszText )
{
	int nLength = ( lpszText ) ? lstrlen( lpszText ) : 0;

	while (( nLength > 0 ) && ( lpszText[nLength-1] <= 32 ) )
	{
		lpszText[ --nLength ] = 0;
	}

	return nLength;
}

void CEMSOrbitObject::_ToTextExpForm( double dValue, LPTSTR lpszResult )
{
	if ( lpszResult )
	{
		TCHAR szTemp[64];
		TCHAR *lpcE = 0;

		sprintf( szTemp, TEXT("%e"), dValue );

		lpcE = _tcschr( szTemp, 'e' );

		if ( lpcE )
		{
			int nExp; 

			*lpcE = 0;

			nExp = atoi( lpcE + 1 );

			if ( nExp != 0 )
				nExp++;

			sprintf( lpszResult, TEXT("%05d%c%d"), (long)( 10000.0 * atof( szTemp ) ), 
							( nExp < 0 ) ? '-' : '+', abs( nExp ) );
		}
		else
			*lpszResult = 0;
	}
}

EMS_RESULT CEMSOrbitObject::_UpdateTLERecord( LPORBITDATA lpSatData, LPVOID lpFile )
{
	EMS_RESULT hr = EMS_OK;

	if ( lpSatData && lpFile )
	{
		FILE *lpFPtr = (FILE*)lpFile;
		EMSTIMEFIELDS timeFields;
		CEMSTime timeEpoch( lpSatData->emsTLE.tle.timeEpoch );
		double dJulianDays = timeEpoch.JulianDayOfTheYear();
		int nEncodedYear = 0;
		TCHAR szBuffer[256], cCheckDigit, szXndd6o[32], szBStar[32];
		double dTemp = 0.0;

		timeEpoch.GetTime( &timeFields );

		if ( timeFields.nYear >= 2000 )
			nEncodedYear = timeFields.nYear - 2000;
		else
			nEncodedYear = timeFields.nYear - 1900;

		fprintf( lpFPtr, "%s", lpSatData->szName );
		fprintf( lpFPtr, "\n" );

		_ToTextExpForm( lpSatData->emsTLE.tle.fXndd6o, szXndd6o );

		_ToTextExpForm( lpSatData->emsTLE.tle.fBStar, szBStar );

		sprintf( szBuffer, c_szLine1Fmt, lpSatData->nNoradNumber,
							nEncodedYear, dJulianDays, 
							(long)(lpSatData->emsTLE.tle.fXndt2o * 100000000.0),
							szXndd6o, szBStar, lpSatData->nElementNumber );

		cCheckDigit = _CalcChecksum( szBuffer );

		fprintf( lpFPtr, TEXT("%s%c\n"), szBuffer, cCheckDigit );

		dTemp = lpSatData->emsTLE.tle.elem.var.fEccentricity * 10000000;

		sprintf( szBuffer, c_szLine2Fmt, lpSatData->nNoradNumber,
							lpSatData->emsTLE.tle.elem.var.fInclination,
							lpSatData->emsTLE.tle.elem.var.fRightAscNode,
							(long)dTemp,
							lpSatData->emsTLE.tle.elem.var.fArgPerigee,
							lpSatData->emsTLE.tle.elem.var.fMeanAnomaly,
							lpSatData->emsTLE.tle.elem.var.fMeanMotion,
							lpSatData->emsTLE.tle.nOrbitNumber );

		cCheckDigit = _CalcChecksum( szBuffer );

		fprintf( lpFPtr, TEXT("%s%c\n"), szBuffer, cCheckDigit );
	}

	return hr;
}

EMS_RESULT CEMSOrbitObject::_UpdateTLEFile( void )
{
	EMS_RESULT hr = EMS_OK;

	if ( m_lpOrbitData )
	{
		if ( m_lpszFilespec && *m_lpszFilespec )
		{
			FILE *lpFPtr = fopen( m_lpszFilespec, "wt" );

			if ( lpFPtr )
			{
		 		LPORBITDATA lpData = (LPORBITDATA)m_lpOrbitData;

				for ( int i = 0; i < MAX_EMS_ORBIT_DATA; i++ )
				{
					if ( lpData[ i ].bInUse )
					{
						_UpdateTLERecord( &lpData[ i ], lpFPtr );
					}
				}
				fclose( lpFPtr );
			}
			else
				hr = EMS_NO_FILE;
		}
		else
			hr = EMS_ORBIT_INVALID_FILESPEC;
	}
	else
		hr = EMS_ORBIT_NO_MEMORY;

	return hr;
}

EMSTIME CEMSOrbitObject::_ConvertEpochTime( double dEpochTime )
{
	CEMSTime epochTime;
	int nYear = (int) (dEpochTime / 1000.0);	// year is YYddd.ddddd

	dEpochTime -= (double)nYear * 1000.0;	// remove year encoding

	if (( nYear >= 57 ) && ( nYear <= 99 ))
		nYear += 1900;
	else
		nYear += 2000;

	dEpochTime -= 1.0;		// want the number of days NOT the Julian Day

	epochTime.SetTime( nYear, dEpochTime );	// set using year and days

	return epochTime;
}

//      EMSTIME timeEpoch; double epoch;     // last two digits of year, day of year, fractional portion of day)
//      float xndt2o;  	// First time derivative of the Mean Motion
//      float xndd6o;  	// Second time derivative of Mean motion
//      int iexp; 	//
//      float fBStar; float bstar;   	// BSTAR drag term
//      int ibexp;
//      float fInclination; float xincl; 	// Inclination (degs)
//      float fRightAscNode; float xnodeo;  	// Right ascension of the ascending node (degs)
//      float fEccentricity; float eo;	    	// eccentricity
//      float fArgPerigee; float omegao;  	// argument of perigee (degs)
//      float fMeanAnomaly; float xmo;	// mean anomaly (degs)
//      float fMeanMotion; float xno;	// mean motion (degs)
//      int nOrbitNumber;  int norbt;	// revolution number at epoch (revs)

// 	add co-variance matrix - needed by Barry ( look at data dict )
// 	get latest 
//	set latest
//   relate to Doppler function



/*
EMS_RESULT CEMSOrbitObject::_AllocateOrbitData( void )
{
	EMS_RESULT hr = EMS_OK;
	LONG lMemSizeBytes = sizeof( ORBITDATA ) * MAX_EMS_ORBIT_DATA;
	LONG lMemSizeKB = ( lMemSizeBytes + 1023 ) / 1024;
	BOOL bInitMem = FALSE;

	m_lpOrbitData = CMM_CreateMem( (char *)c_szOrbitMemoryName, &lMemSizeKB );

	if ( m_lpOrbitData )	// this instance has "created" the memory
	{
		bInitMem = TRUE;
	}
	else
	{
		m_lpOrbitData = CMM_OpenMem( (char *)c_szOrbitMemoryName, &lMemSizeKB );
	}

	if ( m_lpOrbitData )
	{
		if ( ( lMemSizeKB * 1024 ) >= lMemSizeBytes )
		{
			if ( bInitMem ) // if this instance created the memory 
			{
				if ( m_mutex.Lock() )
				{
					hr = _InitData();

					m_mutex.Unlock();
				}
			}
		}
		else	// not large enough
		{
			CMM_CloseMem( (char *)c_szOrbitMemoryName );

			m_lpOrbitData = 0;

			hr = EMS_ORBIT_NO_MEMORY;
		}
	}
	else
		hr = EMS_ORBIT_NO_MEMORY;

	return hr;
}
*/

EMS_RESULT 
CEMSOrbitObject::_AllocateOrbitDataX( void )
{
	EMS_RESULT hr = EMS_OK;
	LONG lMemSizeBytes = sizeof( ORBITDATA ) * MAX_EMS_ORBIT_DATA;
	LONG lMemSizeKB = ( lMemSizeBytes + 1023 ) / 1024;
	BOOL bInitMem = FALSE;

	m_hMapFile = CreateFileMapping(
				   INVALID_HANDLE_VALUE,    // use paging file
				   NULL,                    // default security
				   PAGE_READWRITE,          // read/write access
				   0,                       // maximum object size (high-order DWORD)
				   lMemSizeBytes,           // maximum object size (low-order DWORD)
				   c_szSharedMemoryName );  // name of mapping object


	if( m_hMapFile != NULL )
	{
		if( GetLastError() == ERROR_ALREADY_EXISTS )
		{
			bInitMem = FALSE;
		}
		else
		{
			bInitMem = TRUE;
		}

		m_lpOrbitData = (ORBITDATA*)MapViewOfFile( m_hMapFile,   // handle to map object
												   FILE_MAP_ALL_ACCESS, // read/write permission
												   0,
												   0,
												   lMemSizeBytes );
	}
	else
	{
		hr = EMS_NO_MEMORY;
	}

	if ( m_lpOrbitData )
	{
		if ( ( lMemSizeKB * 1024 ) >= lMemSizeBytes )
		{
			if ( bInitMem ) // if this instance created the memory 
			{
				if ( m_mutex.Lock() )
				{
					hr = _InitData();

					m_mutex.Unlock();
				}
			}
		}
		else	// not large enough
		{
			CloseHandle(m_hMapFile);

			m_lpOrbitData = 0;

			hr = EMS_ORBIT_NO_MEMORY;
		}
	}
	else
		hr = EMS_ORBIT_NO_MEMORY;

	return hr;
}


TCHAR CEMSOrbitObject::_CalcChecksum( LPCTSTR lpcszBuffer )
{
	TCHAR cCheckDigit = 0;

	if ( lpcszBuffer && *lpcszBuffer )
	{
		int nLength = lstrlen( lpcszBuffer );

		if ( nLength >= 68 )
		{
			int nCounter = 0;

			for ( int i = 0; i < 68; i++ )
			{
				switch ( lpcszBuffer[i] )
				{
					case '0': nCounter += 0; break;
					case '1': nCounter += 1; break;
					case '2': nCounter += 2; break;
					case '3': nCounter += 3; break;
					case '4': nCounter += 4; break;
					case '5': nCounter += 5; break;
					case '6': nCounter += 6; break;
					case '7': nCounter += 7; break;
					case '8': nCounter += 8; break;
					case '9': nCounter += 9; break;
					case '-': nCounter += 1; break;
					default: break;
				}
			}
			if ( nCounter )
				cCheckDigit = ( nCounter % 10 ) + '0';
		}
	}
	return cCheckDigit;
}


void 
CEMSOrbitObject::_DeallocateOrbitData( void )
{
	UnmapViewOfFile( m_lpOrbitData );
	CloseHandle(m_hMapFile);
}

EMS_RESULT CEMSOrbitObject::_ReadTLEFromFile( ULONG ulSatID, LPORBITDATA lpData )
{
	EMS_RESULT hr = EMS_OK;

	if ( lpData )
	{
		TCHAR szFilespec[ 256 ];

		wsprintf( szFilespec, c_szTLEFileNameFmt, m_lpszPath, ulSatID );

 		FILE *lpFPtr = fopen( szFilespec, "rb" );

		if ( lpFPtr )
		{
			if ( fread( lpData, sizeof( ORBITDATA ), 1 , lpFPtr ) != 1 )
				hr = EMS_IO_ERROR;
				
			fclose( lpFPtr );
		}
		else
			hr = EMS_NO_FILE;
	}
	else
		hr = EMS_BAD_PARAM;

	return hr;
}

EMS_RESULT CEMSOrbitObject::_WriteTLEToFile( ULONG ulSatID, LPORBITDATA lpData )
{
	EMS_RESULT hr = EMS_OK;

	if ( lpData )
	{
		TCHAR szFilespec[ 256 ];

		wsprintf( szFilespec, c_szTLEFileNameFmt, m_lpszPath, ulSatID );

 		FILE *lpFPtr = fopen( szFilespec, "wb" );

		if ( lpFPtr )
		{
			if ( fwrite( lpData, sizeof( ORBITDATA ), 1 , lpFPtr ) != 1 )
				hr = EMS_IO_ERROR;
				
			fclose( lpFPtr );
		}
		else
			hr = EMS_NO_FILE;
	}
	else
		hr = EMS_BAD_PARAM;

	return hr;
}

STDMETHODIMP CEMSOrbitObject::PropogateTLE( /* [in] */ EMSTLEDATA __RPC_FAR *lpTLE,
            /* [in] */ const EMSTIME eTime,
            /* [out] */ EMSTLEDATA __RPC_FAR *lpPropogatedTLE )
{
	EMS_RESULT hr = EMS_OK;

    if( lpTLE )
    {
		if ( !m_bInitialized )
			hr = Init();

		if ( hr == EMS_OK )
		{
			TTLEVector tleVector = _ConvertTLE( *lpTLE );
			TOrbit tempObj( &tleVector, 2 );
			TINVector inVector;
			CEMSTime epochTime( lpTLE->timeEpoch );
			float fDifference = (float)epochTime.MinutesDifferent( eTime );
			float ts = 0.0;

			inVector = tempObj.Predict( fDifference );
		    TCLVector clVector = TConvert::ToCL( &inVector );

			TTLEVector newTLE = tempObj.GetPropogatedTLE( );

			TINVector inVectorEpoch = tempObj.Predict( 0e0 );
		    TCLVector clVectorEpoch = TConvert::ToCL( &inVectorEpoch );

			if ( lpPropogatedTLE )
			{	
				memset( lpPropogatedTLE, 0, sizeof( EMSTLEDATA ) );

				lpPropogatedTLE->timeEpoch = eTime;
				lpPropogatedTLE->fXndt2o = newTLE.xndt2o; 
				lpPropogatedTLE->fXndd6o = newTLE.xndd6o;
				lpPropogatedTLE->elem.var.fInclination = newTLE.xincl;
				lpPropogatedTLE->fBStar = newTLE.bstar;
				lpPropogatedTLE->elem.var.fRightAscNode = newTLE.xnodeo;
				lpPropogatedTLE->elem.var.fEccentricity = newTLE.eo;
				lpPropogatedTLE->elem.var.fArgPerigee = newTLE.omegao;
				lpPropogatedTLE->elem.var.fMeanAnomaly = newTLE.xmo;
				lpPropogatedTLE->elem.var.fMeanMotion = newTLE.xno;

				memcpy( &lpPropogatedTLE->fCoVarMatrix, &lpTLE->fCoVarMatrix, 
							sizeof( lpPropogatedTLE->fCoVarMatrix ) );

	//			lpPropogatedTLE->nOrbitNumber = (int)( lpTLE->nOrbitNumber + fDifference *60e0/ 
	//					clVector.PERIOD +
	//               DMOD ( clVectorEpoch.ANOM + clVectorEpoch.PERI, 360e0)/360e0 -
	//               DMOD ( clVector.ANOM  + clVector.PERI , 360e0)/360e0 + 0.5e0 );

				double dTerm1, dTerm2, dTerm3, dTerm4;

				dTerm1 = (double) lpTLE->nOrbitNumber;
				dTerm2 = ( fDifference * 60e0 ) / clVectorEpoch.PERIOD;
				dTerm3 = DMOD ( clVectorEpoch.ANOM + clVectorEpoch.PERI, 360e0) / 360e0;
				dTerm4 =  DMOD ( clVector.ANOM  + clVector.PERI , 360e0) / 360e0 ;

				lpPropogatedTLE->nOrbitNumber = (int)( dTerm1 + dTerm2 + dTerm3 - dTerm4 + 0.5e0 );

			}
		}
	}
	else
		hr = EMS_BAD_PARAM;

	return hr;
}

        
STDMETHODIMP CEMSOrbitObject::Reload( /* [in] */ DWORD dwFlags )
{
	EMS_RESULT hr = EMS_OK;

	hr = EMS_NOT_IMPLEMENTED;

	return hr;
}

/*********************************************************************
*	IEMSOrbit2 Methods
*********************************************************************/

STDMETHODIMP CEMSOrbitObject::SetSatelliteTLE2( /* [in] */ ULONG ulSatID,
									          const EMSTLEDATA2 __RPC_FAR *lpTLEData )
{
	return SetSatelliteTLE3( ulSatID, lpTLEData, 0 );
}
        
STDMETHODIMP CEMSOrbitObject::GetSatelliteTLE2( /* [in] */ ULONG ulSatelliteID,
				const EMSTIME eTime, EMSTLEDATA2 __RPC_FAR *lpTLEData )
{
	EMS_RESULT hr = EMS_OK;

	if ( lpTLEData )
	{
		int nIndex = -1;

		memset( lpTLEData, 0, sizeof( EMSTLEDATA ) );

		if ( !m_bInitialized)
			hr = Init();

		if (( hr == EMS_OK ) && m_mutex.Lock() )
		{
			if ( eTime.intTime == 0 ) 	// get the most current TLE's
			{
			}
			else
			{
			}

			nIndex = _FindOrbitData( (int)ulSatelliteID );

			if ( nIndex != -1 )
			{
				LPORBITDATA lpData = (LPORBITDATA)m_lpOrbitData;

				*lpTLEData = lpData[nIndex].emsTLE;
			}
			else 
				hr = EMS_ORBIT_NO_SATELLITE;

			m_mutex.Unlock();
		}
	}
	else
		hr = EMS_BAD_PARAM;

	return hr;
}

/*********************************************************************
*	IEMSOrbit3 Methods
*********************************************************************/

STDMETHODIMP CEMSOrbitObject::SetSatelliteTLE3( /* [in] */ ULONG ulSatID,
           const EMSTLEDATA2 __RPC_FAR *lpTLEData, DWORD dwFlags )
{
	EMS_RESULT hr = EMS_OK;

	if ( lpTLEData )
	{
		if ( !m_bInitialized)
			hr = Init();

		if ( ( hr == EMS_OK ) && m_mutex.Lock() )
		{
			LPORBITDATA lpData = (LPORBITDATA)m_lpOrbitData;
			int nIndex = _FindOrbitData( (int)ulSatID );

			if ( nIndex >= 0 )
			{
				lpData[ nIndex ].nElementNumber++;	// up the counter

				if ( lpData[ nIndex ].nElementNumber > c_nMaxElementNumber )
					lpData[ nIndex ].nElementNumber = 1;

				lpData[ nIndex ].emsTLE = *lpTLEData;

				hr = _WriteTLEToFile( lpData[ nIndex].nSatelliteID, &lpData[ nIndex ] );
			}
			else
				hr = EMS_ORBIT_NO_SATELLITE;

			m_mutex.Unlock();
		}
	}
	else
		hr = EMS_BAD_PARAM;

	return hr;
}
        
STDMETHODIMP CEMSOrbitObject::ConvertOrbitVectorToTLE( /* [in] */ ULONG ulSatelliteID,
            /* [in] */ EMSTIMECOORD timeCoord, /* [out] */ EMSTLEDATA __RPC_FAR *lpTLEData )
{
	EMS_RESULT hr = EMS_OK;

	if ( !m_bInitialized )
		hr = Init();

	if ( hr == EMS_OK )
		hr = _ConvertEF2TLE( &timeCoord, lpTLEData );

	return hr;
}

HRESULT CEMSOrbitObject::_ConvertEF2TLE( EMSTIMECOORD *pvCoord, EMSTLEDATA *lpTLE )
{
	EMS_RESULT	hr = EMS_OK;

	if( pvCoord )
	{
		CEMSCoordSystem coordSys;

		TTLEVector	tleVector;
		TINVector	inVector;
		
		EMSTIMECOORD pvIC1, pvIC2, pvICtemp;
		EMSVECTOR	vH, vRdiff, vVdiff;
		double		dRmag, dVmag, dRV, dHR, dHV, dHxy, dHmag;
		double		dVmagN2, dAA, dBB, dTemp1, dTemp2;
		double		dSma, dEcc, dIncl, dNode, dPeri, dAnom, dMM;
		double		dErrRadius, dErrVelocity;
		double		dMinErrRadius = 1e-3, dMinErrVelocity = 1e-6;
		int			nIteration = 0, nMaxIteration = 200;
		BOOL        bLoopTest = false, bErrTest = false, bValid = true;

		if ( !m_bInitialized )
			hr = Init();

		if ( hr == EMS_OK )
		{

			pvIC1 = coordSys.ToInertial( pvCoord[0] );
			pvIC2 = pvIC1;

			lpTLE->timeEpoch = pvCoord->vTime;
			lpTLE->fXndt2o   = 0.0; 
			lpTLE->fXndd6o   = 0.0;
			lpTLE->fBStar    = 0.0;

			do    // Loop based upon maximum number of iterations or minimum errors obtained
			{
   
				dRmag = (double) CEMSVector::DotProduct( pvIC2.coord.radius, pvIC2.coord.radius );
				dRmag = sqrt( dRmag );
				dVmag = (double) CEMSVector::DotProduct( pvIC2.coord.velocity, pvIC2.coord.velocity );
				dVmag = sqrt( dVmag );
				dRV   = (double) CEMSVector::DotProduct( pvIC2.coord.radius, pvIC2.coord.velocity );
				vH    = CEMSVector::CrossProduct( pvIC2.coord.radius, pvIC2.coord.velocity );
      
				dHR   = vH.fX * pvIC2.coord.radius.fY   - vH.fY * pvIC2.coord.radius.fX;
				dHV   = vH.fX * pvIC2.coord.velocity.fY - vH.fY * pvIC2.coord.velocity.fX;
				dHxy  = vH.fX * vH.fX + vH.fY * vH.fY;
				dHmag = sqrt( dHxy + vH.fZ * vH.fZ ); 
				
				dVmagN2 = dVmag * dVmag / c_dGravitationConstant; 
				dAA   = dVmagN2 - 1/dRmag; 
				dBB   = dRV / c_dGravitationConstant;
 
				dSma  = dRmag / (2 - dVmagN2 * dRmag );
				dEcc  = sqrt (fabs (dAA * dAA * dRmag * dRmag + dBB * dRV / dSma));
				dIncl = (double) atan2( (double)sqrt (dHxy), (double)vH.fZ);
				dPeri = 0;
				dNode = atan2 (vH.fX, -vH.fY);
      
				if (dEcc > 0)
				{
      				if ( dHxy )
					{
						dTemp1 = dAA * pvIC2.coord.radius.fZ - dBB * pvIC2.coord.velocity.fZ;
						dTemp1 *= dHmag;
						dTemp2 = dAA * dHR - dBB * dHV;
						dPeri  = atan2( dTemp1, dTemp2 );

						dTemp1 = dHmag * pvIC2.coord.radius.fZ;
						dTemp2 = dHR;
						dAnom  = atan2( dTemp1, dTemp2 ) - dPeri;
					}
					else
					{
						dTemp1 = SIGN( dAA * pvIC2.coord.radius.fY - dBB * pvIC2.coord.velocity.fY);
						dTemp1 *= fabs( vH.fZ );
						dTemp2 = dAA * pvIC2.coord.radius.fX - dBB * pvIC2.coord.velocity.fX;
						dPeri  = atan2( dTemp1, dTemp2);

						dTemp1 = SIGN( pvIC2.coord.radius.fY ) * fabs( vH.fZ );
						dTemp2 = pvIC2.coord.radius.fX;
						dAnom  = atan2( dTemp1, dTemp2 ) - dPeri;
					}
				}
      
				dMM = sqrt( c_dGravitationConstant / dSma ) / dSma;
				dMM *= 86400.0 / c_dTwoPI; 

				lpTLE->elem.var.fInclination  = dIncl * c_dRadToDeg;
				lpTLE->elem.var.fRightAscNode = dNode * c_dRadToDeg;
				lpTLE->elem.var.fEccentricity = dEcc;
				lpTLE->elem.var.fArgPerigee   = dPeri * c_dRadToDeg;
				lpTLE->elem.var.fMeanAnomaly  = dAnom * c_dRadToDeg;
				lpTLE->elem.var.fMeanMotion   = dMM;
   
				tleVector = _ConvertTLE( *lpTLE );
				if (tleVector.xno > 0)
				{
					TOrbit orbitObj( &tleVector, 2 );

					inVector = orbitObj.Predict();
   
					pvICtemp.coord.radius.fX = (float) inVector.Rx;
					pvICtemp.coord.radius.fY = (float) inVector.Ry;
					pvICtemp.coord.radius.fZ = (float) inVector.Rz;
					pvICtemp.coord.velocity.fX = (float) inVector.Vx;
					pvICtemp.coord.velocity.fY = (float) inVector.Vy;
					pvICtemp.coord.velocity.fZ = (float) inVector.Vz;
   
					vRdiff = CEMSVector::Sub( pvICtemp.coord.radius,   pvIC1.coord.radius );
					vVdiff = CEMSVector::Sub( pvICtemp.coord.velocity, pvIC1.coord.velocity );

					pvIC2.coord.radius   = CEMSVector::Sub(pvIC2.coord.radius, vRdiff );
					pvIC2.coord.velocity = CEMSVector::Sub(pvIC2.coord.velocity, vVdiff );

					dErrRadius = (double) CEMSVector::DotProduct( vRdiff, vRdiff );
					dErrRadius = sqrt( dErrRadius );
					
					dErrVelocity = (double) CEMSVector::DotProduct( vVdiff, vVdiff );
					dErrVelocity = sqrt( dErrVelocity );

					nIteration++;

					if (nIteration > nMaxIteration/2) 
					{
						bErrTest  =	( (dErrRadius > dMinErrRadius*10) || (dErrVelocity > dMinErrVelocity*10) );
					}
					else
					{
						bErrTest  =	( (dErrRadius > dMinErrRadius) || (dErrVelocity > dMinErrVelocity) );
					}
					bLoopTest = ( nIteration < nMaxIteration );
				}
				else
				{
					bValid = false;
				}

			} while ( bLoopTest && bErrTest && bValid );

			if ((bErrTest)||(!bValid))
			{
				hr = EMS_BAD_PARAM;
			}
		}
	}
	else
		hr = EMS_BAD_PARAM;


	return hr;
}

/*********************************************************************
*	IEMSOrbit4 Methods
*********************************************************************/

STDMETHODIMP CEMSOrbitObject::EarthFixedOrbitDouble( ULONG ulSatelliteID,
				 const EMSTIME __RPC_FAR *pvTime, ULONG cbCount,
				 EMSTIMECOORDD __RPC_FAR *pvCoord, ULONG __RPC_FAR *pcbResult)
{
	CEMSCoordSystem coordSys;

	HRESULT hr = InertialOrbitDouble( ulSatelliteID, pvTime, cbCount, 
						pvCoord, pcbResult );

	if ( (hr == EMS_OK) && pcbResult && ( *pcbResult > 0 ) )
	{
		for ( int i = 0; i < (int)*pcbResult; i++ )
		{
			pvCoord[i] = coordSys.ToEarthFixed( pvCoord[i] );
		}
	}
	return hr;
}

STDMETHODIMP CEMSOrbitObject::EarthFixedOrbitTLEDouble( EMSTLEDATA __RPC_FAR *lpTLE,
				 const EMSTIME __RPC_FAR *pvTime, ULONG cbCount, 
				 EMSTIMECOORDD __RPC_FAR *pvCoord, ULONG __RPC_FAR *pcbResult )
{
	CEMSCoordSystem coordSys;

	HRESULT hr = InertialOrbitTLEDouble( lpTLE, pvTime, cbCount, pvCoord, pcbResult );

	if ( (hr == EMS_OK) && pcbResult && ( *pcbResult > 0 ) )
	{
		for ( int i = 0; i < (int)*pcbResult; i++ )
		{
			pvCoord[i] = coordSys.ToEarthFixed( pvCoord[i] );
		}
	}
	return hr;
}

STDMETHODIMP CEMSOrbitObject::InertialOrbitDouble( ULONG ulSatelliteID,
				 const EMSTIME __RPC_FAR *pvTime, ULONG cbCount,
				 EMSTIMECOORDD __RPC_FAR *pvCoord, ULONG __RPC_FAR *pcbResult)
{
	HRESULT hr = EMS_OK;
	int nIndex = -1;
	ULONG ulReturned = 0;

	if ( !m_bInitialized)
		hr = Init();

	if ( hr == EMS_OK )
	{
		ORBITDATA orbitData;

		if ( _GetOrbitData( (int)ulSatelliteID, &orbitData ) )
		{
			hr = InertialOrbitTLEDouble( &orbitData.emsTLE.tle, pvTime, cbCount,
							pvCoord, &ulReturned );
		}
		else
			hr = EMS_ORBIT_NO_SATELLITE;
	}

	if ( pcbResult )
		*pcbResult = ulReturned;

	return hr;
}
        
STDMETHODIMP CEMSOrbitObject::InertialOrbitTLEDouble( EMSTLEDATA __RPC_FAR *lpTLE,
				 const EMSTIME __RPC_FAR *pvTime, ULONG cbCount, 
            EMSTIMECOORDD __RPC_FAR *pvCoord, ULONG __RPC_FAR *pcbResult )
{
	EMS_RESULT hr = EMS_OK;
	ULONG ulReturned = 0;

	if( lpTLE )
	{
		if ( !m_bInitialized )
			hr = Init();

		if ( hr == EMS_OK ) 
		{
			TTLEVector tleVector = _ConvertTLE( *lpTLE );
			TOrbit tempObj( &tleVector, 2 );
			TINVector inVector;
			TEFVector efVector;
			CEMSTime epochTime( lpTLE->timeEpoch );
			float fDifference;

			for ( int i = 0; pvCoord && ( i < (int)cbCount ); i++ )
			{
				fDifference = (float)epochTime.MinutesDifferent( pvTime[i] );

				inVector = tempObj.Predict( fDifference );

				if ( pvCoord )
				{
					pvCoord[i].coordSystem = COORDSYS_Inertial;
					pvCoord[i].vTime = pvTime[i];
					pvCoord[i].coord.radius.dX   = inVector.Rx;
					pvCoord[i].coord.radius.dY   = inVector.Ry;
					pvCoord[i].coord.radius.dZ   = inVector.Rz;
					pvCoord[i].coord.velocity.dX = inVector.Vx;
					pvCoord[i].coord.velocity.dY = inVector.Vy;
					pvCoord[i].coord.velocity.dZ = inVector.Vz;

					ulReturned++;
				}
			}
		}
	}
	else
		hr = EMS_BAD_PARAM;

	if ( pcbResult )
		*pcbResult = ulReturned;

	return hr;
}
        
STDMETHODIMP CEMSOrbitObject::ConvertOrbitVectorToTLEDouble( /* [in] */ ULONG ulSatelliteID,
            /* [in] */ EMSTIMECOORDD timeCoord, /* [out] */ EMSTLEDATA __RPC_FAR *lpTLEData )
{
	EMS_RESULT hr = EMS_OK;

	if ( !m_bInitialized )
		hr = Init();

	if ( hr == EMS_OK )
		hr = _ConvertEF2TLEDouble( &timeCoord, lpTLEData );

	return hr;
}

HRESULT CEMSOrbitObject::_ConvertEF2TLEDouble( EMSTIMECOORDD *pvCoord, EMSTLEDATA *lpTLE )
{
	EMS_RESULT	hr = EMS_OK;

	if( pvCoord )
	{
		CEMSCoordSystem coordSys;

		TTLEVector	tleVector;
		TINVector	inVector;
	
		EMSTIMECOORDD pvIC1, pvIC2, pvICtemp;
		EMSVECTORD	vH, vRdiff, vVdiff;
		double		dRmag, dVmag, dRV, dHR, dHV, dHxy, dHmag;
		double		dVmagN2, dAA, dBB, dTemp1, dTemp2;
		double		dSma, dEcc, dIncl, dNode, dPeri, dAnom, dMM;
		double		dErrRadius, dErrVelocity;
		double		dMinErrRadius = 1e-3, dMinErrVelocity = 1e-6;
		int			nIteration = 0, nMaxIteration = 200;
		BOOL        bLoopTest = false, bErrTest = false, bValid = true;

		if ( !m_bInitialized )
			hr = Init();

		if ( hr == EMS_OK ) 
		{

			pvIC1 = coordSys.ToInertial( pvCoord[0] );
			pvIC2 = pvIC1;

			lpTLE->timeEpoch = pvCoord->vTime;
			lpTLE->fXndt2o   = 0.0; 
			lpTLE->fXndd6o   = 0.0;
			lpTLE->fBStar    = 0.0;

			const long clTotalMaxIterations = 1000;
			long lTotalIterations = 0;

			do    // Loop based upon maximum number of iterations or minimum errors obtained
			{
   
				dRmag = (double) CEMSVector::DotProduct( pvIC2.coord.radius, pvIC2.coord.radius );
				dRmag = sqrt( dRmag );
				dVmag = (double) CEMSVector::DotProduct( pvIC2.coord.velocity, pvIC2.coord.velocity );
				dVmag = sqrt( dVmag );
				dRV   = (double) CEMSVector::DotProduct( pvIC2.coord.radius, pvIC2.coord.velocity );
				vH    = CEMSVector::CrossProduct( pvIC2.coord.radius, pvIC2.coord.velocity );
      
				dHR   = vH.dX * pvIC2.coord.radius.dY   - vH.dY * pvIC2.coord.radius.dX;
				dHV   = vH.dX * pvIC2.coord.velocity.dY - vH.dY * pvIC2.coord.velocity.dX;
				dHxy  = vH.dX * vH.dX + vH.dY * vH.dY;
				dHmag = sqrt( dHxy + vH.dZ * vH.dZ ); 
				
				dVmagN2 = dVmag * dVmag / c_dGravitationConstant; 
				dAA   = dVmagN2 - 1/dRmag; 
				dBB   = dRV / c_dGravitationConstant;
 
				dSma  = dRmag / (2 - dVmagN2 * dRmag );
				dEcc  = sqrt (fabs (dAA * dAA * dRmag * dRmag + dBB * dRV / dSma));
				dIncl = atan2 (sqrt (dHxy), vH.dZ);
				dPeri = 0;
				dNode = atan2 (vH.dX, -vH.dY);
      
				if (dEcc > 0)
				{
      				if ( dHxy )
					{
						dTemp1 = dAA * pvIC2.coord.radius.dZ - dBB * pvIC2.coord.velocity.dZ;
						dTemp1 *= dHmag;
						dTemp2 = dAA * dHR - dBB * dHV;
						dPeri  = atan2( dTemp1, dTemp2 );

						dTemp1 = dHmag * pvIC2.coord.radius.dZ;
						dTemp2 = dHR;
						dAnom  = atan2( dTemp1, dTemp2 ) - dPeri;
					}
					else
					{
						dTemp1 = SIGN( dAA * pvIC2.coord.radius.dY - dBB * pvIC2.coord.velocity.dY);
						dTemp1 *= fabs( vH.dZ );
						dTemp2 = dAA * pvIC2.coord.radius.dX - dBB * pvIC2.coord.velocity.dX;
						dPeri  = atan2( dTemp1, dTemp2);

						dTemp1 = SIGN( pvIC2.coord.radius.dY ) * fabs( vH.dZ );
						dTemp2 = pvIC2.coord.radius.dX;
						dAnom  = atan2( dTemp1, dTemp2 ) - dPeri;
					}
				}
      
				dMM = sqrt( c_dGravitationConstant / dSma ) / dSma;
				dMM *= 86400.0 / c_dTwoPI; 

				lpTLE->elem.var.fInclination  = dIncl * c_dRadToDeg;
				lpTLE->elem.var.fRightAscNode = dNode * c_dRadToDeg;
				lpTLE->elem.var.fEccentricity = dEcc;
				lpTLE->elem.var.fArgPerigee   = dPeri * c_dRadToDeg;
				lpTLE->elem.var.fMeanAnomaly  = dAnom * c_dRadToDeg;
				lpTLE->elem.var.fMeanMotion   = dMM;
   
				tleVector = _ConvertTLE( *lpTLE );
				if (tleVector.xno > 0)
				{
					TOrbit orbitObj( &tleVector, 2 );

					inVector = orbitObj.Predict();
   
					pvICtemp.coord.radius.dX   = inVector.Rx;
					pvICtemp.coord.radius.dY   = inVector.Ry;
					pvICtemp.coord.radius.dZ   = inVector.Rz;
					pvICtemp.coord.velocity.dX = inVector.Vx;
					pvICtemp.coord.velocity.dY = inVector.Vy;
					pvICtemp.coord.velocity.dZ = inVector.Vz;
   
					vRdiff = CEMSVector::Sub( pvICtemp.coord.radius,   pvIC1.coord.radius );
					vVdiff = CEMSVector::Sub( pvICtemp.coord.velocity, pvIC1.coord.velocity );

					pvIC2.coord.radius   = CEMSVector::Sub(pvIC2.coord.radius, vRdiff );
					pvIC2.coord.velocity = CEMSVector::Sub(pvIC2.coord.velocity, vVdiff );

					dErrRadius = (double) CEMSVector::DotProduct( vRdiff, vRdiff );
					dErrRadius = sqrt( dErrRadius );
					
					dErrVelocity = (double) CEMSVector::DotProduct( vVdiff, vVdiff );
					dErrVelocity = sqrt( dErrVelocity );

					nIteration++;

					if (nIteration > nMaxIteration/2) 
					{
						bErrTest  =	( (dErrRadius > dMinErrRadius*10) || (dErrVelocity > dMinErrVelocity*10) );
					}
					else
					{
						bErrTest  =	( (dErrRadius > dMinErrRadius) || (dErrVelocity > dMinErrVelocity) );
					}
					bLoopTest = ( nIteration < nMaxIteration );
				}
				else
				{
					bValid = false;
				}

				lTotalIterations++;
				if( lTotalIterations >= clTotalMaxIterations )
					bValid = false;

			} while ( bLoopTest && bErrTest && bValid );

			if ((bErrTest)||(!bValid))
			{
				hr = EMS_BAD_PARAM;
			}
		}
	}
	else
		hr = EMS_BAD_PARAM;

	return hr;
}
