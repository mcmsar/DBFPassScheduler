/*********************************************************************
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

	$Log:
	$
********************************************************************/


#pragma warning(disable:4786)
#pragma warning( disable:4503)

#include <windows.h>
#include <stdio.h> // debug
#include <iostream>

#include "ESecDesc.h"

#include <xercesc/util/PlatformUtils.hpp>

#include "emscmreg.h"
#include "aobjbase.h"

#include "EMSDBFPassScheduler.h"
#include "Version.h"
#include "emsvectr.h"
#include "DBFNullPointing.h"
#include "logclient.h"
#include "emsconfg.h"
#include "configurationaccessor.h"
#include "emsexcpt.h"
#include "leosite.h"
#include "emsconst.h"
#include "LogHelper.h"
#include "DBFPSConfig.h"

const TCHAR c_szServiceName[] = TEXT("TSIDBFPassScheduler");
const TCHAR c_szComponent[]		= TEXT("TSI DBF Pass Scheduler Service");
const TCHAR c_szDescription[]	= TEXT("TSIDBFPassScheduler");

const ULONG c_ulTimeoutInterval	= 1000 * 10;// every 1 minute
const ULONG c_ulSleepInterval	= 	250;
const ULONG c_ulShutdownWait		= 500; // wait one half (1/2) second

const double c_dTimeSpan = 20.0; // 10 hours

CEMSDBFSchedulerService::CEMSDBFSchedulerService() : m_dwRegister(0), m_ulMinObjCount(0), m_bStopEnabled(FALSE),
								m_hEventSignal(INVALID_HANDLE_VALUE), m_hEventStop(INVALID_HANDLE_VALUE)		
{
	m_dTimeSpan = c_dTimeSpan;
	m_LastGenTime.intTime = 0L;
	m_ulPlates = 0;
	m_ulLutId = 0;

	m_hEventSignal = CreateEvent( NULL, FALSE, FALSE, NULL );

	m_hEventStop = CreateEvent( NULL, FALSE, FALSE, NULL );
}

CEMSDBFSchedulerService::~CEMSDBFSchedulerService()
{
	if ( m_hEventStop != INVALID_HANDLE_VALUE )
	{
		CloseHandle( m_hEventStop );
		m_hEventStop = INVALID_HANDLE_VALUE;
	}

	if ( m_hEventSignal != INVALID_HANDLE_VALUE )
	{
		CloseHandle( m_hEventSignal );
		m_hEventSignal = INVALID_HANDLE_VALUE;
	}

	_ReleaseObjects();
}

void	
CEMSDBFSchedulerService::_ReleaseObjects( void )
{
}

void	
CEMSDBFSchedulerService::_DoIdle()
{
	if( _TimeToGeneratePassSched() )
	{
		// generate starting the next hour but set the last time to hh:10.
		CEMSTime oTempTime = m_oSysTime.GetTime();
		EMSTIMEFIELDS tmTimeFields;
		oTempTime.GetTime( &tmTimeFields );

		CEMSTime  oCurrentTime( tmTimeFields.nYear, tmTimeFields.nMonth, tmTimeFields.nDay, tmTimeFields.nHour );
		
		m_LastGenTime = oCurrentTime;
		oTempTime.SetTimeNano(m_LastGenTime.intTime);
		m_LastGenTime = oTempTime.AddMinutes(10.0);

		CEMSTime  oStartTime = oCurrentTime;
		CEMSTime  oEndTime = oCurrentTime;

		oEndTime.AddHours(23.0);

		FILE *lpTraceFILE =  CDBFPSConfig::Instance().OpenTraceLog();
		if (lpTraceFILE)
		{
			// Build stamp on every schedule block as well as at startup: trace
			// pruning can drop the startup line, but keeps whole blocks. It goes
			// after the date, which is all _ParseTraceAnchorDate reads.
			fprintf(lpTraceFILE,"Generating new pass schedule : %4d/%2d/%2d %2d:%2d:%2d  [%s %s]\n",
				tmTimeFields.nYear, tmTimeFields.nMonth, tmTimeFields.nDay,
				tmTimeFields.nHour, tmTimeFields.nMinute, tmTimeFields.nSecond,
				DBF_PASS_SCHEDULER_VERSION, BUILD_GIT_HASH );
			fclose(lpTraceFILE);
		}

		_GenerateNewPassSched( oStartTime, oEndTime );
	}
}

bool 
CEMSDBFSchedulerService::_TimeToGeneratePassSched()
{
	bool bRet = false;
	if( 0 == m_LastGenTime.intTime )
	{
		bRet = true;
	}
	else
	{
		// check one hour.
		CEMSTime oCurrentTime = m_oSysTime.GetTime();
		CEMSTime oNext(m_LastGenTime);
		oNext.AddHours(1.0);
		EMSTIME tmNext = oNext;
		EMSTIME tmCurrent = oCurrentTime;

		if( tmCurrent.intTime >= tmNext.intTime )
		{
			bRet = true;
		}
	}
	return bRet;
}

