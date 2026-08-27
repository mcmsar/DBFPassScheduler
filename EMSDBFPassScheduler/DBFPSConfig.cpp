#include <windows.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <string>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>

#include "DBFPSConfig.h"

// For DBF_MAX_SATELLITES, the compile-time capacity <MaxSatellites> is clamped to.
#include "emsDBFtypes2.h"

namespace xercesc = XERCES_CPP_NAMESPACE;

static const char c_szConfigFileName[] = "DBFPSConfig.xml";

// ---------------------------------------------------------------------------
// DOM helper: returns the text content of the first element in the document
// with the given tag name, or "" if not found.
// ---------------------------------------------------------------------------

static std::string _GetElementText( xercesc::DOMDocument* doc, const char* tagName )
{
    XMLCh* tag = xercesc::XMLString::transcode( tagName );
    xercesc::DOMNodeList* list = doc->getElementsByTagName( tag );
    xercesc::XMLString::release( &tag );

    if ( !list || list->getLength() == 0 )
        return "";

    xercesc::DOMNode* child = list->item( 0 )->getFirstChild();
    while ( child )
    {
        if ( child->getNodeType() == xercesc::DOMNode::TEXT_NODE )
        {
            char* text = xercesc::XMLString::transcode( child->getNodeValue() );
            std::string result( text );
            xercesc::XMLString::release( &text );
            return result;
        }
        child = child->getNextSibling();
    }
    return "";
}

// ---------------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------------

// Advances dir one level toward the root (dir must end with '\').
// Returns false when already at root (e.g. "C:\").
static bool _GoUpOneLevel( char* dir )
{
    int len = (int)strlen( dir );
    if ( len <= 3 ) return false;       // root like "C:\"
    dir[ len - 1 ] = '\0';             // strip trailing backslash
    char* p = strrchr( dir, '\\' );
    if ( !p ) return false;
    *( p + 1 ) = '\0';                 // re-add backslash, drop last component
    return true;
}

// ---------------------------------------------------------------------------

CDBFPSConfig::CDBFPSConfig()
    : m_bLoaded( false )
    , m_passScheduleFile( "C:\\Program Files (x86)\\EMS Technologies\\DBFPSConfig\\DBFPassSchedule.xml" )
    , m_orbitDataFile( "C:\\HGT\\Orbit\\sarsat.txt" )
    , m_satNoradFile( "C:\\HGT\\Orbit\\SatNorad.txt" )
    , m_satTLEDir( "C:\\HGT\\Orbit\\" )
    , m_refBeaconFile( "C:\\HGT\\Config\\DBF_ReferenceBeacons.csv" )
    , m_traceLogFile( "C:\\HGT\\PassScheduleTrace.csv" )
    , m_sunAzElFile( "C:\\SunAzEl.csv" )
    , m_phaseOutputDir( "C:\\HGT\\DBFBufferPhase\\" )
    , m_nullPointingFile( "C:\\NullFile_1.csv" )
    // Default to the full compiled capacity: the behaviour before <MaxSatellites>
    // existed, minus the records the collector was discarding anyway.
    , m_ulMaxSatellites( (unsigned long) DBF_MAX_SATELLITES )
    // A week of trace output. The scheduler appends a block every generation cycle
    // and never overwrites, so without a retention limit the file grows forever.
    , m_ulTraceRetentionDays( 7 )
    , m_tLastTracePrune( 0 )
{
}

CDBFPSConfig& CDBFPSConfig::Instance()
{
    static CDBFPSConfig instance;
    return instance;
}

// ---------------------------------------------------------------------------

EMS_RESULT CDBFPSConfig::Load()
{
    if ( m_bLoaded ) return EMS_OK;

    char path[MAX_PATH] = { 0 };
    if ( !_FindConfigFile( path, MAX_PATH ) )
        return EMS_NO_FILE;

    return _Parse( path );
}

bool CDBFPSConfig::_TryPath( const char* dir, char* outPath, int maxLen ) const
{
    sprintf_s( outPath, maxLen, "%s%s", dir, c_szConfigFileName );
    return ( _access( outPath, 0 ) == 0 );
}

