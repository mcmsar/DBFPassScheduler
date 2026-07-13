#pragma warning(disable:4786)

#include <windows.h>
#include <assert.h>

#include <string>
using std::string;

#include "emsexcpt.h"
#include "criticalsection.h"
#include "convutility.h"
#include "processconfigconstants.h"
#include "ccbootstrap.h"

#include "LogHelper.h"

CEMSLogHelper*       CEMSLogHelper::ms_pLogger = NULL;
CEMSCriticalSection  CEMSLogHelper::ms_csLoggerCreation;

CEMSLogHelper::~CEMSLogHelper()
{
	if ( m_pLogger )
	{
		m_pLogger->Release();
		m_pLogger = NULL;
	}
}

void
CEMSLogHelper::Reset()
{
	if ( ms_pLogger )
	{
		ms_csLoggerCreation.Enter();
		try
		{
			if ( ms_pLogger )
			{
				ULONG ulCount = ms_pLogger->Release();
				if ( ulCount == 0 )
				{
					ms_pLogger = NULL;
				}
			}
			ms_csLoggerCreation.Leave();
		}
		catch( ... )
		{
			ms_csLoggerCreation.Leave();
			throw;
		}
	}
	return;
}

void
CEMSLogHelper::Init( const unsigned long culLutID, const WORD cwProcessID )
{
	bool bAddRef = true;
	if ( !ms_pLogger )
	{
		CEMSLogHelper* pLogger = NULL;

		ms_csLoggerCreation.Enter();

		try
		{
			if ( !ms_pLogger )
			{
				pLogger = new CEMSLogHelper;

				if ( !pLogger )
				{
					THROW_NOMEMORY_EXCEPTION();
				}
				pLogger->_Init( culLutID, cwProcessID );
				ms_pLogger = pLogger;
				bAddRef = false;
			}
		}
		catch( ... )
		{
			delete pLogger;
			ms_csLoggerCreation.Leave();
			throw;
		}

		ms_csLoggerCreation.Leave();
	}
	if ( bAddRef )
	{
		ms_pLogger->AddRef();
	}

	return;
}

// COM logger (CLSID_EMSLeo600Logger) is not used in this application.
void
CEMSLogHelper::_Init( const unsigned long culLutID, const WORD cwProcessID )
{
}

// Configuration manager COM server not used; return the caller-supplied LUT ID.
unsigned long
CEMSLogHelper::_GetLutID( const unsigned long culLutID )
{
	return culLutID;
}

void
CEMSLogHelper::LogException( CEMSException& ex, const bool cbAsAlarm )
{
	if( !ex.LogDisabled() )
	{
		if( !ex.HasBeenAlarmed() && cbAsAlarm )
		{
			if( ex.IsWarning() )
			{
				LogWarningMsg( ex.GetErrCode(), ex.GetParam() );
			}
			else
			{
				LogAlarmMsg( ex.GetErrCode(), ex.GetParam() );
			}

			ex.SetAlarmLogged();
		}
		else
		{
			LogInfoMsg( ex.GetErrCode(), ex.GetParam() );
		}

		CEMSLogMsgParam param(5);
		param.AddX( ex.GetErrCode() );
		param.AddX( ex.GetSubErrCode() );
		param.Add( ex.GetFileName() );
		param.Add( ex.GetLineNumber() );
		param.Add( ex.GetComment() );
		LogInfoMsg( EMS_EXCEPTION_TRACE, param );
	}
}

void
CEMSLogHelper::_LogMsg( const EMSLOGMSGTYPE ceType, const DWORD cdwMsg, CEMSLogMsgParam& param )
{
	if ( m_pLogger )
	{
		m_pLogger->Report( ceType, cdwMsg, param.GetCount(), param.GetParams() );
	}
}

void
CEMSLogHelper::_LogMsg( const EMSLOGMSGTYPE ceType, const DWORD cdwMsg, LPCWSTR cwszSingleParam )
{
	if ( m_pLogger )
	{
		if ( cwszSingleParam )
		{
			m_pLogger->Report( ceType, cdwMsg, 1, &cwszSingleParam );
		}
		else
		{
			m_pLogger->Report( ceType, cdwMsg, 0, NULL );
		}
	}
}

void
CEMSLogHelper::LogMsg( const EMSLOGMSGTYPE ceType, const DWORD cdwMsg, CEMSLogMsgParam& param )
{
	if ( ms_pLogger ) ms_pLogger->_LogMsg( ceType, cdwMsg, param );
	_LogDebugTrace( ceType, cdwMsg, param );
}