bool elevation_sorter(EMSDBFAZEL const& lhs, EMSDBFAZEL const& rhs) {
	//return lhs.degSatElevation > rhs.degSatElevation;
	return lhs.degPlateElevation > rhs.degPlateElevation;
}

// Splits a combiner's <constellation> value (e.g. "BEIDOU,GLONASS") into a
// primary constellation name and an optional secondary (fallback) name.
static void _SplitConstellationNames( const std::string& szConfigured, std::string& szPrimary, std::string& szSecondary )
{
	size_t nComma = szConfigured.find(',');

	std::string szFirst  = ( nComma == std::string::npos ) ? szConfigured : szConfigured.substr(0, nComma);
	std::string szSecond  = ( nComma == std::string::npos ) ? std::string() : szConfigured.substr(nComma + 1);

	// trim whitespace
	const char* szWhitespace = " \t\r\n";
	size_t nStart = szFirst.find_first_not_of(szWhitespace);
	size_t nEnd   = szFirst.find_last_not_of(szWhitespace);
	szPrimary = ( nStart == std::string::npos ) ? std::string() : szFirst.substr(nStart, nEnd - nStart + 1);

	nStart = szSecond.find_first_not_of(szWhitespace);
	nEnd   = szSecond.find_last_not_of(szWhitespace);
	szSecondary = ( nStart == std::string::npos ) ? std::string() : szSecond.substr(nStart, nEnd - nStart + 1);
}

std::vector<EMSDBFPASATCFG>&
CEMSDBFSchedulerService::_GetSatVectorForConstellation( const std::string& szConstellationName )
{
	static std::vector<EMSDBFPASATCFG> s_vectEmpty;

	if ( szConstellationName == "BEIDOU" )       return m_vectBeidouSats;
	else if ( szConstellationName == "GALILEO" ) return m_vectGalileoSats;
	else if ( szConstellationName == "LEO" )     return m_vectLeoSats;
	else if ( szConstellationName == "GLONASS" ) return m_vectGlonassSats;

	return s_vectEmpty;
}

// Elevation thresholds a satellite must clear to actually be scheduled. These used
// to be compile-time constants here; they now come from <satminelevation>,
// <satmaxelevation>, <plateminelevationleo> and <plateminelevationmeo> on the plate
// in DBFPassSchedule.xml, defaulted in CEMSDBFPassScheduleCfg to the values that
// were hardcoded. The satellite elevation window can additionally be set per class
// with <satmin/maxelevationleo> and <satmin/maxelevationmeo>. This is the single implementation of the criteria, so the fallback
// decision in _HasSchedulableSatellite and the real bPassOK decision below cannot
// drift apart.
static bool _IsSchedulable( const EMSDBFAZEL& azel, const EMSDBFPLATECFG& plateCfg )
{
	// Thresholds by constellation class. Ids between 200 and 400 match neither
	// class and are never schedulable here.
	float fSatMinElevation, fSatMaxElevation, fPlateMinElevation;
	if ( azel.ulSatID < 200 )
	{
		fSatMinElevation   = plateCfg.fSatMinElevationLEO;
		fSatMaxElevation   = plateCfg.fSatMaxElevationLEO;
		fPlateMinElevation = plateCfg.fPlateMinElevationLEO;
	}
	else if ( azel.ulSatID > 400 )
	{
		fSatMinElevation   = plateCfg.fSatMinElevationMEO;
		fSatMaxElevation   = plateCfg.fSatMaxElevationMEO;
		fPlateMinElevation = plateCfg.fPlateMinElevationMEO;
	}
	else
	{
		return false;
	}

	// Satellite elevation above the local horizon. The upper bound is a site
	// blockage limit, not a geometry one.
	if ( ( azel.degSatElevation <= fSatMinElevation )
		|| ( azel.degSatElevation >= fSatMaxElevation ) )
	{
		return false;
	}

	// Floor on the angle off the plate boresight.
	return azel.degPlateElevation > fPlateMinElevation;
}

// True if at least one entry in aAzEls would actually clear the bPassOK criteria
// used below -- i.e. would really be scheduled, not merely above the site/plate's
// much looser pipeline cutoff used to populate aAzEls in the first place.
static bool _HasSchedulableSatellite( const EMSDBFAZEL* aAzEls, ULONG ulActual, const EMSDBFPLATECFG& plateCfg )
{
	for ( ULONG i = 0; i < ulActual; i++ )
	{
		if ( _IsSchedulable( aAzEls[i], plateCfg ) ) return true;
	}
	return false;
}

