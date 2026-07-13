/********************************************************************
*   Module:         DBFOrbitCalc.cpp
*   Description:    Standalone orbit calculation – no COM dependencies.
*                   Ported from the Orbit DLL's CEMSOrbitObject class
*                   (ORBITOBJ.CPP).  All COM infrastructure has been
*                   stripped; heap memory replaces the COM shared-memory
*                   mapping; NORAD → satellite-ID mapping is read from
*                   SatNorad.txt (same directory as sarsat.txt).
*
*   Orbit prediction engine is the SGP4 / SDP4 / SGP8 / SDP8 code in
*   the OrbitCalc sub-directory (SgpxStr.h / TOrbit / TConvert …).
*
*********************************************************************
*   Copyright (c) 2006 by EMS Technologies, Inc.  All rights reserved.
********************************************************************/

#include <stdio.h>
#include <math.h>
#include <io.h>
#include <tchar.h>
#include <ctype.h>
#include <new>

#include "emsvectr.h"
#include "emstime.h"
#include "emscoord.h"
#include "emsconst.h"
#include "DBFPSConfig.h"

#include "DBFOrbitCalc.h"

#include "OrbitCalc\SgpxStr.h"

// ----------------------------------------------------------------
// Constants mirrored from ORBITOBJ.CPP
// ----------------------------------------------------------------
static const TCHAR c_szMutexName[]          = TEXT("EMSOrbitMutex");
static const TCHAR c_szTLEFileNameFmt[]     = TEXT("%sEMS-SatTLE-%03ld.dat");

static const int c_nMaxElementNumber = 9999;

#define DMOD(a,b)  ((a) - (long int)((a)/(b)) * (b))

// ----------------------------------------------------------------
// Static helpers (copied verbatim from ORBITOBJ.CPP)
// ----------------------------------------------------------------

static double _EncodeEpochTime( EMSTIME timeEMS )
{
    double dEpoch = 0.0;
    CEMSTime timeEpoch( timeEMS );
    EMSTIMEFIELDS timeFields;
    int nDaysInYear = timeEpoch.IsLeapYear() ? 366 : 365;

    timeEpoch.GetTime( &timeFields );

    if ( timeFields.nJulianDay >= nDaysInYear )
    {
        timeFields.nYear      += 1;
        timeFields.nJulianDay -= nDaysInYear;
    }

    if ( timeFields.nYear >= 2000 )
        timeFields.nYear -= 2000;
    else
        timeFields.nYear -= 1900;

    dEpoch  = (double)timeFields.nYear * 1000.0;
    dEpoch += (double)timeFields.nJulianDay;

    dEpoch += (double)timeFields.nHour / 24.0
            + (double)timeFields.nMinute / 1440.0
            + (double)timeFields.nSecond / 86400.0
            + (double)timeFields.lNanosecond / 86400000000000.0;

    return dEpoch;
}

static TTLEVector _ConvertTLE( EMSTLEDATA emsTLE )
{
    TTLEVector tleVector;
    memset( &tleVector, 0, sizeof( tleVector ) );

    tleVector.epoch   = _EncodeEpochTime( emsTLE.timeEpoch );
    tleVector.xndt2o  = emsTLE.fXndt2o;
    tleVector.xndd6o  = emsTLE.fXndd6o;
    tleVector.iexp    = 0;
    tleVector.xincl   = emsTLE.elem.var.fInclination;
    tleVector.bstar   = emsTLE.fBStar;
    tleVector.ibexp   = 0;
    tleVector.xnodeo  = emsTLE.elem.var.fRightAscNode;
    tleVector.eo      = emsTLE.elem.var.fEccentricity;
    tleVector.omegao  = emsTLE.elem.var.fArgPerigee;
    tleVector.xmo     = emsTLE.elem.var.fMeanAnomaly;
    tleVector.xno     = emsTLE.elem.var.fMeanMotion;
    tleVector.norbt   = emsTLE.nOrbitNumber;

    return tleVector;
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
    double dTerm1 = (double)ulOrigNumber;
    double dTerm2 = ( dTimeDifference * 60e0 ) / clVectorEpoch.PERIOD;
    double dTerm3 = DMOD( _Angle360( clVectorEpoch.ANOM ) + _Angle360( clVectorEpoch.PERI ), 360e0 ) / 360e0;
    double dTerm4 = DMOD( _Angle360( clVector.ANOM )      + _Angle360( clVector.PERI ),      360e0 ) / 360e0;

    return (ULONG)( dTerm1 + dTerm2 + dTerm3 - dTerm4 + 0.5e0 );
}