void
CEMSLogHelper::LogMsg( const EMSLOGMSGTYPE ceType, const DWORD cdwMsg, LPCWSTR wszSingleParam )
{
	if ( ms_pLogger ) ms_pLogger->_LogMsg( ceType, cdwMsg, wszSingleParam );
	_LogDebugTrace( ceType, cdwMsg, wszSingleParam );
}

#ifdef _DEBUG
void
CEMSLogHelper::_LogDebugTrace( const EMSLOGMSGTYPE ceType, DWORD cdwMsg, CEMSLogMsgParam& param )
{
	if ( ms_pLogger ) return;

	wchar_t wszBuff[1024];
	const wchar_t** awszParam = param.GetParams();

	const wchar_t* wszType = NULL;
	switch( ceType )
	{
	case EMSLogMsgAlarm:   wszType = L"Alarm";   break;
	case EMSLogMsgWarn:    wszType = L"Warn";    break;
	case EMSLogMsgTrace:   wszType = L"Trace";   break;
	case EMSLogMsgInfo:    wszType = L"Info";    break;
	case EMSLogMsgDebug:   wszType = L"Debug";   break;
	case EMSLogMsgStatus:  wszType = L"Status";  break;
	case EMSLogMsgFatal:   wszType = L"Fatal";   break;
	case EMSLogMsgStartup: wszType = L"Startup"; break;
	default:               wszType = L"Unknown"; break;
	}

	const int iMaxChars = 1022;

	memset( wszBuff, 0, 1024 * sizeof(wchar_t) );
	swprintf( wszBuff, L"Log %s: 0x%x with params ", wszType, cdwMsg );

	int iLen  = wcslen( wszBuff );
	int iLeft = iMaxChars - iLen;

	for ( int i = 0; i < param.GetCount() && iLeft > 2; i++ )
	{
		wszBuff[iLen++] = '('; iLeft--;
		wszBuff[iLen++] = 0;   iLeft--;
		wcsncat( wszBuff, awszParam[i], iLeft );
		iLen  = wcslen( wszBuff );
		iLeft = iMaxChars - iLen;
		if ( iLeft > 0 )
		{
			wszBuff[iLen++] = ')'; iLeft--;
		}
	}
	int iPos = wcslen( wszBuff );
	wszBuff[iPos++] = '\n';
	wszBuff[iPos]   = 0;

	OutputDebugStringW( wszBuff );
}

void
CEMSLogHelper::_LogDebugTrace( const EMSLOGMSGTYPE ceType, DWORD cdwMsg, LPCWSTR wszSingleParam )
{
	if ( ms_pLogger ) return;

	const wchar_t* wszType = NULL;
	switch( ceType )
	{
	case EMSLogMsgAlarm:   wszType = L"Alarm";   break;
	case EMSLogMsgWarn:    wszType = L"Warn";    break;
	case EMSLogMsgTrace:   wszType = L"Trace";   break;
	case EMSLogMsgInfo:    wszType = L"Info";    break;
	case EMSLogMsgDebug:   wszType = L"Debug";   break;
	case EMSLogMsgStatus:  wszType = L"Status";  break;
	case EMSLogMsgFatal:   wszType = L"Fatal";   break;
	case EMSLogMsgStartup: wszType = L"Startup"; break;
	default:               wszType = L"Unknown"; break;
	}

	wchar_t wszBuff[1024];
	memset( wszBuff, 0, 1024 * sizeof(wchar_t) );
	const int iMaxChars = 1022;

	if ( wszSingleParam )
	{
		swprintf( wszBuff, L"Log %s: 0x%x with param ", wszType, cdwMsg );
		wcsncat( wszBuff, wszSingleParam, iMaxChars - wcslen(wszBuff) );
	}
	else
	{
		swprintf( wszBuff, L"Log %s: 0x%x", wszType, cdwMsg );
	}
	int iPos = wcslen( wszBuff );
	wszBuff[iPos++] = '\n';
	wszBuff[iPos]   = 0;

	OutputDebugStringW( wszBuff );
}
#endif

void
CEMSLogHelper::LogMsg( const EMSLOGMSGTYPE ceType, const DWORD cdwMsg, LPCSTR szSingleParam )
{
	if ( !ms_pLogger ) return;

	wstring wstr;
	if ( szSingleParam )
	{
		wstr = CEMSConversionUtil::ConvertToString( szSingleParam );
	}
	ms_pLogger->_LogMsg( ceType, cdwMsg, wstr.c_str() );
	_LogDebugTrace( ceType, cdwMsg, wstr.c_str() );
}

void
CEMSLogHelper::LogAlarmMsg( const DWORD cdwMsg, const EMS_RESULT cerrResult )
{
	CEMSLogMsgParam param(1);
	param.AddX( cerrResult );
	LogAlarmMsg( cdwMsg, param );
}