void
CEMSDBFSchedulerService::_GenerateNewPassSched( EMSTIME startTime, EMSTIME endTime )
{
	ULONG			  ulActual = 0;
	EMSDBFAZEL	      aAzEls[culMaxSats]={{0}};
	EMSDBFAZEL	      aAzElsSecondary[culMaxSats]={{0}};
	EMSTLEDATA2		  aTLE2[culMaxSats]={{0}};

	EMS_RESULT hr = EMS_OK;

	// Everything per plate - acceptance window, scheduling thresholds, location and
	// combiners - lives in DBFPassSchedule.xml, not in the plate file that SetPlate()
	// below comes from. All of it is taken from this plate's own entry: these used to
	// be single members overwritten per plate in _Init, so every plate was written to
	// the last plate's combiners and each plate truncated the previous one's files.
	// m_ulPlates is this vector's own size, so the index is in range; the guard only
	// covers a malformed config.
	const std::vector<EMSDBFPLATECFG>& vectPlateCfg = m_oDBFPassSchedCfg.GetDbfPassScheduleCfg().vectDbfPlate;

	for(ULONG nPlate = 0; nPlate < m_ulPlates; nPlate++)
	{
		if ( nPlate >= vectPlateCfg.size() ) continue;

		// No plate geometry was loaded for this plate - its file was missing or
		// unreadable in both formats (ReadDBFPlateConfigFromXML logs which and why).
		// SetPlate() below copies m_vectPlates[nPlate] wholesale, so indexing an
		// empty vector here was an access violation in VCRUNTIME140 on every start.
		if ( nPlate >= m_vectPlates.size() || nPlate >= m_vectPlateLoaded.size() || !m_vectPlateLoaded[nPlate] )
		{
			FILE* lpTrace = CDBFPSConfig::Instance().OpenTraceLog();
			if ( lpTrace )
			{
				fprintf( lpTrace,
					"*** Plate %lu: no plate configuration loaded from '%s' - skipping pass schedule "
					"generation for this plate (see the DBF PLATE lines above) ***\n",
					nPlate + 1, vectPlateCfg[nPlate].szPlateCfgFile.c_str() );
				fclose( lpTrace );
			}
			continue;
		}

		const EMSDBFPLATECFG& plateCfg = vectPlateCfg[nPlate];
		const std::vector<EMSDBFCOMBINERCFG>& vectCombiners = plateCfg.vectDbfCombiner;
		const WORD wPlateId = plateCfg.wPlateId;

		EMSDBFPASITELOCATIONCFG siteLocation;
		memset( &siteLocation, 0, sizeof(EMSDBFPASITELOCATIONCFG) );
		siteLocation.ulLutId = m_ulLutId;
		memcpy( &siteLocation.locnAntenna, &plateCfg.plateLocation, sizeof(EMSLOCATION) );

		ULONG nCombBoardCount  = vectCombiners.size();

		for(ULONG nCombBoard = 0; nCombBoard < nCombBoardCount; nCombBoard++)
		{
			m_oDMgr.Initialize();
			CDBFPointingAngles oPA;
			CDBFPointingAngles oPASecondary;
			CDBFNullPointing oNP;
			CEMSCoordSystem	oCoord;
			//CEMSRangeRate oRange;

			EMSDBFPASSRECORD2	PassRecord;
			EMSLOCATION refLocation;
			EMSVECTOR refPosition;
			EMSVECTOR sitePosition;
			EMSVECTOR vDiff, vTemp;
			EMSTIMECOORD timeCoord;
			EMSRANGERATE UPRangeRate;
			EMSRANGERATE DOWNRangeRate;
			double dConstellationFrequency;
			double dWaveLength;
			float fUpDoppler;
			float fDownDoppler;
			float fDownFrequency;
			float fRefFrequency;
			float fCosElevation;
			ULONG xx = sizeof(PassRecord);

			BOOL bOK = false;

			double dScaleFactor = 4096; // 2^12 temporary test RR
			
			std::vector<EMSDBFPASATCFG> ulVectAllSats;
			std::vector<EMSDBFPASATCFG> ulVectAllSatsSecondary;
			std::string szCombinerIP = vectCombiners[nCombBoard].szIpAddress;

			// The <constellation> value may name a single constellation ("BEIDOU") or a
			// primary and fallback pair ("BEIDOU,GLONASS"). When no primary satellite is
			// in view at a given time step, the fallback constellation is tried instead.
			std::string szPrimaryConstellation, szSecondaryConstellation;
			_SplitConstellationNames( vectCombiners[nCombBoard].szConstellation, szPrimaryConstellation, szSecondaryConstellation );

			ulVectAllSats = _GetSatVectorForConstellation( szPrimaryConstellation );

			bool bHasSecondaryConstellation = !szSecondaryConstellation.empty();
			if ( bHasSecondaryConstellation )
			{
				ulVectAllSatsSecondary = _GetSatVectorForConstellation( szSecondaryConstellation );
			}

			hr = oPA.Initialize(ulVectAllSats, siteLocation);
			hr = oNP.Initialize();

			oPA.SetPlate(&m_vectPlates[nPlate]);

			if ( bHasSecondaryConstellation )
			{
				hr = oPASecondary.Initialize(ulVectAllSatsSecondary, siteLocation);
				oPASecondary.SetPlate(&m_vectPlates[nPlate]);
			}

			oPA.SetPlateLimits( plateCfg.fPlateMinElevation,
								plateCfg.fPlateMinAzimuth,
								plateCfg.fPlateMaxAzimuth );

			if ( bHasSecondaryConstellation )
			{
				oPASecondary.SetPlateLimits( plateCfg.fPlateMinElevation,
											 plateCfg.fPlateMinAzimuth,
											 plateCfg.fPlateMaxAzimuth );
			}

			ULONG irec = 0;

			EMSTIME timeNow = startTime;
			int nCurrentHour = -1;

			while( timeNow.intTime < endTime.intTime  )
			{
				CEMSTime oHourCheck = timeNow;
				EMSTIMEFIELDS tmHourCheck;
				oHourCheck.GetTime( &tmHourCheck );
				if ( tmHourCheck.nHour != nCurrentHour )
				{
					nCurrentHour = tmHourCheck.nHour;
					m_oDMgr.EnsureHourFileOpen( nCurrentHour, szCombinerIP );
				}

				memset( aAzEls, 0, sizeof(EMSDBFAZEL) * culMaxSats);

				// Tracks which constellation's data is actually driving this time step's
				// pass record, so it can be logged and so downstream lookups (satellite
				// frequency/polarization, wavelength, ECI vector) use the right source.
				CDBFPointingAngles* pActivePA = &oPA;
				std::vector<EMSDBFPASATCFG>* pActiveSatVector = &ulVectAllSats;
				std::string szActiveConstellation = szPrimaryConstellation;

				if( m_vectPlates[nPlate].wFixedPointNumber  > 0 )
				{
					oPA.GetFixedPointSched( aAzEls, &ulActual );
				}
				else
				{
					hr = oPA.GetSatelliteAZELs( timeNow, aAzEls, &ulActual );
					hr = oPA.CheckLEOMEO( aAzEls, &ulActual );
					hr = oPA.GetPlateAZELs( aAzEls, &ulActual );
					hr = oPA.GetConstellationAZELs( aAzEls, &ulActual );
					hr = oPA.GetDBFPhase( aAzEls, &ulActual );

					if ( bHasSecondaryConstellation && !_HasSchedulableSatellite( aAzEls, ulActual, plateCfg ) )
					{
						// Primary constellation has nothing that will actually clear the
						// scheduling threshold at this time step (it may still have shown
						// up in aAzEls/ulActual above -- that only reflects the much looser
						// horizon pipeline cutoff). Try the secondary constellation instead.
						ULONG ulActualSecondary = 0;
						memset( aAzElsSecondary, 0, sizeof(EMSDBFAZEL) * culMaxSats);

						hr = oPASecondary.GetSatelliteAZELs( timeNow, aAzElsSecondary, &ulActualSecondary );
						hr = oPASecondary.CheckLEOMEO( aAzElsSecondary, &ulActualSecondary );
						hr = oPASecondary.GetPlateAZELs( aAzElsSecondary, &ulActualSecondary );
						hr = oPASecondary.GetConstellationAZELs( aAzElsSecondary, &ulActualSecondary );
						hr = oPASecondary.GetDBFPhase( aAzElsSecondary, &ulActualSecondary );

						if ( _HasSchedulableSatellite( aAzElsSecondary, ulActualSecondary, plateCfg ) )
						{
							memcpy( aAzEls, aAzElsSecondary, sizeof(EMSDBFAZEL) * ulActualSecondary );
							ulActual = ulActualSecondary;
							pActivePA = &oPASecondary;
							pActiveSatVector = &ulVectAllSatsSecondary;
							szActiveConstellation = szSecondaryConstellation;
						}
					}

					if (ulActual > 5)
					{
						float x = 1.0;
					}
				}

				memset( &PassRecord, 0, sizeof(EMSDBFPASSRECORD2) );

				PassRecord.wProcessID = 0; // do nothing
				PassRecord.ulLutID	  = m_ulLutId;
				PassRecord.wPlateID	  = wPlateId;
				PassRecord.ulPassID	  = m_ulPassId;
			
				bool bTestSun = true;
				if ( ulActual == 0 && PassRecord.wProcessID > 0 )
				{
					// Write Pass Records
					PassRecord.timestamp  = timeNow;
					//PassRecord.wAntennaID = m_vectPlates[nPlate].wPlateID;
					PassRecord.wAntennaID = vectCombiners[nCombBoard].wCombinerId;
					hr = m_oDMgr.WritePassSchedule( PassRecord, szCombinerIP, bTestSun, szActiveConstellation );
				}
				else if ( ulActual > 0 )
				{
					bool bPassOK = false;

					std::sort(aAzEls, aAzEls + ulActual, &elevation_sorter);

					// Publish at most MaxSatellites records for this time step. The sort
					// above is by descending plate elevation, so the ones dropped are the
					// lowest-elevation satellites. These are the same records the collector
					// was already discarding on read -- CEMSDBFDataMgr::_AddPassRecord stops
					// at DBF_MAX_SATELLITES per second -- so this does not lose anything that
					// was reaching the beamformer. It just makes the cut deliberate and
					// visible here instead of silent over there.
					//
					// ulActual is deliberately left alone: it is tested for == 1 below to
					// pick the frequency-domain algorithm, and that test is about how many
					// satellites are in view, not how many are being scheduled.
					ULONG ulToSchedule = ulActual;
					const ULONG culMaxToSchedule = CDBFPSConfig::Instance().MaxSatellites();

					if ( ulToSchedule > culMaxToSchedule )
					{
						FILE* lpTrace = CDBFPSConfig::Instance().OpenTraceLog();
						if ( lpTrace )
						{
							fprintf( lpTrace,
								"Scheduling %lu of %lu satellites in view (MaxSatellites=%lu);"
								" dropping the %lu lowest by plate elevation.\n",
								culMaxToSchedule, ulActual, culMaxToSchedule,
								ulActual - culMaxToSchedule );
							fclose( lpTrace );
						}

						ulToSchedule = culMaxToSchedule;
					}

					for ( ULONG j = 0; j < ulToSchedule; j++ )
					{
						memset( &PassRecord, 0, sizeof(EMSDBFPASSRECORD2) );
					
						PassRecord.ulLutID	= m_ulLutId;
						
						int nSatType = pActivePA->GetConstellationType( aAzEls[j].ulSatID );

						if ( ( aAzEls[j].ulSatID < 300 ) && ( ulActual == 1 ) )
						{
							PassRecord.wProcessID = 1; // Perform Frequency Domain method
						}
						else
						{
							PassRecord.wProcessID = 2; // Perform Time Domain method
						}

						// Check satellite elevation against the plate's configured window
						// (the upper bound is TSI in-house DBF only -- blockage).
						bPassOK = _IsSchedulable( aAzEls[j], plateCfg );

						if ( bPassOK )
						{
							PassRecord.timestamp  = timeNow;
							//PassRecord.wAntennaID      = m_vectPlates[nPlate].wPlateID;
							PassRecord.wAntennaID	   = vectCombiners[nCombBoard].wCombinerId;
							PassRecord.ulSatID         = aAzEls[j].ulSatID;
							PassRecord.ulPassID        = aAzEls[j].ulPassID;

							for ( ULONG s = 0; s < pActiveSatVector->size(); s++ )
							{
								if ( (*pActiveSatVector)[s].ulSatId == aAzEls[j].ulSatID )
								{
									// Config carries MHz as a double; the record carries whole Hz.
									PassRecord.ulDownFrequencyHz = EMSDBF_HZ_FROM_MHZ( (*pActiveSatVector)[s].fFrequency );
									PassRecord.wPolarization  = ( (*pActiveSatVector)[s].szPolarisation == "RHCP" ) ? 1 : 0;
									break;
								}
							}

							PassRecord.fAzimuth        = (float) aAzEls[j].degSatAzimuth;
							PassRecord.fElevation      = (float) aAzEls[j].degSatElevation;
							PassRecord.fPlateAzimuth   = (float) aAzEls[j].degPlateAzimuth;
							PassRecord.fPlateElevation = (float) aAzEls[j].degPlateElevation;
							PassRecord.wPlateID		   = wPlateId;
							memcpy( &PassRecord.timeCoord, &aAzEls[j].timeCoord, sizeof(EMSTIMECOORD) );

							// RR the number of channels is one more than the number of cells
							for ( ULONG k = 0; k < m_vectPlates[nPlate].wCellNumber + 1; k++ )
							{
								float fPhaseAngle = atan2( aAzEls[j].acmplxPhase[k].im, aAzEls[j].acmplxPhase[k].re );
								PassRecord.fPhase[k] = fPhaseAngle * c_dRadToDeg;
							}

							// Check time for Reference Beacon
										
							bOK = m_oDMgr.CheckRefBeacon( &PassRecord );
							if (bOK )
							{

								refPosition  = oCoord.ToEarthFixed( PassRecord.RefLocation );
								sitePosition = oCoord.ToEarthFixed( siteLocation.locnAntenna );
							    
								bool bEarthFixed = true;
								hr = pActivePA->GetSatelliteVectorEF( timeNow,  PassRecord.ulSatID, &timeCoord );
								
								// Check Reference Beacon visibibility
								vDiff = CEMSVector::Sub( timeCoord.coord.radius, refPosition);
								vDiff =	CEMSVector::Normalize(vDiff);
								vTemp = CEMSVector::Normalize(refPosition);
								fCosElevation = CEMSVector::DotProduct(vDiff,vTemp);
								PassRecord.fBeaconElevation = asin(fCosElevation) * c_dRadToDeg;

								if( PassRecord.fBeaconElevation > 0.0 )
								{
									timeCoord.coordSystem = COORDSYS_EarthFixed;
									hr = CEMSRangeRate::Calculate( refPosition, &timeCoord, 1, &UPRangeRate );
									hr = CEMSRangeRate::Calculate( sitePosition, &timeCoord, 1, &DOWNRangeRate );

									PassRecord.fTOA = (UPRangeRate.fRange + DOWNRangeRate.fRange) / c_dVelocityOfLight;

									double dV = DOWNRangeRate.fRangeRate / c_dVelocityOfLight;

									//fUpDoppler = (PassRecord.fRefTxFrequency + 406.0e6) * UPRangeRate.fRangeRate / c_dVelocityOfLight;
									double dNominalUpFreq = (double)PassRecord.fRefTxFrequency + 406.0e6;
									fUpDoppler = (dNominalUpFreq / (1.0 + UPRangeRate.fRangeRate / c_dVelocityOfLight) - dNominalUpFreq);

									pActivePA->GetConstellation( PassRecord.ulSatID, &dWaveLength, &dConstellationFrequency );
			
									fDownFrequency = dConstellationFrequency + (double)(PassRecord.fRefTxFrequency -50000.0 + fUpDoppler); 
									
									fDownDoppler = -(1.0 - sqrt( (1.0-dV)/(1.0+dV) )) * fDownFrequency;
						
									PassRecord.fFOA = PassRecord.fRefTxFrequency + fDownDoppler + fUpDoppler;

								}
								else
								{
									// Beacon is below this satellite's horizon, so there is no
									// FOA or TOA to compute. CheckRefBeacon has already stamped
									// the beacon id, message, location and transmit frequency
									// into the record, and leaving them there publishes a
									// beacon with fFOA and fTOA still at zero -- indistinguishable
									// downstream from a measured zero. Put the record back into
									// the "no beacon this second" state instead. fBeaconElevation
									// is deliberately left as computed: it is what says why the
									// beacon was dropped.
									CEMSDBFDataMgr::ClearRefBeacon( &PassRecord );
								}
							}
							hr = m_oDMgr.WritePassSchedule( PassRecord, szCombinerIP, bTestSun, szActiveConstellation );
						}
					}

				}

				irec++;

				timeNow.intTime += c_iNanoSecsPerSec;
			}  //End while

			m_oDMgr.Reset(); //Close the last hour file descriptors

		} //End Loop for Combiner boards

	}  //End Loop for plates
}