// ================================================================
// CDBFOrbitCalc
// ================================================================

CDBFOrbitCalc::CDBFOrbitCalc()
    : m_bInitialized( FALSE )
    , m_lpOrbitData( NULL )
    , m_lpszFilespec( NULL )
    , m_lpszPath( NULL )
    , m_nNoradMapCount( 0 )
{
    memset( m_anNoradID, 0, sizeof( m_anNoradID ) );
    memset( m_anSatID,   0, sizeof( m_anSatID ) );
    m_mutex.Create( c_szMutexName );
}

CDBFOrbitCalc::~CDBFOrbitCalc()
{
    if ( m_lpszPath )
    {
        delete [] m_lpszPath;
        m_lpszPath = 0;
    }

    if ( m_lpszFilespec )
    {
        delete [] m_lpszFilespec;
        m_lpszFilespec = 0;
    }

    _DeallocateOrbitData();

    m_mutex.Close();

    m_bInitialized = FALSE;
}

// ----------------------------------------------------------------

EMS_RESULT CDBFOrbitCalc::Init()
{
    EMS_RESULT hr = EMS_OK;

    if ( !m_bInitialized )
    {
        try
        {
            hr = _SetPath();

            if ( hr == EMS_OK )
                hr = _LoadNoradMap();

            if ( hr == EMS_OK )
            {
                hr = _AllocateOrbitData();
                if ( hr == EMS_OK )
                    m_bInitialized = TRUE;
            }
        }
        catch ( ... )
        {
            hr = EMS_EXCEPTION;
        }
    }

    return hr;
}

// ----------------------------------------------------------------

EMS_RESULT CDBFOrbitCalc::GetSatelliteTLE( ULONG ulSatelliteID,
                                            const EMSTIME eTime,
                                            EMSTLEDATA *lpTLEData )
{
    EMS_RESULT  hr = EMS_OK;
    EMSTLEDATA2 tleData2;

    memset( &tleData2, 0, sizeof( tleData2 ) );

    if ( !m_bInitialized )
        hr = Init();

    if ( hr == EMS_OK && m_mutex.Lock() )
    {
        int nIndex = _FindOrbitData( (int)ulSatelliteID );

        if ( nIndex != -1 )
        {
            if ( lpTLEData )
                *lpTLEData = m_lpOrbitData[ nIndex ].emsTLE.tle;
        }
        else
        {
            hr = EMS_ORBIT_NO_SATELLITE;
        }

        m_mutex.Unlock();
    }

    return hr;
}

// ----------------------------------------------------------------

EMS_RESULT CDBFOrbitCalc::GetOrbitNumber( ULONG ulSatelliteID,
                                           const EMSTIME eTime,
                                           ULONG *lpulOrbitNumber )
{
    EMS_RESULT hr = EMS_OK;
    ULONG ulOrbitNumber = 0;

    if ( !m_bInitialized )
        hr = Init();

    if ( hr == EMS_OK && m_mutex.Lock() )
    {
        int nIndex = _FindOrbitData( (int)ulSatelliteID );

        if ( nIndex != -1 )
        {
            TTLEVector tleVector = _ConvertTLE( m_lpOrbitData[ nIndex ].emsTLE.tle );
            TOrbit tempObj( &tleVector, 2 );
            CEMSTime epochTime( m_lpOrbitData[ nIndex ].emsTLE.tle.timeEpoch );
            float fDifference = (float)epochTime.MinutesDifferent( eTime );

            m_mutex.Unlock();

            TINVector inVector      = tempObj.Predict( fDifference );
            TCLVector clVector      = TConvert::ToCL( &inVector );

            TINVector inVectorEpoch = tempObj.Predict( 0e0 );
            TCLVector clVectorEpoch = TConvert::ToCL( &inVectorEpoch );

            ulOrbitNumber = _CalcOrbitNumber(
                                m_lpOrbitData[ nIndex ].emsTLE.tle.nOrbitNumber,
                                clVectorEpoch, clVector, fDifference );
        }
        else
        {
            m_mutex.Unlock();
            hr = EMS_ORBIT_NO_SATELLITE;
        }
    }

    if ( lpulOrbitNumber )
        *lpulOrbitNumber = ulOrbitNumber;

    return hr;
}