bool CDBFPSConfig::_FindConfigFile( char* outPath, int maxLen ) const
{
    char exeDir[MAX_PATH] = { 0 };
    if ( !GetModuleFileNameA( NULL, exeDir, MAX_PATH ) )
        return false;

    char* p = strrchr( exeDir, '\\' );
    if ( !p ) return false;
    *( p + 1 ) = '\0';                 // keep trailing backslash

    char dir[MAX_PATH];
    strcpy_s( dir, MAX_PATH, exeDir );

    for ( int level = 0; level <= 4; level++ )
    {
        if ( _TryPath( dir, outPath, maxLen ) )
            return true;

        char cfgSubdir[MAX_PATH];
        sprintf_s( cfgSubdir, MAX_PATH, "%sConfig\\", dir );
        if ( _TryPath( cfgSubdir, outPath, maxLen ) )
            return true;

        char dbfpsCfgSubdir[MAX_PATH];
        sprintf_s( dbfpsCfgSubdir, MAX_PATH, "%sDBFPSConfig\\", dir );
        if ( _TryPath( dbfpsCfgSubdir, outPath, maxLen ) )
            return true;

        if ( !_GoUpOneLevel( dir ) )
            break;
    }

    return false;
}

EMS_RESULT CDBFPSConfig::_Parse( const char* path )
{
    // main() owns the Xerces lifecycle via Initialize()/Terminate(); do not nest them here.
    EMS_RESULT hr = EMS_OK;

    try
    {
        xercesc::XercesDOMParser parser;
        parser.setValidationScheme( xercesc::XercesDOMParser::Val_Never );
        parser.setDoNamespaces( false );
        parser.parse( path );

        xercesc::DOMDocument* doc = parser.getDocument();
        if ( !doc )
        {
            hr = EMS_EXCEPTION;
        }
        else
        {
            std::string val;

            val = _GetElementText( doc, "PassScheduleFile" );
            if ( !val.empty() ) m_passScheduleFile = val;

            val = _GetElementText( doc, "OrbitDataFile" );
            if ( !val.empty() ) m_orbitDataFile = val;

            val = _GetElementText( doc, "SatNoradFile" );
            if ( !val.empty() ) m_satNoradFile = val;

            val = _GetElementText( doc, "SatTLEDir" );
            if ( !val.empty() ) m_satTLEDir = val;

            val = _GetElementText( doc, "ReferenceBeaconFile" );
            if ( !val.empty() ) m_refBeaconFile = val;

            val = _GetElementText( doc, "TraceLogFile" );
            if ( !val.empty() ) m_traceLogFile = val;

            val = _GetElementText( doc, "SunAzElFile" );
            if ( !val.empty() ) m_sunAzElFile = val;

            val = _GetElementText( doc, "PhaseOutputDir" );
            if ( !val.empty() ) m_phaseOutputDir = val;

            val = _GetElementText( doc, "NullPointingFile" );
            if ( !val.empty() ) m_nullPointingFile = val;

            // Zero is meaningful here -- it disables trimming -- so a negative or
            // unparseable value is what falls back to the default.
            val = _GetElementText( doc, "TraceRetentionDays" );
            if ( !val.empty() )
            {
                long n = atol( val.c_str() );
                if ( n >= 0 ) m_ulTraceRetentionDays = (unsigned long) n;
            }

            // Clamped rather than trusted: this caps how many pass records are
            // written per time step, and the collector cannot store more than
            // DBF_MAX_SATELLITES of them per second regardless of what is asked for.
            val = _GetElementText( doc, "MaxSatellites" );
            if ( !val.empty() )
            {
                long n = atol( val.c_str() );
                if ( n < 1 )
                    n = 1;
                if ( n > (long) DBF_MAX_SATELLITES )
                    n = (long) DBF_MAX_SATELLITES;
                m_ulMaxSatellites = (unsigned long) n;
            }

            m_bLoaded = true;
        }
    }
    catch ( ... )
    {
        hr = EMS_EXCEPTION;
    }

    return hr;
}


// ---------------------------------------------------------------------------
// Trace log retention
// ---------------------------------------------------------------------------

// The trace is a run of undated lines punctuated by one dated marker per
// generation cycle, written by CEMSDBFSchedulerService::_RunSchedule:
//
//     Generating new pass schedule : 2026/ 8/26 14:03: 7
//
// That marker is the only thing in the file carrying a date, so trimming works in
// whole blocks: everything from one marker up to the next belongs to that cycle and
// is kept or dropped together. Undated lines written before the very first marker
// (startup banners, reference beacons) age out with the block they precede.
static const char c_szTraceAnchor[] = "Generating new pass schedule :";