EMS_RESULT 
CEMSDBFSchedulerService::_ServiceProcessLoop( void )
{
	EMS_RESULT hr = EMS_OK;

	try
	{
		BOOL bStopped = FALSE;

		HANDLE hEvent[2];
		hEvent[ 0 ] = m_hEventStop;
		hEvent[ 1 ] = m_hEventSignal;

		while ( !bStopped )
		{
			DWORD dwResult = WaitForMultipleObjects( 2, hEvent, FALSE, c_ulTimeoutInterval );

			switch ( dwResult )
			{
				case WAIT_OBJECT_0:		// asked to stop 
					{
						bStopped = TRUE;
					}
					break;

				case WAIT_OBJECT_0 + 1:	// signalled!
					break;

				default:
				case WAIT_TIMEOUT:
					{
						if( _OkayToStop() != m_bStopEnabled )
						{
							// this will recheck and reset SERVICE_ACCEPT_STOP
							ReportServiceStatus( EMS_SERVICE_RUNNING );
						}
						_DoIdle();
					}
					break;
			}
		}
	}
	catch( CEMSException& e )
	{
		CEMSLogHelper::LogException( e );
		hr = e.GetErrCode();
	}
	catch( ... )
	{
		hr = EMS_UNKNOWN_ERROR;
	}

	return hr;
}

