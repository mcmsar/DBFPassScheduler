/********************************************************************
*   Module:         DBFOrbitCalc.h
*   Description:    Standalone orbit calculation class, providing the
*                   same orbit prediction functionality previously
*                   supplied by the Orbit COM DLL (CEMSOrbitObject /
*                   IEMSOrbit).  No COM dependencies.
*
*   NORAD → satellite-ID mapping is read from SatNorad.txt (same
*   directory as sarsat.txt).  Format: first line is "Count N", then
*   one "SatelliteID,NoradID" pair per line.
*
*   Orbit source files are compiled in from the OrbitCalc sub-directory.
*
*********************************************************************
*   Copyright (c) 2006 by EMS Technologies, Inc.  All rights reserved.
*   Unpublished – contains trade secrets of EMS Technologies, Inc.
********************************************************************/

#ifndef INC_DBFORBITCALC
#define INC_DBFORBITCALC

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <math.h>

#include "emstypes.h"
#include "emserror.h"
#include "emstime.h"
#include "emsorbit.h"   // defines EMSTLEDATA, EMSTLEDATA2, EMSTIMECOORD …
#include "EMSMUTEX.H"

#define DBF_MAX_EMS_ORBIT_DATA  (128)
#define DBF_MAX_SAT_NAME_LENGTH (32)

typedef struct _tagDBFORBITDATA
{
    int     nSatelliteID;
    int     nNoradNumber;
    EMSTLEDATA2 emsTLE;
    TCHAR   szName[ DBF_MAX_SAT_NAME_LENGTH ];
    BOOL    bInUse;
    int     nElementNumber;
} DBFORBITDATA, FAR *LPDBFORBITDATA;


class CDBFOrbitCalc
{
public:
    CDBFOrbitCalc();
    ~CDBFOrbitCalc();

    EMS_RESULT Init();

    EMS_RESULT GetSatelliteTLE ( ULONG ulSatelliteID,
                                 const EMSTIME eTime,
                                 EMSTLEDATA *lpTLEData );

    EMS_RESULT GetOrbitNumber  ( ULONG ulSatelliteID,
                                 const EMSTIME eTime,
                                 ULONG *lpulOrbitNumber );

    EMS_RESULT InertialOrbitTLE( EMSTLEDATA *lpTLE,
                                 const EMSTIME *pvTime,
                                 ULONG cbCount,
                                 EMSTIMECOORD *pvCoord,
                                 ULONG *pcbResult );

    EMS_RESULT EarthFixedOrbitTLE( EMSTLEDATA *lpTLE,
                                   const EMSTIME *pvTime,
                                   ULONG cbCount,
                                   EMSTIMECOORD *pvCoord,
                                   ULONG *pcbResult );

private:
    BOOL          m_bInitialized;
    DBFORBITDATA *m_lpOrbitData;        // heap-allocated array
    CEMSMutex     m_mutex;
    LPTSTR        m_lpszFilespec;       // full path to sarsat.txt
    LPTSTR        m_lpszPath;           // directory containing sarsat.txt

    // NORAD-number → satellite-ID table loaded from SatNorad.txt
    int           m_anNoradID[ DBF_MAX_EMS_ORBIT_DATA ];
    int           m_anSatID  [ DBF_MAX_EMS_ORBIT_DATA ];
    int           m_nNoradMapCount;

    int          _FindOrbitData ( int nID, BOOL bUseNorad = FALSE );
    BOOL         _GetOrbitData  ( int nID, LPDBFORBITDATA lpOrbitData,
                                  BOOL bUseNorad = FALSE );
    int          _FindEmptySlot ();
    EMS_RESULT   _InitData      ();
    EMS_RESULT   _InitDataFromFile ( LPCTSTR lpcszFilename );
    EMS_RESULT   _LoadNoradMap  ();
    int          _SatIDFromNoradNumber ( int nNoradNumber );
    int          _RemoveTrailingUnprintables ( LPTSTR lpszText );
    EMSTIME      _ConvertEpochTime ( double dEpochTime );
    EMS_RESULT   _AllocateOrbitData ();
    void         _DeallocateOrbitData ();
    EMS_RESULT   _ReadTLEFromFile ( ULONG ulSatID, LPDBFORBITDATA lpData );
    EMS_RESULT   _SetPath ();
    BOOL         _ValidName ( LPCTSTR lpcszSatelliteName );
    TCHAR        _CalcChecksum ( LPCTSTR lpcszBuffer );
};

#endif // INC_DBFORBITCALC
