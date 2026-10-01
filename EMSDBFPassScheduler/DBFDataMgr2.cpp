/********************************************************************
*	Module:			DBFDataMgr.cpp 
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

	0.0	2011/01/21		rcr		start

********************************************************************/
#pragma warning(disable : 4786)

#include <memory.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <tchar.h>
#include <io.h>

#include "emsclock.h"
#include "emsconst.h"
#include "emsDBFtypes2.h"
#include "emsDBFDataMgr2.h"
#include "emsencod.h"
#include "emsexcpt.h"
#include "convutility.h"
#include <Winnetwk.h>
#include "DBFPSConfig.h"

/********************************************************************/



/********************************************************************/

CEMSDBFDataMgr::CEMSDBFDataMgr()
{
	m_lpPassFileBIN  = NULL;
	m_lpPassFileCSV  = NULL;
	m_nCurrentHour   = -1;
	m_wCellNumber = 0;
}


CEMSDBFDataMgr::~CEMSDBFDataMgr()
{
	Reset();
}

/********************************************************************/

void 
CEMSDBFDataMgr::Reset()
{
	try
	{
	
		if ( m_lpPassFileBIN )
		{
			fclose( m_lpPassFileBIN );
			m_lpPassFileBIN = NULL;
		}

		if ( m_lpPassFileCSV)
		{
			fclose( m_lpPassFileCSV );
			m_lpPassFileCSV = NULL;
		}

		m_nCurrentHour = -1;
	}
	catch( ... )
	{
		throw;
	}

}

/********************************************************************/

EMS_RESULT
CEMSDBFDataMgr::Initialize( std::vector<EMSDBFCOMBINERCFG>& vectDbfCombiner, WORD wCellNumber )
{

	EMS_RESULT hr = EMS_BAD_PARAM;
	
	if( vectDbfCombiner.size() > 0 )
	{

		DWORD dwRetVal;
		NETRESOURCE nr;
		memset( &nr, 0, sizeof(nr) );

		for(int i= 0; i < vectDbfCombiner.size(); i++)
		{
			TCHAR szServerPathCSV[256];
			memset(&szServerPathCSV, 0, sizeof(szServerPathCSV));
			TCHAR szServerPathBIN[256];
			memset(&szServerPathBIN, 0, sizeof(szServerPathBIN));

			std::string szServerPath;
			szServerPath += vectDbfCombiner[i].szIpAddress.c_str();
			szServerPath += "\\CSV";

			lstrcpy( szServerPathCSV, szServerPath.c_str() );

			nr.dwType       = RESOURCETYPE_ANY;
			nr.lpLocalName  = NULL;
			nr.lpRemoteName = szServerPathCSV;
			nr.lpProvider   = NULL;
	//		DWORD dwFlags = CONNECT_UPDATE_PROFILE;
	//		
			LPSTR    lpPassword = "sarsat";
			LPSTR    lpUserid = "Administrator";
	//
	////		dwRetVal = WNetAddConnection2(&nr, NULL, NULL, dwFlags);
			dwRetVal = WNetAddConnection2(&nr, lpPassword, lpUserid, 0);
			if (dwRetVal == NO_ERROR)
				printf("Connection added to %s\n", nr.lpRemoteName);
			else
				wprintf(L"WNetAddConnection2 failed with error: %u\n", dwRetVal);

			//For BIN
			dwRetVal = 0;
			std::string szServerPathTmp;
			szServerPathTmp += vectDbfCombiner[i].szIpAddress.c_str();
			szServerPathTmp += "\\BIN";

			lstrcpy( szServerPathBIN, szServerPathTmp.c_str() );
			nr.lpRemoteName = szServerPathBIN;
			dwRetVal = WNetAddConnection2(&nr, lpPassword, lpUserid, 0);
			
			FILE* lpTraceFILE = CDBFPSConfig::Instance().OpenTraceLog();
			if (lpTraceFILE)
			{
				if (dwRetVal == NO_ERROR)
					fprintf(lpTraceFILE,"Connection added to %s\n", nr.lpRemoteName);
				else
					fprintf(lpTraceFILE,"WNetAddConnection2 failed with error: %u\n", dwRetVal);
				fclose(lpTraceFILE);
			}

		}
		m_wCellNumber = wCellNumber;

		hr = EMS_OK;
	}
	return hr;
}

void CEMSDBFDataMgr::Initialize()
{
	CEMSSystemClock     oTime;
	m_TimeStart = oTime.GetTime();
	m_TimeEnd.intTime = 0;

	m_ulRecordCount = 0;
	m_ulLastRecord  = 0;
}