BOOL 
CEMSDBFSchedulerService::StopIsAccepted( void )
{
	m_bStopEnabled = _OkayToStop();
	//return m_bStopEnabled;
	return TRUE;
}

BOOL 
CEMSDBFSchedulerService::_OkayToStop( void )
{
	return ( CApiObjBase::ObjectCount() == 0 );
}

EMS_RESULT
CEMSDBFSchedulerService::StartEMSService( DWORD dwArgc, LPTSTR *lpszArgv )
{
	xercesc::XMLPlatformUtils::Initialize();

	EMS_RESULT hr = EMS_OK;
	int nMaxRetries = c_ulShutdownWait / c_ulSleepInterval, nRetries = 0;

	m_dwThreadID = GetCurrentThreadId();

	CEMSSecurityDescriptor secDesc;

	ReportServiceStatus( EMS_SERVICE_RUNNING );

	m_ulMinObjCount = CApiObjBase::ObjectCount();

	secDesc.InitializeFromThreadToken();

	hr = _Init();

	//if( SUCCEEDED(hr) )
	//{
	//	CEMSLogHelper::LogMsg( EMSLogMsgInfo, EMS_TS_STARTED );
	//}
	//else
	//{
	//	CEMSLogMsgParam oParam;
	//	oParam.AddX( hr );

	//	CEMSLogHelper::LogMsg( EMSLogMsgAlarm, EMS_TS_START_FAILED, oParam );
	//}


	if(hr == EMS_OK)
	{
		hr = _ServiceProcessLoop();

		if( FAILED(hr) )
		{
			// Log a warning.
		}

		_ReleaseObjects();

		//CEMSLogHelper::LogMsg( EMSLogMsgInfo, EMS_TS_STOPPED );	
	}

	while ( CApiObjBase::ObjectCount() && ( nRetries++ < nMaxRetries ) )
	{
		Sleep( c_ulSleepInterval );
	}

	xercesc::XMLPlatformUtils::Terminate();

	return hr;
}

