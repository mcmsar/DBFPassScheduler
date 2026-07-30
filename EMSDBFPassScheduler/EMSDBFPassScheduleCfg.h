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