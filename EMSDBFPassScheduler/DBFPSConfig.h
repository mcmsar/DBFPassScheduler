/********************************************************************
*   Module:         DBFPSConfig.h
*   Description:    Application file-path configuration loaded from
*                   DBFPSConfig.xml at startup.  Replaces hard-coded
*                   input/output paths throughout the scheduler.
*
*   Search order for DBFPSConfig.xml (stops at first match or root):
*     <exedir>\
*     <exedir>\Config\
*     <exedir>\..\           (and \..\Config\)
*     <exedir>\..\..\        (and \..\..\Config\)
*     <exedir>\..\..\..\     (and \..\..\..\Config\)
*
*   Built-in defaults match the previously hard-coded paths; if the
*   config file is not found the application behaves as before.
*
*********************************************************************
*   Copyright (c) 2006 by EMS Technologies, Inc.  All rights reserved.
********************************************************************/

#ifndef INC_DBFPSCONFIG
#define INC_DBFPSCONFIG

#include <string>
#include "emserror.h"

class CDBFPSConfig
{
public:
    static CDBFPSConfig& Instance();

    EMS_RESULT Load();
    bool       IsLoaded() const { return m_bLoaded; }

    // Input
    const std::string& PassScheduleFile()    const { return m_passScheduleFile; }
    const std::string& OrbitDataFile()       const { return m_orbitDataFile; }
    const std::string& SatNoradFile()        const { return m_satNoradFile; }
    const std::string& SatTLEDir()           const { return m_satTLEDir; }
    const std::string& ReferenceBeaconFile() const { return m_refBeaconFile; }

    // Output
    const std::string& TraceLogFile()     const { return m_traceLogFile; }
    const std::string& SunAzElFile()      const { return m_sunAzElFile; }
    const std::string& PhaseOutputDir()   const { return m_phaseOutputDir; }
    const std::string& NullPointingFile() const { return m_nullPointingFile; }

private:
    CDBFPSConfig();
    CDBFPSConfig( const CDBFPSConfig& );
    CDBFPSConfig& operator=( const CDBFPSConfig& );

    bool       _FindConfigFile( char* outPath, int maxLen ) const;
    bool       _TryPath( const char* dir, char* outPath, int maxLen ) const;
    EMS_RESULT _Parse( const char* path );

    bool        m_bLoaded;
    std::string m_passScheduleFile;
    std::string m_orbitDataFile;
    std::string m_satNoradFile;
    std::string m_satTLEDir;
    std::string m_refBeaconFile;
    std::string m_traceLogFile;
    std::string m_sunAzElFile;
    std::string m_phaseOutputDir;
    std::string m_nullPointingFile;
};

#endif // INC_DBFPSCONFIG