EMS_RESULT
CEMSDBFSchedulerService::_Init()
{
	EMS_RESULT hr = EMS_OK;

	try
	{
		CDBFPSConfig::Instance().Load();

		std::string szConfigFile = CDBFPSConfig::Instance().PassScheduleFile();

		hr = m_oDBFPassSchedCfg.ReadDBFConfigFromXML(szConfigFile);

		{
			FILE* lpT = CDBFPSConfig::Instance().OpenTraceLog();
			if ( lpT )
			{
				fprintf( lpT, "=== %s starting ===\n", DBF_PASS_SCHEDULER_BUILD_STRING );
				fprintf( lpT, "_Init: PassScheduleFile=%s  ReadDBFConfigFromXML hr=0x%08X\n",
					szConfigFile.c_str(), hr );
				fclose( lpT );
			}
		}

		hr = m_oDMgr.ReadRefBeacons();

		if(hr == EMS_OK)
		{
			EMSDBFPASSSCHEDULECFG oPassCfg = m_oDBFPassSchedCfg.GetDbfPassScheduleCfg();

			m_ulLutId = oPassCfg.ulLutId;
			m_ulPlates = oPassCfg.vectDbfPlate.size();


			m_vectPlates.clear();
			m_vectPlateLoaded.clear();

			for(ULONG i = 0; i < m_ulPlates; i++)
			{
				// ReadDBFPlateConfigFromXML appends to GetDBFPlatesCfg() only when the
				// file loads, so its index does not follow the config's. Keep
				// m_vectPlates aligned with vectDbfPlate by taking whatever it appended,
				// or a zeroed placeholder flagged as not loaded.
				const size_t nBefore = m_oDBFPassSchedCfg.GetDBFPlatesCfg().size();
				m_oDBFPassSchedCfg.ReadDBFPlateConfigFromXML(oPassCfg.vectDbfPlate[i].szPlateCfgFile);

				EMSDBFARRAY plate;
				memset( &plate, 0, sizeof(EMSDBFARRAY) );
				const bool bLoaded = m_oDBFPassSchedCfg.GetDBFPlatesCfg().size() > nBefore;
				if ( bLoaded )
					plate = m_oDBFPassSchedCfg.GetDBFPlatesCfg().back();

				m_vectPlates.push_back( plate );
				m_vectPlateLoaded.push_back( bLoaded );

				if ( !bLoaded ) continue;

				m_oDMgr.Initialize( oPassCfg.vectDbfPlate[i].vectDbfCombiner, plate.wCellNumber );
			}

			m_vectLeoSats.clear();
			m_vectGalileoSats.clear();
			m_vectBeidouSats.clear();
			m_vectGlonassSats.clear();

			int nLeoSatCount = oPassCfg.vectLeoSats.size();
			int nGaliLeoCount = oPassCfg.vectGalileoSats.size();
			int nBeidouSatCount = oPassCfg.vectBeidouSats.size();
			int nGlonassSatCount = oPassCfg.vectGlonassSats.size();

			for(int i =0; i < nLeoSatCount; i++)
			{
				EMSDBFPASATCFG satCfg;
				satCfg.szType = oPassCfg.vectLeoSats[i].szType;
				satCfg.fFrequency = oPassCfg.vectLeoSats[i].fFrequency;
				satCfg.szPolarisation = oPassCfg.vectLeoSats[i].szPolarisation;
				satCfg.ulSatId = oPassCfg.vectLeoSats[i].ulSatId;
				satCfg.ulPriority = oPassCfg.vectLeoSats[i].ulPriority;

				//memcpy(&satCfg, &oPassCfg.vectLeoSats[i], sizeof(EMSDBFPASATCFG));
				m_vectLeoSats.push_back(satCfg);
			}

			for(int j = 0; j < nGaliLeoCount; j++)
			{
				EMSDBFPASATCFG satCfg;
				//memcpy(&satCfg, &oPassCfg.vectGalileoSats[j], sizeof(EMSDBFPASATCFG));
				satCfg.szType = oPassCfg.vectGalileoSats[j].szType;
				satCfg.fFrequency = oPassCfg.vectGalileoSats[j].fFrequency;
				satCfg.szPolarisation = oPassCfg.vectGalileoSats[j].szPolarisation;
				satCfg.ulSatId = oPassCfg.vectGalileoSats[j].ulSatId;
				satCfg.ulPriority = oPassCfg.vectGalileoSats[j].ulPriority;
				m_vectGalileoSats.push_back(satCfg);
			}

			for(int k = 0; k < nBeidouSatCount; k++)
			{
				EMSDBFPASATCFG satCfg;
				//memcpy(&satCfg, &oPassCfg.vectBeidouSats[k], sizeof(EMSDBFPASATCFG));
				satCfg.szType = oPassCfg.vectBeidouSats[k].szType;
				satCfg.fFrequency = oPassCfg.vectBeidouSats[k].fFrequency;
				satCfg.szPolarisation = oPassCfg.vectBeidouSats[k].szPolarisation;
				satCfg.ulSatId = oPassCfg.vectBeidouSats[k].ulSatId;
				satCfg.ulPriority = oPassCfg.vectBeidouSats[k].ulPriority;
				m_vectBeidouSats.push_back(satCfg);
			}

			for(int m = 0; m < nGlonassSatCount; m++)
			{
				EMSDBFPASATCFG satCfg;
				satCfg.szType = oPassCfg.vectGlonassSats[m].szType;
				satCfg.fFrequency = oPassCfg.vectGlonassSats[m].fFrequency;
				satCfg.szPolarisation = oPassCfg.vectGlonassSats[m].szPolarisation;
				satCfg.ulSatId = oPassCfg.vectGlonassSats[m].ulSatId;
				satCfg.ulPriority = oPassCfg.vectGlonassSats[m].ulPriority;
				m_vectGlonassSats.push_back(satCfg);
			}
		}
	}
	catch(const xml_schema::exception& e)
	{
		//std::cout<<"Exception is "<<e<<std::endl;
		hr = EMS_FALSE;
	}
	catch(...)
	{
		hr = EMS_FALSE;
		//throw;

	}

	return hr;
}

