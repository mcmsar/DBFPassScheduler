/********************************************************************
*	Module:			emsDBFDataMgr.h
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

	0.0	06/07/09		rcr		start

********************************************************************/
#ifndef INC_EMSDBFDATAMGR
#define INC_EMSDBFDATAMGR

#include <string>
#include "emstypes.h"
#include "emserror.h"
#include <stdio.h>
#include "emsdbftypes2.h"
#include <vector>
#include "EMSDBFPassScheduleCfg.h"

using std::string;
using std::wstring;

#define MAX_SCHEDULE_RECORDS (26000)
#define MAX_SCHEDULE_RECORDS_HOUR (25200)

class CEMSDBFDataMgr
{
public:

	~CEMSDBFDataMgr( void );
	CEMSDBFDataMgr();

	CEMSDBFDataMgr( const CEMSDBFDataMgr& x ) ;

	void Initialize();
	EMS_RESULT Initialize( std::vector<EMSDBFCOMBINERCFG>& vectDbfCombiner, WORD wCellNumber );

	EMS_RESULT WritePassSchedule( EMSDBFPASSRECORD2 aPass, std::string szCombinerIP, bool bTestSun = false, const std::string& szConstellation = "" );
	void EnsureHourFileOpen( int nHour, const std::string& szCombinerIP );

	EMS_RESULT ReadRefBeacons( );

	BOOL CheckRefBeacon( EMSDBFPASSRECORD2 *PassRecord );

	// Puts the reference-beacon fields back into the "no beacon this second" state:
	// ID and message filled with '0', location and transmit frequency zeroed. That
	// is the representation CheckRefBeacon leaves when no beacon is due, so a record
	// cleared here is indistinguishable from one that never had a beacon.
	static void ClearRefBeacon( EMSDBFPASSRECORD2 *PassRecord );

	void Reset( );


protected:

private: // data
	FILE 					*m_lpPassFileBIN;
	FILE 					*m_lpPassFileCSV;

	int						m_nCurrentHour;
	WORD					m_wCellNumber;

	EMSTIME					m_TimeStart;
	EMSTIME					m_TimeEnd;

	ULONG					m_ulRecordCount;
	ULONG					m_ulLastRecord;

	REFERENCEBEACONS		m_RefBeacons;
	int						m_RefCount;
};


#endif // INC_EMSDBFDATAMGR