// ----------------------------------------------------------------

EMS_RESULT CDBFOrbitCalc::InertialOrbitTLE( EMSTLEDATA *lpTLE,
                                             const EMSTIME *pvTime,
                                             ULONG cbCount,
                                             EMSTIMECOORD *pvCoord,
                                             ULONG *pcbResult )
{
    EMS_RESULT hr     = EMS_OK;
    ULONG ulReturned  = 0;

    if ( lpTLE )
    {
        if ( !m_bInitialized )
            hr = Init();

        if ( hr == EMS_OK )
        {
            TTLEVector tleVector = _ConvertTLE( *lpTLE );
            TOrbit     tempObj( &tleVector, 2 );
            CEMSTime   epochTime( lpTLE->timeEpoch );

            for ( int i = 0; pvCoord && ( i < (int)cbCount ); i++ )
            {
                float fDifference = (float)epochTime.MinutesDifferent( pvTime[i] );
                TINVector inVector = tempObj.Predict( fDifference );

                pvCoord[i].coordSystem       = COORDSYS_Inertial;
                pvCoord[i].vTime             = pvTime[i];
                pvCoord[i].coord.radius.fX   = (float)inVector.Rx;
                pvCoord[i].coord.radius.fY   = (float)inVector.Ry;
                pvCoord[i].coord.radius.fZ   = (float)inVector.Rz;
                pvCoord[i].coord.velocity.fX = (float)inVector.Vx;
                pvCoord[i].coord.velocity.fY = (float)inVector.Vy;
                pvCoord[i].coord.velocity.fZ = (float)inVector.Vz;

                ulReturned++;
            }
        }
    }
    else
    {
        hr = EMS_BAD_PARAM;
    }

    if ( pcbResult )
        *pcbResult = ulReturned;

    return hr;
}

// ----------------------------------------------------------------

EMS_RESULT CDBFOrbitCalc::EarthFixedOrbitTLE( EMSTLEDATA *lpTLE,
                                               const EMSTIME *pvTime,
                                               ULONG cbCount,
                                               EMSTIMECOORD *pvCoord,
                                               ULONG *pcbResult )
{
    CEMSCoordSystem coordSys;

    EMS_RESULT hr = InertialOrbitTLE( lpTLE, pvTime, cbCount, pvCoord, pcbResult );

    if ( ( hr == EMS_OK ) && pcbResult && ( *pcbResult > 0 ) )
    {
        for ( int i = 0; i < (int)*pcbResult; i++ )
        {
            pvCoord[i] = coordSys.ToEarthFixed( pvCoord[i] );
        }
    }

    return hr;
}

// ================================================================
// Private helpers
// ================================================================

int CDBFOrbitCalc::_FindOrbitData( int nID, BOOL bUseNorad )
{
    int nIndex = -1;

    if ( m_lpOrbitData )
    {
        int i = 0;
        while ( ( nIndex == -1 ) && ( i < DBF_MAX_EMS_ORBIT_DATA ) )
        {
            if ( m_lpOrbitData[i].bInUse )
            {
                if ( ( bUseNorad  && ( m_lpOrbitData[i].nNoradNumber    == nID ) ) ||
                     ( !bUseNorad && ( m_lpOrbitData[i].nSatelliteID    == nID ) ) )
                    nIndex = i;
            }
            i++;
        }
    }

    return nIndex;
}

BOOL CDBFOrbitCalc::_GetOrbitData( int nID, LPDBFORBITDATA lpOrbitData, BOOL bUseNorad )
{
    BOOL bOK = FALSE;

    if ( lpOrbitData && m_mutex.Lock() )
    {
        int nIndex = _FindOrbitData( nID, bUseNorad );

        if ( nIndex != -1 )
        {
            *lpOrbitData = m_lpOrbitData[ nIndex ];
            bOK = TRUE;
        }

        m_mutex.Unlock();
    }

    return bOK;
}