int main(int argc, char* argv[])
{
	printf( "%s\n", DBF_PASS_SCHEDULER_BUILD_STRING );
	if( argc > 1 && ( 0 == _stricmp( argv[1], "-version" ) || 0 == _stricmp( argv[1], "--version" ) ) )
		return 0;

	try
	{
		CEMSDBFSchedulerService oDBFPassSchedService;

		if( argc > 1 &&
			0 == stricmp( argv[1], "-debug" ) )
		{
			oDBFPassSchedService.RunAsService( FALSE );	// for DEBUGGING
		}

		oDBFPassSchedService.AllowPause( FALSE );
		oDBFPassSchedService.Run( oDBFPassSchedService.GetService(), argc, argv );
	}
	catch( CEMSException& e )
	{
		CEMSLogHelper::LogException( e );
	}
	catch( ... )
	{
		CEMSLogHelper::LogMsg( EMSLogMsgAlarm, EMS_EXCEPTION );
	}

	return 0;
}


EMS_RESULT 
CEMSDBFSchedulerService::StopEMSService( void ) 
{
	ReportServiceStatus( EMS_SERVICE_STOP_PENDING );

	SetEvent( m_hEventStop );

	return EMS_OK; 
}

EMS_RESULT 
CEMSDBFSchedulerService::PauseEMSService( void )
{
	if ( !m_bPaused )
	{
		m_bPaused = TRUE;

		SetEvent( m_hEventSignal );
	}
	return EMS_OK; 
}

EMS_RESULT 
CEMSDBFSchedulerService::ContinueEMSService( void )
{
	if ( m_bPaused )
	{
		m_bPaused = FALSE;

		SetEvent( m_hEventSignal );
	}
	return EMS_OK;
}