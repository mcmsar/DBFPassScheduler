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

#ifndef __EMS_DBF_PASSSCHEDULER_SERVICE_H__
#define __EMS_DBF_PASSSCHEDULER_SERVICE_H__

#include "eservice.h"  // for CEMSService class
#include "emsclock.h"
#include <string>
#include "emsDBFDataMgr2.h"
#include <vector>
#include "DBFPointingAngles.h"
#include "emscoord.h" 
#include "emsrange.h"
#include <initguid.h>

class CEMSDBFSchedulerService : public CEMSService
{
	public:
		CEMSDBFSchedulerService();
		virtual ~CEMSDBFSchedulerService();

		
		const TCHAR* GetService() { return TEXT("TSIDBFPassScheduler"); }
		const wchar_t* GetComponent() { return L"TSIDBFPassScheduler"; }
		static const wchar_t* GetServiceName() { return L"TSIDBFPassScheduler"; }

	protected:
		virtual EMS_RESULT StartEMSService( DWORD dwArgc, LPTSTR *lpszArgv );
		virtual EMS_RESULT StopEMSService( void );
		virtual EMS_RESULT PauseEMSService( void );
		virtual EMS_RESULT ContinueEMSService( void );
		virtual BOOL StopIsAccepted( void );

	private: // methods
		EMS_RESULT _ServiceProcessLoop(void);
		void _ReleaseObjects( void );
		BOOL _OkayToStop( void );
		EMS_RESULT _Init();

		void _DoIdle();
		bool _TimeToGeneratePassSched();
		void _GenerateNewPassSched( EMSTIME startTime, EMSTIME endTime );
		std::vector<EMSDBFPASATCFG>& _GetSatVectorForConstellation( const std::string& szConstellationName );

	protected:


	private: //data
		HANDLE					m_hEventStop;
		HANDLE					m_hEventSignal;
		BOOL					m_bPaused;
		DWORD					m_dwRegister;
		DWORD					m_dwThreadID;
		ULONG					m_ulMinObjCount;
		BOOL					m_bStopEnabled;

		double                  m_dTimeSpan;

		EMSTIME					m_LastGenTime;
//		EMSTIME					m_TimeEnd;
		CEMSSystemClock         m_oSysTime;

		ULONG				m_ulLutId;
		ULONG				m_ulPassId;
		WORD				m_wPlateId;
		ULONG				m_ulPlates;
		std::vector<EMSDBFARRAY>	m_vectPlates;
		std::vector<EMSDBFCOMBINERCFG> m_vectDbfCombiner;

		std::vector<EMSDBFPASATCFG> m_vectLeoSats;
		std::vector<EMSDBFPASATCFG> m_vectGalileoSats;
		std::vector<EMSDBFPASATCFG> m_vectBeidouSats;
		std::vector<EMSDBFPASATCFG> m_vectGlonassSats;

		EMSDBFPASITELOCATIONCFG	m_siteLocation;

		CEMSDBFPassScheduleCfg	m_oDBFPassSchedCfg;
		CEMSDBFDataMgr			m_oDMgr;
};

#endif // __EMS_DBF_PASSSCHEDULER_SERVICE_H__