int CDBFOrbitCalc::_FindEmptySlot()
{
    int nIndex = -1;

    if ( m_lpOrbitData )
    {
        int i = 0;
        while ( ( nIndex == -1 ) && ( i < DBF_MAX_EMS_ORBIT_DATA ) )
        {
            if ( !m_lpOrbitData[i].bInUse )
                nIndex = i;
            else
                i++;
        }
    }

    return nIndex;
}

EMS_RESULT CDBFOrbitCalc::_AllocateOrbitData()
{
    EMS_RESULT hr = EMS_OK;

    m_lpOrbitData = new (std::nothrow) DBFORBITDATA[ DBF_MAX_EMS_ORBIT_DATA ];

    if ( m_lpOrbitData )
    {
        if ( m_mutex.Lock() )
        {
            hr = _InitData();
            m_mutex.Unlock();
        }
    }
    else
    {
        hr = EMS_ORBIT_NO_MEMORY;
    }

    return hr;
}

void CDBFOrbitCalc::_DeallocateOrbitData()
{
    if ( m_lpOrbitData )
    {
        delete [] m_lpOrbitData;
        m_lpOrbitData = NULL;
    }
}

EMS_RESULT CDBFOrbitCalc::_InitData()
{
    EMS_RESULT hr = EMS_UNKNOWN_ERROR;

    if ( m_lpOrbitData )
    {
        memset( m_lpOrbitData, 0, sizeof( DBFORBITDATA ) * DBF_MAX_EMS_ORBIT_DATA );

        if ( m_lpszFilespec )
        {
            hr = _InitDataFromFile( m_lpszFilespec );

            if ( hr == EMS_OK )
            {
                DBFORBITDATA tempData;

                for ( int i = 0; i < DBF_MAX_EMS_ORBIT_DATA; i++ )
                {
                    if ( m_lpOrbitData[i].bInUse )
                    {
                        if ( _ReadTLEFromFile( m_lpOrbitData[i].nSatelliteID, &tempData ) == EMS_OK )
                        {
                            if ( tempData.emsTLE.tle.timeEpoch.intTime >
                                 m_lpOrbitData[i].emsTLE.tle.timeEpoch.intTime )
                            {
                                m_lpOrbitData[i]        = tempData;
                                m_lpOrbitData[i].bInUse = TRUE;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            hr = EMS_ORBIT_INVALID_FILESPEC;
        }
    }
    else
    {
        hr = EMS_ORBIT_NO_MEMORY;
    }

    return hr;
}

EMS_RESULT CDBFOrbitCalc::_InitDataFromFile( LPCTSTR lpcszFilename )
{
    EMS_RESULT hr = EMS_UNKNOWN_ERROR;

    if ( lpcszFilename && *lpcszFilename )
    {
        FILE *pFile = fopen( lpcszFilename, "rt" );

        try
        {
            if ( pFile )
            {
                char szLine1[256], szLine2[256], szSatelliteName[256];
                EMSTLEDATA tleData;
                int i, nNoradNumber, nIndex, nElementNumber = 0;
                double dEpoch;

                hr = EMS_OK;

                while ( !feof( pFile ) && ( hr == EMS_OK ) )
                {
                    if ( fgets( szSatelliteName, sizeof( szSatelliteName ), pFile ) )
                    {
                        if ( _ValidName( szSatelliteName ) &&
                             fgets( szLine1, sizeof( szLine1 ), pFile ) )
                        {
                            TCHAR cCheck1 = _CalcChecksum( szLine1 );

                            if ( ( lstrlen( szLine1 ) > 68 ) && ( cCheck1 == szLine1[68] ) )
                            {
                                if ( fgets( szLine2, sizeof( szLine2 ), pFile ) )
                                {
                                    TCHAR cCheck2 = _CalcChecksum( szLine2 );

                                    if ( ( lstrlen( szLine2 ) > 68 ) && ( cCheck2 == szLine2[68] ) )
                                    {
                                        memset( &tleData, 0, sizeof( tleData ) );

                                        for ( i = 18; i < 32; i++ )
                                        {
                                            if ( szLine1[i] == ' ' )
                                                szLine1[i] = '0';
                                        }

                                        sscanf( &szLine1[18], "%14lf", &dEpoch );
                                        sscanf( &szLine1[33], "%10lf", &tleData.fXndt2o );

                                        sscanf( &szLine1[44], "%6lf",  &tleData.fXndd6o );
                                        tleData.fXndd6o /= 100000.0;

                                        int nExp = 0;
                                        sscanf( &szLine1[50], "%2d",  &nExp );
                                        if ( nExp != 0 )
                                            tleData.fXndd6o *= pow( 10.0, nExp );

                                        sscanf( &szLine1[53], "%6lf", &tleData.fBStar );
                                        tleData.fBStar /= 100000.0;

                                        sscanf( &szLine1[59], "%2d",  &nExp );
                                        if ( nExp != 0 )
                                            tleData.fBStar *= pow( 10.0, nExp );

                                        for ( i = 64; i < 68; i++ )
                                        {
                                            if ( szLine1[i] == ' ' )
                                                szLine1[i] = '0';
                                        }
                                        sscanf( &szLine1[64], "%4d", &nElementNumber );

                                        szLine2[25] = '.';

                                        for ( i = 63; i < 68; i++ )
                                        {
                                            if ( szLine2[i] == ' ' )
                                                szLine2[i] = '0';
                                        }

                                        sscanf( &szLine2[ 2], "%5d",  &nNoradNumber );
                                        sscanf( &szLine2[ 8], "%8lf", &tleData.elem.var.fInclination );
                                        sscanf( &szLine2[17], "%8lf", &tleData.elem.var.fRightAscNode );
                                        szLine2[25] = '.';
                                        sscanf( &szLine2[25], "%8lf", &tleData.elem.var.fEccentricity );
                                        sscanf( &szLine2[34], "%8lf", &tleData.elem.var.fArgPerigee );
                                        sscanf( &szLine2[43], "%8lf", &tleData.elem.var.fMeanAnomaly );
                                        sscanf( &szLine2[52], "%11lf",&tleData.elem.var.fMeanMotion );
                                        sscanf( &szLine2[63], "%5d", &tleData.nOrbitNumber );

                                        nIndex = _FindOrbitData( nNoradNumber, TRUE );

                                        if ( nIndex == -1 )
                                            nIndex = _FindEmptySlot();

                                        if ( nIndex != -1 )
                                        {
                                            INT nSatID = _SatIDFromNoradNumber( nNoradNumber );

                                            if ( nSatID > 0 )
                                            {
                                                _RemoveTrailingUnprintables( szSatelliteName );

                                                m_lpOrbitData[ nIndex ].bInUse         = TRUE;
                                                m_lpOrbitData[ nIndex ].nElementNumber = nElementNumber;
                                                m_lpOrbitData[ nIndex ].emsTLE.tle     = tleData;
                                                m_lpOrbitData[ nIndex ].nNoradNumber   = nNoradNumber;
                                                m_lpOrbitData[ nIndex ].nSatelliteID   = nSatID;

                                                lstrcpyn( m_lpOrbitData[ nIndex ].szName,
                                                          szSatelliteName,
                                                          DBF_MAX_SAT_NAME_LENGTH );

                                                m_lpOrbitData[ nIndex ].emsTLE.tle.timeEpoch =
                                                    _ConvertEpochTime( dEpoch );
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
        catch ( ... )
        {
            hr = EMS_EXCEPTION;
        }

        if ( pFile )
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

EMS_RESULT CDBFOrbitCalc::_LoadNoradMap()
{
    const std::string& sNorad = CDBFPSConfig::Instance().SatNoradFile();
    if ( sNorad.empty() )
        return EMS_ORBIT_INVALID_FILESPEC;

    FILE *pFile = fopen( sNorad.c_str(), "rt" );
    if ( !pFile )
        return EMS_NO_FILE;

    EMS_RESULT hr = EMS_OK;
    m_nNoradMapCount = 0;

    char szLine[64];

    // First line: "Count N" – read and discard
    if ( !fgets( szLine, sizeof( szLine ), pFile ) )
    {
        fclose( pFile );
        return EMS_ORBIT_BAD_DATA;
    }

    while ( !feof( pFile ) && ( m_nNoradMapCount < DBF_MAX_EMS_ORBIT_DATA ) )
    {
        if ( !fgets( szLine, sizeof( szLine ), pFile ) )
            break;

        int nSatID = 0, nNoradID = 0;
        if ( sscanf( szLine, "%d,%d", &nSatID, &nNoradID ) == 2 )
        {
            m_anSatID  [ m_nNoradMapCount ] = nSatID;
            m_anNoradID[ m_nNoradMapCount ] = nNoradID;
            m_nNoradMapCount++;
        }
    }

    fclose( pFile );

    if ( m_nNoradMapCount == 0 )
        hr = EMS_ORBIT_BAD_DATA;

    return hr;
}

int CDBFOrbitCalc::_SatIDFromNoradNumber( int nNoradNumber )
{
    for ( int i = 0; i < m_nNoradMapCount; i++ )
    {
        if ( m_anNoradID[i] == nNoradNumber )
            return m_anSatID[i];
    }
    return 0;
}

int CDBFOrbitCalc::_RemoveTrailingUnprintables( LPTSTR lpszText )
{
    int nLength = lpszText ? lstrlen( lpszText ) : 0;

    while ( ( nLength > 0 ) && ( lpszText[ nLength - 1 ] <= 32 ) )
    {
        lpszText[ --nLength ] = 0;
    }

    return nLength;
}

EMSTIME CDBFOrbitCalc::_ConvertEpochTime( double dEpochTime )
{
    CEMSTime epochTime;
    int nYear = (int)( dEpochTime / 1000.0 );

    dEpochTime -= (double)nYear * 1000.0;

    if ( ( nYear >= 57 ) && ( nYear <= 99 ) )
        nYear += 1900;
    else
        nYear += 2000;

    dEpochTime -= 1.0;

    epochTime.SetTime( nYear, dEpochTime );

    return epochTime;
}

EMS_RESULT CDBFOrbitCalc::_ReadTLEFromFile( ULONG ulSatID, LPDBFORBITDATA lpData )
{
    EMS_RESULT hr = EMS_OK;

    if ( lpData )
    {
        TCHAR szFilespec[_MAX_PATH];
        wsprintf( szFilespec, c_szTLEFileNameFmt,
                  CDBFPSConfig::Instance().SatTLEDir().c_str(), ulSatID );

        FILE *lpFPtr = fopen( szFilespec, "rb" );

        if ( lpFPtr )
        {
            if ( fread( lpData, sizeof( DBFORBITDATA ), 1, lpFPtr ) != 1 )
                hr = EMS_IO_ERROR;

            fclose( lpFPtr );
        }
        else
        {
            hr = EMS_NO_FILE;
        }
    }
    else
    {
        hr = EMS_BAD_PARAM;
    }

    return hr;
}

EMS_RESULT CDBFOrbitCalc::_SetPath()
{
    const std::string& sFile = CDBFPSConfig::Instance().OrbitDataFile();
    if ( sFile.empty() )
        return EMS_ORBIT_INVALID_FILESPEC;

    if ( _access( sFile.c_str(), 0 ) != 0 )
        return EMS_ORBIT_INVALID_FILESPEC;

    m_lpszFilespec = new TCHAR[ sFile.size() + 1 ];
    if ( !m_lpszFilespec )
        return EMS_ORBIT_NO_MEMORY;
    lstrcpy( m_lpszFilespec, sFile.c_str() );

    TCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR];
    TCHAR szPath[_MAX_PATH] = TEXT("");
    _splitpath( m_lpszFilespec, szDrive, szDir, NULL, NULL );
    wsprintf( szPath, TEXT("%s%s"), szDrive, szDir );

    m_lpszPath = new TCHAR[ lstrlen( szPath ) + 1 ];
    if ( !m_lpszPath )
        return EMS_ORBIT_NO_MEMORY;
    lstrcpy( m_lpszPath, szPath );

    return EMS_OK;
}

BOOL CDBFOrbitCalc::_ValidName( LPCTSTR lpcszSatName )
{
    return ( lpcszSatName && *lpcszSatName && isalnum( *lpcszSatName ) ) ? TRUE : FALSE;
}

TCHAR CDBFOrbitCalc::_CalcChecksum( LPCTSTR lpcszBuffer )
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
                TCHAR ch = lpcszBuffer[i];
                nCounter += isdigit( ch ) ? ( ch - '0' ) : ( ch == '-' ) ? 1 : 0;
            }

            if ( nCounter )
                cCheckDigit = ( nCounter % 10 ) + '0';
        }
    }

    return cCheckDigit;
}