// Extracts the date from a marker line. Returns false for any other line.
static bool _ParseTraceAnchorDate( const char* szLine, time_t* ptOut )
{
    const char* p = strstr( szLine, c_szTraceAnchor );
    if ( !p ) return false;
    p += sizeof(c_szTraceAnchor) - 1;

    int y = 0, mo = 0, d = 0, h = 0, mi = 0, sec = 0;
    if ( 6 != sscanf( p, "%d/%d/%d %d:%d:%d", &y, &mo, &d, &h, &mi, &sec ) ) return false;

    struct tm tmLine;
    memset( &tmLine, 0, sizeof(tmLine) );
    tmLine.tm_year  = y - 1900;
    tmLine.tm_mon   = mo - 1;
    tmLine.tm_mday  = d;
    tmLine.tm_hour  = h;
    tmLine.tm_min   = mi;
    tmLine.tm_sec   = sec;
    tmLine.tm_isdst = -1;           // let the CRT work out DST for that date

    time_t t = mktime( &tmLine );
    if ( t == (time_t)-1 ) return false;

    *ptOut = t;
    return true;
}

void CDBFPSConfig::_PruneTraceLog()
{
    if ( m_ulTraceRetentionDays == 0 ) return;      // retention disabled
    if ( m_traceLogFile.empty() ) return;

    time_t tNow = time( NULL );

    // Trimming rewrites the whole file, so do it once at startup and then hourly
    // rather than on every trace write.
    if ( ( m_tLastTracePrune != 0 ) && ( difftime( tNow, m_tLastTracePrune ) < 3600.0 ) ) return;
    m_tLastTracePrune = tNow;

    // Binary throughout: the offsets below are byte offsets, and the copy must
    // reproduce the existing CRLF line endings rather than translate them twice.
    FILE* lpIn = fopen( m_traceLogFile.c_str(), "rb" );
    if ( !lpIn ) return;

    const time_t tCutoff = tNow - (time_t)( m_ulTraceRetentionDays * 24 * 60 * 60 );

    long lKeepFrom   = -1;      // first block at or after the cutoff
    long lLastAnchor = -1;      // newest block, whatever its date
    long lLineStart  = 0;
    char szLine[ 1024 ];

    while ( fgets( szLine, sizeof(szLine), lpIn ) )
    {
        time_t tLine;
        if ( _ParseTraceAnchorDate( szLine, &tLine ) )
        {
            lLastAnchor = lLineStart;
            if ( ( lKeepFrom < 0 ) && ( tLine >= tCutoff ) ) lKeepFrom = lLineStart;
        }
        lLineStart = ftell( lpIn );
    }

    // Nothing inside the window: the scheduler has not run for longer than the
    // retention period. Keep the newest block rather than emptying the file, so the
    // last thing it did is still on record.
    if ( lKeepFrom < 0 ) lKeepFrom = lLastAnchor;

    // lKeepFrom <= 0 covers both "already within the window" and "no dated markers
    // at all", in which case there is nothing safe to trim on.
    if ( lKeepFrom <= 0 )
    {
        fclose( lpIn );
        return;
    }

    const std::string szTmp = m_traceLogFile + ".prune";

    FILE* lpOut = fopen( szTmp.c_str(), "wb" );
    if ( !lpOut )
    {
        fclose( lpIn );
        return;
    }

    bool bOK = ( 0 == fseek( lpIn, lKeepFrom, SEEK_SET ) );

    char buf[ 8192 ];
    size_t nRead;
    while ( bOK && ( nRead = fread( buf, 1, sizeof(buf), lpIn ) ) > 0 )
    {
        if ( fwrite( buf, 1, nRead, lpOut ) != nRead ) bOK = false;
    }

    if ( ferror( lpIn ) || ferror( lpOut ) ) bOK = false;

    fclose( lpOut );
    fclose( lpIn );

    // Replace in one move so a failure leaves the original trace intact rather than
    // deleting it and then discovering the rename will not go through.
    if ( !bOK || !MoveFileExA( szTmp.c_str(), m_traceLogFile.c_str(), MOVEFILE_REPLACE_EXISTING ) )
    {
        remove( szTmp.c_str() );
    }
}

FILE* CDBFPSConfig::OpenTraceLog()
{
    _PruneTraceLog();
    return fopen( m_traceLogFile.c_str(), "at" );
}
