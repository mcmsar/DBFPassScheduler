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

const TCHAR c_szServiceName[] = TEXT("EMSDBFPassScheduler");
const TCHAR c_szComponent[]		= TEXT("EMS DBF Pass Scheduler Service");
const TCHAR c_szDescription[]	= TEXT("EMSDBFPassScheduler");

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

		FILE *lpTraceFILE =  fopen(CDBFPSConfig::Instance().TraceLogFile().c_str(),"at");
		if (lpTraceFILE)
		{
			fprintf(lpTraceFILE,"Generating new pass schedule : %4d/%2d/%2d %2d:%2d:%2d\n",
				tmTimeFields.nYear, tmTimeFields.nMonth, tmTimeFields.nDay,
				tmTimeFields.nHour, tmTimeFields.nMinute, tmTimeFields.nSecond );
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

void
CEMSDBFSchedulerService::_GenerateNewPassSched( EMSTIME startTime, EMSTIME endTime )
{
	ULONG			  ulActual = 0;
	EMSDBFAZEL	      aAzEls[culMaxSats]={{0}};
	EMSTLEDATA2		  aTLE2[culMaxSats]={{0}};

	EMS_RESULT hr = EMS_OK;

	for(ULONG nPlate = 0; nPlate < m_ulPlates; nPlate++)
	{
		ULONG nCombBoardCount  = m_vectDbfCombiner.size();

		for(ULONG nCombBoard = 0; nCombBoard < nCombBoardCount; nCombBoard++)
		{
			m_oDMgr.Initialize();
			CDBFPointingAngles oPA;
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
			std::string szCombinerIP;

			std::string szConstellation = m_vectDbfCombiner[nCombBoard].szConstellation;

			if (szConstellation == "BEIDOU")
			{
				ulVectAllSats = m_vectBeidouSats;
				szCombinerIP = m_vectDbfCombiner[nCombBoard].szIpAddress;
			}
			else if(szConstellation == "GALILEO")
			{
				ulVectAllSats = m_vectGalileoSats;
				szCombinerIP = m_vectDbfCombiner[nCombBoard].szIpAddress;
			}
			else if(szConstellation == "LEO")
			{
				ulVectAllSats = m_vectLeoSats;
				szCombinerIP = m_vectDbfCombiner[nCombBoard].szIpAddress;
			}

			hr = oPA.Initialize(ulVectAllSats, m_siteLocation);
			hr = oNP.Initialize();

			oPA.SetPlate(&m_vectPlates[nPlate]);

			m_wPlateId = nPlate + 1;

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
				
					if (ulActual > 5)
					{
						float x = 1.0;
					}
				}

				memset( &PassRecord, 0, sizeof(EMSDBFPASSRECORD2) );

				PassRecord.wProcessID = 0; // do nothing
				PassRecord.ulLutID	  = m_ulLutId;
				PassRecord.wPlateID	  = m_wPlateId;
				PassRecord.ulPassID	  = m_ulPassId;
			
				bool bTestSun = true;
				if ( ulActual == 0 && PassRecord.wProcessID > 0 )
				{
					// Write Pass Records
					PassRecord.timestamp  = timeNow;
					//PassRecord.wAntennaID = m_vectPlates[nPlate].wPlateID;
					PassRecord.wAntennaID = m_vectDbfCombiner[nCombBoard].wCombinerId;
					hr = m_oDMgr.WritePassSchedule( PassRecord, szCombinerIP, bTestSun );
				}
				else if ( ulActual > 0 )
				{
					double dLEOelevation = 5.0;
					double dMEOelevation = 30.0;
					double dSATelevationMin = 5.0;
					double dSATelevationMax = 65.0;
					bool bPassOK = false;

					std::sort(aAzEls, aAzEls + ulActual, &elevation_sorter);

					for ( ULONG j = 0; j < ulActual; j++ )
					{
						memset( &PassRecord, 0, sizeof(EMSDBFPASSRECORD2) );
					
						PassRecord.ulLutID	= m_ulLutId;
						
						int nSatType = oPA.GetConstellationType( aAzEls[j].ulSatID );

						if ( ( aAzEls[j].ulSatID < 300 ) && ( ulActual == 1 ) )
						{
							PassRecord.wProcessID = 1; // Perform Frequency Domain method
						}
						else
						{
							PassRecord.wProcessID = 2; // Perform Time Domain method
						}

						bPassOK = false;
						// Check satellite elevation
						if ( ( aAzEls[j].degSatElevation > dSATelevationMin )
							  && ( aAzEls[j].degSatElevation < dSATelevationMax ) ) // TSI in-house DBF only (blockage)
						{
							if ( aAzEls[j].ulSatID < 200 )
							{
								if( aAzEls[j].degPlateElevation > dLEOelevation ) bPassOK = true;
							}
							else if( aAzEls[j].ulSatID > 400 )
							{
							   if ( aAzEls[j].degPlateElevation > dMEOelevation ) bPassOK = true;
							}
						}

						if ( bPassOK )
						{
							PassRecord.timestamp  = timeNow;
							//PassRecord.wAntennaID      = m_vectPlates[nPlate].wPlateID;
							PassRecord.wAntennaID	   = m_vectDbfCombiner[nCombBoard].wCombinerId;
							PassRecord.ulSatID         = aAzEls[j].ulSatID;
							PassRecord.ulPassID        = aAzEls[j].ulPassID;

							for ( ULONG s = 0; s < ulVectAllSats.size(); s++ )
							{
								if ( ulVectAllSats[s].ulSatId == aAzEls[j].ulSatID )
								{
									PassRecord.fDownFrequency = ulVectAllSats[s].fFrequency;
									PassRecord.wPolarization  = ( ulVectAllSats[s].szPolarisation == "RHCP" ) ? 1 : 0;
									break;
								}
							}

							PassRecord.fAzimuth        = (float) aAzEls[j].degSatAzimuth;
							PassRecord.fElevation      = (float) aAzEls[j].degSatElevation;
							PassRecord.fPlateAzimuth   = (float) aAzEls[j].degPlateAzimuth;
							PassRecord.fPlateElevation = (float) aAzEls[j].degPlateElevation;
							PassRecord.wPlateID		   = m_wPlateId;
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
								sitePosition = oCoord.ToEarthFixed( m_siteLocation.locnAntenna );
							    
								bool bEarthFixed = true;
								hr = oPA.GetSatelliteVectorEF( timeNow,  PassRecord.ulSatID, &timeCoord );
								
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

									oPA.GetConstellation( PassRecord.ulSatID, &dWaveLength, &dConstellationFrequency );
			
									fDownFrequency = dConstellationFrequency + (double)(PassRecord.fRefTxFrequency -50000.0 + fUpDoppler); 
									
									fDownDoppler = -(1.0 - sqrt( (1.0-dV)/(1.0+dV) )) * fDownFrequency;
						
									PassRecord.fFOA = PassRecord.fRefTxFrequency + fDownDoppler + fUpDoppler;

								}
							}
							hr = m_oDMgr.WritePassSchedule( PassRecord, szCombinerIP, bTestSun );
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

		hr = m_oDMgr.ReadRefBeacons();

		if(hr == EMS_OK)
		{
			EMSDBFPASSSCHEDULECFG oPassCfg = m_oDBFPassSchedCfg.GetDbfPassScheduleCfg();

			m_ulLutId = oPassCfg.ulLutId;
			m_ulPlates = oPassCfg.vectDbfPlate.size();


			for(ULONG i = 0; i < m_ulPlates; i++)  //Now consider one plate only
			{
				memset(	&m_siteLocation, 0, sizeof(EMSDBFPASITELOCATIONCFG));
				m_siteLocation.ulLutId = m_ulLutId;
				memcpy(&m_siteLocation.locnAntenna, &oPassCfg.vectDbfPlate[i].plateLocation, sizeof(EMSLOCATION));


				m_oDBFPassSchedCfg.ReadDBFPlateConfigFromXML(oPassCfg.vectDbfPlate[i].szPlateCfgFile);

				m_vectDbfCombiner = oPassCfg.vectDbfPlate[i].vectDbfCombiner;

				m_vectPlates = m_oDBFPassSchedCfg.GetDBFPlatesCfg();
				if ( i >= m_vectPlates.size() ) continue;
				WORD wCellNumber = m_vectPlates[i].wCellNumber;

				m_oDMgr.Initialize( m_vectDbfCombiner ,  wCellNumber);
			}

			m_vectLeoSats.clear();
			m_vectGalileoSats.clear();
			m_vectBeidouSats.clear();

			int nLeoSatCount = oPassCfg.vectLeoSats.size();
			int nGaliLeoCount = oPassCfg.vectGalileoSats.size();
			int nBeidouSatCount = oPassCfg.vectBeidouSats.size();

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

	try
	{
		xercesc::XMLPlatformUtils::Initialize();

		CEMSDBFSchedulerService oDBFPassSchedService;

		if( argc > 1 &&
			0 == stricmp( argv[1], "-debug" ) )
		{
			oDBFPassSchedService.RunAsService( FALSE );	// for DEBUGGING
		}

		oDBFPassSchedService.AllowPause( FALSE );
		oDBFPassSchedService.Run( oDBFPassSchedService.GetService(), argc, argv );

		xercesc::XMLPlatformUtils::Terminate();
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