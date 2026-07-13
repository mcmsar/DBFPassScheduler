#include <windows.h>
#include <io.h>
#include <string.h>
#include <string>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>

#include "DBFPSConfig.h"

namespace xercesc = XERCES_CPP_NAMESPACE;

static const char c_szConfigFileName[] = "DBFPSConfig.xml";

// ---------------------------------------------------------------------------
// DOM helper: returns the text content of the first element in the document
// with the given tag name, or "" if not found.
// ---------------------------------------------------------------------------

static std::string _GetElementText(xercesc::DOMDocument* doc, const char* tagName)
{
    XMLCh* tag = xercesc::XMLString::transcode(tagName);
    xercesc::DOMNodeList* list = doc->getElementsByTagName(tag);
    xercesc::XMLString::release(&tag);

    if (!list || list->getLength() == 0)
        return "";

    xercesc::DOMNode* child = list->item(0)->getFirstChild();
    while (child)
    {
        if (child->getNodeType() == xercesc::DOMNode::TEXT_NODE)
        {
            char* text = xercesc::XMLString::transcode(child->getNodeValue());
            std::string result(text);
            xercesc::XMLString::release(&text);
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
static bool _GoUpOneLevel(char* dir)
{
    int len = (int)strlen(dir);
    if (len <= 3) return false;       // root like "C:\"
    dir[len - 1] = '\0';             // strip trailing backslash
    char* p = strrchr(dir, '\\');
    if (!p) return false;
    *(p + 1) = '\0';                 // re-add backslash, drop last component
    return true;
}

// ---------------------------------------------------------------------------

CDBFPSConfig::CDBFPSConfig()
    : m_bLoaded(false)
    , m_passScheduleFile("C:\\Program Files (x86)\\EMS Technologies\\DBFPSConfig\\DBFPassSchedule.xml")
    , m_orbitDataFile("C:\\HGT\\Orbit\\sarsat.txt")
    , m_satNoradFile("C:\\HGT\\Orbit\\SatNorad.txt")
    , m_satTLEDir("C:\\HGT\\Orbit\\")
    , m_refBeaconFile("C:\\HGT\\Config\\DBF_ReferenceBeacons.csv")
    , m_traceLogFile("C:\\HGT\\PassScheduleTrace.csv")
    , m_sunAzElFile("C:\\SunAzEl.csv")
    , m_phaseOutputDir("C:\\HGT\\DBFBufferPhase\\")
    , m_nullPointingFile("C:\\NullFile_1.csv")
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
    if (m_bLoaded) return EMS_OK;

    char path[MAX_PATH] = { 0 };
    if (!_FindConfigFile(path, MAX_PATH))
        return EMS_NO_FILE;

    return _Parse(path);
}

bool CDBFPSConfig::_TryPath(const char* dir, char* outPath, int maxLen) const
{
    sprintf_s(outPath, maxLen, "%s%s", dir, c_szConfigFileName);
    return (_access(outPath, 0) == 0);
}

bool CDBFPSConfig::_FindConfigFile(char* outPath, int maxLen) const
{
    char exeDir[MAX_PATH] = { 0 };
    if (!GetModuleFileNameA(NULL, exeDir, MAX_PATH))
        return false;

    char* p = strrchr(exeDir, '\\');
    if (!p) return false;
    *(p + 1) = '\0';                 // keep trailing backslash

    char dir[MAX_PATH];
    strcpy_s(dir, MAX_PATH, exeDir);

    for (int level = 0; level <= 3; level++)
    {
        if (_TryPath(dir, outPath, maxLen))
            return true;

        char cfgSubdir[MAX_PATH];
        sprintf_s(cfgSubdir, MAX_PATH, "%sConfig\\", dir);
        if (_TryPath(cfgSubdir, outPath, maxLen))
            return true;

        char dbfpsCfgSubdir[MAX_PATH];
        sprintf_s(dbfpsCfgSubdir, MAX_PATH, "%sDBFPSConfig\\", dir);
        if (_TryPath(dbfpsCfgSubdir, outPath, maxLen))
            return true;

        if (!_GoUpOneLevel(dir))
            break;
    }

    return false;
}

EMS_RESULT CDBFPSConfig::_Parse(const char* path)
{
    // main() owns the Xerces lifecycle via Initialize()/Terminate(); do not nest them here.
    EMS_RESULT hr = EMS_OK;

    try
    {
        xercesc::XercesDOMParser parser;
        parser.setValidationScheme(xercesc::XercesDOMParser::Val_Never);
        parser.setDoNamespaces(false);
        parser.parse(path);

        xercesc::DOMDocument* doc = parser.getDocument();
        if (!doc)
        {
            hr = EMS_EXCEPTION;
        }
        else
        {
            std::string val;

            val = _GetElementText(doc, "PassScheduleFile");
            if (!val.empty()) m_passScheduleFile = val;

            val = _GetElementText(doc, "OrbitDataFile");
            if (!val.empty()) m_orbitDataFile = val;

            val = _GetElementText(doc, "SatNoradFile");
            if (!val.empty()) m_satNoradFile = val;

            val = _GetElementText(doc, "SatTLEDir");
            if (!val.empty()) m_satTLEDir = val;

            val = _GetElementText(doc, "ReferenceBeaconFile");
            if (!val.empty()) m_refBeaconFile = val;

            val = _GetElementText(doc, "TraceLogFile");
            if (!val.empty()) m_traceLogFile = val;

            val = _GetElementText(doc, "SunAzElFile");
            if (!val.empty()) m_sunAzElFile = val;

            val = _GetElementText(doc, "PhaseOutputDir");
            if (!val.empty()) m_phaseOutputDir = val;

            val = _GetElementText(doc, "NullPointingFile");
            if (!val.empty()) m_nullPointingFile = val;

            m_bLoaded = true;
        }
    }
    catch (...)
    {
        hr = EMS_EXCEPTION;
    }

    return hr;
}