/********************************************************************/

void
CEMSDBFDataMgr::EnsureHourFileOpen( int nHour, const std::string& szCombinerIP )
{
	if ( m_nCurrentHour == nHour && m_lpPassFileCSV )
		return;

	if ( m_lpPassFileCSV )
	{
		fflush( m_lpPassFileCSV );
		fclose( m_lpPassFileCSV );
		m_lpPassFileCSV = NULL;
	}

	TCHAR szFileName[256];
	TCHAR szFileSpec[256];
	_stprintf( szFileName, TEXT( "\\CSV\\TSIDBFPass-%ld.csv" ), nHour );
	lstrcpy( szFileSpec, szCombinerIP.c_str() );
	lstrcat( szFileSpec, szFileName );
	m_lpPassFileCSV = fopen( szFileSpec, "wt" );
	m_nCurrentHour = nHour;
	{
		FILE* lpT = CDBFPSConfig::Instance().OpenTraceLog();
		if ( lpT )
		{
			if ( m_lpPassFileCSV )
				fprintf( lpT, "CSV opened: %s\n", szFileSpec );
			else
				fprintf( lpT, "CSV fopen failed: %s\n", szFileSpec );
			fclose( lpT );
		}
	}
}

EMS_RESULT
CEMSDBFDataMgr::WritePassSchedule( EMSDBFPASSRECORD2 PassRecord, std::string szCombinerIP, bool bTestSun, const std::string& szConstellation )
{

	EMS_RESULT hr = EMS_BAD_PARAM;

	if ( &PassRecord )
	{
		CEMSTime  oTime = PassRecord.timestamp;
		EMSTIMEFIELDS tmFields;
		oTime.GetTime(&tmFields);
		int nPassHour = tmFields.nHour;

		EMSTIMEFIELDS tmFieldsFile;
		oTime = m_TimeEnd;
		oTime.GetTime(&tmFieldsFile);
		int nFileHour = tmFieldsFile.nHour;
		int iError;
		
		if ( !( nFileHour == nPassHour ) || !( m_lpPassFileBIN ))
		{
			TCHAR szFileName[256];
			TCHAR szFileSpec[256];

			memset(szFileName, 0, sizeof(szFileName));
			memset(szFileSpec, 0, sizeof(szFileSpec));

			if ( m_lpPassFileBIN )
			{
				fflush(m_lpPassFileBIN );

				fclose( m_lpPassFileBIN );
				m_lpPassFileBIN = NULL;
			}

			if ( !m_lpPassFileBIN)
			{
				_stprintf( szFileName, TEXT( "\\BIN\\TSIDBFPass-%ld.bin" ), nPassHour );
				
				lstrcpy( szFileSpec, szCombinerIP.c_str() );

				lstrcat( szFileSpec, szFileName ); 

				m_lpPassFileBIN = fopen( szFileSpec, "wb" );
				{
					FILE* lpT = CDBFPSConfig::Instance().OpenTraceLog();
					if ( lpT )
					{
						if ( m_lpPassFileBIN )
							fprintf( lpT, "BIN opened: %s\n", szFileSpec );
						else
							fprintf( lpT, "BIN fopen failed: %s\n", szFileSpec );
						fclose( lpT );
					}
				}

				m_TimeStart.intTime = PassRecord.timestamp.intTime;
				m_TimeEnd.intTime   = PassRecord.timestamp.intTime;

				m_ulRecordCount = 0;
				m_ulLastRecord  = 0;

			}
			
	
			if ( m_nCurrentHour != nPassHour )
			{
				EnsureHourFileOpen( nPassHour, szCombinerIP );
			}

		}

		if ( m_lpPassFileBIN )
		{
			// Write binary Pass Record
			iError = fwrite( &PassRecord, 1, sizeof( EMSDBFPASSRECORD2 ), m_lpPassFileBIN );
			fflush(m_lpPassFileBIN );
			hr = EMS_OK;

		}

		if ( m_lpPassFileCSV )
		{

			// Write CSV Pass Record
			iError = fprintf(m_lpPassFileCSV, "%04d/%02d/%02d,%02d:%02d:%02d,",
				tmFields.nYear,
				tmFields.nMonth,
				tmFields.nDay,
				tmFields.nHour,
				tmFields.nMinute,
				tmFields.nSecond );

			iError = fprintf(m_lpPassFileCSV, "%I64d,%4d,%02d,%4d,%3d,%3d,%15.6f,%1d,%6.1f,%5.1f,%6.1f,%5.1f,%5.1f,%9.1f,%9.1f,%8.6f,",
				PassRecord.timestamp,
				PassRecord.ulLutID,
				PassRecord.wPlateID,
				PassRecord.wProcessID,
				PassRecord.wAntennaID,
				PassRecord.ulSatID,
				EMSDBF_MHZ_FROM_HZ( PassRecord.ulDownFrequencyHz ), // "%15.6f" -- CSV stays in MHz
				PassRecord.wPolarization,
				PassRecord.fAzimuth,
				PassRecord.fElevation,
				PassRecord.fPlateAzimuth,
				PassRecord.fPlateElevation,
				PassRecord.fBeaconElevation,
				PassRecord.fRefTxFrequency,
				PassRecord.fFOA,
				PassRecord.fTOA);

			for ( int k = 0; k < 15; k++ )
			{
				iError = fprintf(m_lpPassFileCSV,"%c",PassRecord.cBeaconID[k]);
			}
			iError = fprintf(m_lpPassFileCSV,",");

			for ( int k = 0; k < 36; k++ )
			{
				iError = fprintf(m_lpPassFileCSV,"%c",PassRecord.cBeaconMess[k]);
			}

			for ( int k = 0; k < DBF_MAX_ELEMENTS; k++ )
			{
				iError = fprintf(m_lpPassFileCSV,",%6.1f",PassRecord.fPhase[k]);
			}
			iError = fprintf(m_lpPassFileCSV,",");

			iError = fprintf(m_lpPassFileCSV,"%I64d,%d,%8.1f,%8.1f,%8.1f,%8.5f,%8.5f,%8.5f",
				PassRecord.timeCoord.vTime.intTime,
				PassRecord.timeCoord.coordSystem,
				PassRecord.timeCoord.coord.radius.fX,
				PassRecord.timeCoord.coord.radius.fY,
				PassRecord.timeCoord.coord.radius.fZ,
				PassRecord.timeCoord.coord.velocity.fX,
				PassRecord.timeCoord.coord.velocity.fY,
				PassRecord.timeCoord.coord.velocity.fZ);

			// Trailing column identifying which constellation this record was tracking
			// (matters when a combiner has a primary + fallback constellation configured).
			iError = fprintf(m_lpPassFileCSV, ",%s", szConstellation.empty() ? "" : szConstellation.c_str() );

			iError = fprintf(m_lpPassFileCSV, "\n" );

			fflush(m_lpPassFileCSV );

		}

		if ( bTestSun && ( PassRecord.ulSatID == 999 ) )
		{
			FILE* lpSun = fopen( CDBFPSConfig::Instance().SunAzElFile().c_str(),"at" );
			if (lpSun)
			{
				fprintf(lpSun, "%04d/%02d/%02d,%02d:%02d:%02d,%10.3f,%10.3f\n",
					tmFields.nYear,
					tmFields.nMonth,
					tmFields.nDay,
					tmFields.nHour,
					tmFields.nMinute,
					tmFields.nSecond,
					PassRecord.fAzimuth,
					PassRecord.fElevation );
				fclose( lpSun );
			}
		}


	}
	return hr;
}

