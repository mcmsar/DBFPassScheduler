#include <Windows.h>
#include <stdlib.h>
#include <cstdio>
#include <vector>
#include <sstream>
#include "emsclock.h"
//#include "CSVReader.h"
//#include "emsdbftypes.h"
#include <initguid.h>

#include "DBFPointingAngles.h"
typedef struct _tagEMSDBFBUFFER
{
	int iDataSetType;
	EMSTIME timeStamp;
	ULONG ulSatID;
	float fPlateAzimuth;
	float fPlateElevation;
	ULONG ulMaxFreqIndex;
	double dMaxPowerlevel;
	double PhaseArray[32];
 } 	EMSDBFBUFFER;

class CDBFPhaseHandling
{
public:
	CDBFPhaseHandling(){}
	~CDBFPhaseHandling(){}

	std::vector<std::string> get_all_files_names_within_folder(std::string folder);
	bool FileExists(const std::string& filename);
	std::string FindFileForCurrentHour(std::string szFolder);
	bool ReadDBFBufferPhaseFromCsv(std::string &inFileName);
	bool MoveProcessedFile(std::string szProcessedFile, std::string szBkUpFile);
	void _OutputTOCSV( );
	//void  GetDBFPhaseBuffer(std::vector<EMSDBFBUFFER>& vectDbfBuffer) const {vectDbfBuffer = m_vectDbfBuffer;}

	//function generatebiasoffset use vectbuff to 
	BOOL GenerateBiasOffset( /*const*/ CDBFPointingAngles& opa );

	const float* GetBiasOffset() const
	{
		return m_BiasOffset;
	}
private:
	 std::string _NumberToString ( int nNumber);
	 INT64 _ConvertStringToInt64( const char* cszValue );
	 int _ConvertStringToInt( const char* cszValue );
 
private:
	CEMSSystemClock         m_oSysTime;
	std::vector<EMSDBFBUFFER> m_vectDbfBuffer;
	float m_BiasOffset[32];
	float m_PhaseStdDev[32];

};