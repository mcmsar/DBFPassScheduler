#include <windows.h>
#include "emsdbftypes2.h"
#include "emserror.h"
#include "dbfConfig.hxx"
#include "dbfPlateConfig.hxx"
#include <vector>
#include <string>

typedef struct _tagEMSDBFSATCFG
{
	std::string szType;
	double fFrequency;
	std::string szPolarisation;
	ULONG ulSatId;
	ULONG ulPriority;

}EMSDBFSATCFG;

typedef struct _tagEMSDBFCOMBINERCFG
{
	WORD wCombinerId;
	std::string szConstellation;
	std::string szIpAddress;
}EMSDBFCOMBINERCFG;


typedef struct EMSDBFPLATELOCATION
{
	EMSDEGREES degLatitude;
	EMSDEGREES degLongitude;
	float fAltitude;
}EMSDBFPLATELOCATION;

typedef struct _tagEMSDBFPLATECFG
{
	WORD wPlateId; 
	std::string szPlateCfgFile;
	float fPlateMinElevation;
	// Azimuth acceptance window at the plate, degrees north. Defaults to 0/360 --
	// the full circle -- when the XML omits the limits, so an unmodified config
	// keeps the pre-existing behaviour of no azimuth gating.
	float fPlateMinAzimuth;
	float fPlateMaxAzimuth;
	// Thresholds a satellite must clear to actually be scheduled, as opposed to
	// fPlateMinElevation above, which only decides what enters the pipeline. The
	// satellite elevation pair bounds the angle above the local horizon; the plate
	// pair are floors on the angle off the plate boresight, picked per constellation
	// class. Defaults match the values these used to have as compile-time constants.
	// The satellite elevation pair is also split per constellation class; each of
	// those falls back to the shared fSatMin/MaxElevation when not configured.
	float fSatMinElevation;
	float fSatMaxElevation;
	float fSatMinElevationLEO;
	float fSatMaxElevationLEO;
	float fSatMinElevationMEO;
	float fSatMaxElevationMEO;
	float fPlateMinElevationLEO;
	float fPlateMinElevationMEO;
	float fPlateMinSeparationAngle;
	EMSDBFPLATELOCATION plateLocation;
	std::vector<EMSDBFCOMBINERCFG> vectDbfCombiner; 
}EMSDBFPLATECFG;

typedef struct _tagEMSDBFPASSSCHEDULECFG
{
	ULONG ulLutId;
	std::vector<EMSDBFSATCFG> vectLeoSats;
	std::vector<EMSDBFSATCFG> vectGalileoSats;
	std::vector<EMSDBFSATCFG> vectBeidouSats;
	std::vector<EMSDBFSATCFG> vectGlonassSats;
	std::vector<EMSDBFPLATECFG> vectDbfPlate;
 } 	EMSDBFPASSSCHEDULECFG;

class CEMSDBFPassScheduleCfg
{
public:
	CEMSDBFPassScheduleCfg();
	~CEMSDBFPassScheduleCfg();

	EMS_RESULT ReadDBFConfigFromXML(const std::string szInFile);
	EMSDBFPASSSCHEDULECFG& GetDbfPassScheduleCfg(){return m_dbfPassScheduleCfg;}

	EMS_RESULT ReadDBFPlateConfigFromXML(const std::string szDBFPlateCfgFile);
	std::vector<EMSDBFARRAY>& GetDBFPlatesCfg(){return m_aDBFplate;}

private:
	WORD _convertStringToWord(std::string str);

private:
	EMSDBFPASSSCHEDULECFG m_dbfPassScheduleCfg;
	std::vector<EMSDBFARRAY>	m_aDBFplate;	

};