EMS_RESULT
CEMSDBFDataMgr::ReadRefBeacons( )
{
	EMS_RESULT hr = EMS_BAD_PARAM;

	if( &m_RefBeacons )
	{
		FILE *lpRefbeacon = fopen( CDBFPSConfig::Instance().ReferenceBeaconFile().c_str(),"rt" );
		FILE *lpTraceFILE =  CDBFPSConfig::Instance().OpenTraceLog();

		if ( !lpRefbeacon )
		{
			if ( lpTraceFILE ) fclose( lpTraceFILE );
			return EMS_NO_FILE;
		}

		REFERENCEBEACON refBeacon;
		memset( &refBeacon, 0, sizeof(REFERENCEBEACON) );
		m_RefCount = 0;
		int ncount = 0;
		int nRrate, nStartTime;
		float fLatitude, fLongitude, fAltitude, fFrequency;
		char cBeaconID[16];
		char cBeaconMess[37];
		char RefHeader[256];
		char RefName[256];
		
		// Header Record
		ncount = fscanf( lpRefbeacon,"%s", &RefHeader );

		while ( ncount > 0  && m_RefCount < 60 )
		{
			ncount = fscanf( lpRefbeacon, "%15s,%36s,%f,%f,%f,%f,%d,%d,%s",
						&cBeaconID,
						&cBeaconMess,
						&fLatitude,
						&fLongitude,
						&fAltitude,
						&fFrequency,
						&nRrate,
						&nStartTime,
						&RefName);
			if (ncount > 0)
			{
				//sscanf( cBeaconID, "%I64x", &refBeacon.ulBeaconID); 
				memcpy( &refBeacon.cBeaconID, &cBeaconID, 15);
				memcpy( &refBeacon.cBeaconMess, &cBeaconMess, 36);
				refBeacon.fFrequency = fFrequency;
				refBeacon.nRrate = nRrate;
				refBeacon.nStartTime = nStartTime;
				refBeacon.refLocation.degLatitude = (double)fLatitude;
				refBeacon.refLocation.degLongitude = (double)fLongitude;
				refBeacon.refLocation.fAltitude = fAltitude;

				memcpy( &m_RefBeacons.rec[m_RefCount], &refBeacon, sizeof(REFERENCEBEACON) );
				m_RefCount++;
				if (lpTraceFILE) fprintf(lpTraceFILE," %d RefBeacon %36s, %7.3f, %8.3f, %6.1f, %10.1f, %d, %d\n", m_RefCount, &cBeaconMess,
					fLatitude, fLongitude, fAltitude, fFrequency, nStartTime, nRrate );
			}
		}
		fclose( lpRefbeacon );
		hr = EMS_OK;
		if (lpTraceFILE)
		{
			fprintf(lpTraceFILE," Total Reference Beacon Count %d\n",m_RefCount);
			fclose(lpTraceFILE);
		}
	}

	return hr;
}

