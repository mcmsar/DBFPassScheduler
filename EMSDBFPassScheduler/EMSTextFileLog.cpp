/*********************************************************************
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

	$Log:
	$
********************************************************************/


#ifdef __EMS_USING_MFC__
#include <afxdisp.h>
#else
#include <windows.h>
#endif

#include "EMSTextFileLog.h"



CEMSTextFile::CEMSTextFile(): m_lpFile(NULL)
{
}

CEMSTextFile::~CEMSTextFile()
{
	Close();
}

bool 
CEMSTextFile::OpenTextFile( const char* szFileName, const char* szMode )
{
	bool bRet = false;

	if( szFileName && strlen( szFileName ) > 0 )
	{
		m_szFileName = szFileName;
		if( m_lpFile )
		{
			fclose( (FILE*)m_lpFile );
			m_lpFile = 0;
		}

		m_lpFile = fopen( szFileName, szMode );
			

		if( m_lpFile )
		{
			bRet = true;
		}
	}

	return bRet;
}

void
CEMSTextFile::Close()
{
	if ( m_lpFile )
	{
		fclose( (FILE*)m_lpFile );
		m_lpFile = 0;
	}
}

bool
CEMSTextFile::WriteText( std::string szText )
{
	bool bRet = false;
	EMSTIMEFIELDS timeFields;
	CEMSSystemClock sysClock;
	char timeArr[256];
	memset(timeArr, 0, 256);

	if( !m_lpFile )
	{
		std::string szFile = m_szFileName;
		OpenTextFile( szFile.c_str(), "a+" );
	}

	CEMSTime timeNow( sysClock.GetTime() );
	timeNow.GetTime( &timeFields );
	sprintf(timeArr, "%d/%d/%d-%02d:%02d:%02d:%06d", timeFields.nYear, timeFields.nMonth, timeFields.nDay,
													timeFields.nHour, timeFields.nMinute, timeFields.nSecond,
													(ULONG) ((double)timeFields.lNanosecond *1e-6));

	if( m_lpFile )
	{

		fprintf(m_lpFile, "%s %s\n", timeArr, szText.c_str());
		fflush(m_lpFile);
		bRet = true;
	}
	
	return bRet;
}