// These two fields are fixed-width character arrays written straight to the wire,
// not C strings, so "empty" is a full array of '0' rather than a terminator.
//
// This used to be done with memcpy( &cBeaconID[i], "0", 8 ) in a loop over every
// index, which wrote eight bytes at each offset and so ran seven bytes past the end
// of the array on its last iteration. It also read eight bytes from a two-byte
// string literal. The damage stayed small by luck rather than design: the visible
// characters came out right, because each iteration wrote '0' at its own offset
// before the next overwrote the tail, and of the seven bytes past cBeaconMess five
// land in struct padding, leaving two in the low-order mantissa bits of
// RefLocation.degLatitude. Which of those bytes were non-zero depended on whatever
// the linker happened to place after the literal, so the whole thing was undefined
// behaviour that merely presented as harmless in this build.
void
CEMSDBFDataMgr::ClearRefBeacon( EMSDBFPASSRECORD2 *PassRecord )
{
	if ( !PassRecord ) return;

	memset( PassRecord->cBeaconID,   '0', sizeof(PassRecord->cBeaconID) );
	memset( PassRecord->cBeaconMess, '0', sizeof(PassRecord->cBeaconMess) );

	memset( &PassRecord->RefLocation, 0, sizeof(PassRecord->RefLocation) );
	PassRecord->fRefTxFrequency = 0.0f;
}

//---------------------------------------------------------------------------

BOOL
CEMSDBFDataMgr::CheckRefBeacon( EMSDBFPASSRECORD2 *PassRecord )
{
	BOOL bOK = false;;

	if ( &PassRecord && &m_RefBeacons )
	{

		CEMSTime  oTime = PassRecord->timestamp;
		EMSTIMEFIELDS tmFields;
		oTime.GetTime(&tmFields);
		int iDaySecond = tmFields.nHour*3600 + tmFields.nMinute*60 + tmFields.nSecond;

		// Assume each beacon starts at the exact second mark requiring an offset of 1 second 
		//iDaySecond += 1;

		ClearRefBeacon( PassRecord );

		//BOOL bOK = false;
		for ( int i = 0; i < m_RefCount; i++ )
		{
			// Assume the data collection is a 2  second window with the beacon transmitting in the centre of it.
			
			int itime = iDaySecond % (m_RefBeacons.rec[i].nRrate);

			if ( itime == m_RefBeacons.rec[i].nStartTime )
			{
				// fill in info
				memcpy( &PassRecord->cBeaconID,  &m_RefBeacons.rec[i].cBeaconID[0], 15 );
				memcpy( &PassRecord->cBeaconMess,  &m_RefBeacons.rec[i].cBeaconMess[0], 36 );
				PassRecord->RefLocation.degLatitude  = m_RefBeacons.rec[i].refLocation.degLatitude;
				PassRecord->RefLocation.degLongitude = m_RefBeacons.rec[i].refLocation.degLongitude;
				PassRecord->RefLocation.fAltitude    = m_RefBeacons.rec[i].refLocation.fAltitude;
				PassRecord->fRefTxFrequency          = m_RefBeacons.rec[i].fFrequency;
				bOK = true;
				break;
			}
		}
	}

	return bOK;
}
