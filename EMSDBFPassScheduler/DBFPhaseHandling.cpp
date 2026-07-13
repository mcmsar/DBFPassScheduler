// DBFPhaseHandling.cpp : Defines the entry point for the console application.
//
#include "DBFPhaseHandling.h"
#include "convutility.h"
#include "CSVReader.h"
#include "emsDBFtypes2.h"
#include "EMSOutlier.h"
#include "emsconst.h"
#include <fstream>
#include "DBFPSConfig.h"

std::string
CDBFPhaseHandling::_NumberToString ( int nNumber)
{
    std::ostringstream ss;
    ss << nNumber;
    return ss.str();
}

INT64 
CDBFPhaseHandling::_ConvertStringToInt64( const char* cszValue )
{
	INT64 i64Ret = 0;

	if( cszValue )
	{
		std::stringstream strm( cszValue);
		strm >> i64Ret;
	}

	return i64Ret;
}

int 
CDBFPhaseHandling::_ConvertStringToInt( const char* cszValue )
{
	int iRet = 0;

	if( cszValue )
	{
		std::stringstream strm( cszValue);
		strm >> iRet;
	}

	return iRet;
}

std::string 
CDBFPhaseHandling::FindFileForCurrentHour(std::string szFolder)
{
	std::string szFileName="";

	CEMSTime oTempTime = m_oSysTime.GetTime();
	EMSTIMEFIELDS tmTimeFields;
	oTempTime.GetTime( &tmTimeFields );

	CEMSTime  oCurrentTime( tmTimeFields.nYear, tmTimeFields.nMonth, tmTimeFields.nDay, tmTimeFields.nHour );
	std::string szHour = _NumberToString(tmTimeFields.nHour);
	
	std::string filename = "DBFBufferPhase_";
	filename += szHour;
	filename += ".csv";

	std::string FullPath = szFolder.append(filename);
	bool bFound  = FileExists(FullPath);
	if(bFound)
		szFileName = filename;

	return szFileName;
}

bool 
CDBFPhaseHandling::FileExists(const std::string& filename)
{
  WIN32_FIND_DATAA fd = {0};
  HANDLE hFound = FindFirstFileA(filename.c_str(), &fd);
  bool retval = hFound != INVALID_HANDLE_VALUE;
  FindClose(hFound);

  return retval;  
}

std::vector<std::string> 
CDBFPhaseHandling::get_all_files_names_within_folder(std::string folder)
{
    std::vector<std::string> names;
    std::string search_path;
	std::string fileType = "\\*.csv";
	search_path = folder.append(fileType);
    WIN32_FIND_DATA fd; 
    HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd); 
    if(hFind != INVALID_HANDLE_VALUE) { 
        do { 
            // read all (real) files in current folder
            // , delete '!' read other 2 default folder . and ..
            if(! (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
                names.push_back(fd.cFileName);
            }
        }while(::FindNextFile(hFind, &fd)); 
        ::FindClose(hFind); 
    }

    return names;
}

void
CDBFPhaseHandling::_OutputTOCSV( )
{
	if( m_vectDbfBuffer.size() )
	{
		CEMSTime oTempTime = m_oSysTime.GetTime();
		EMSTIMEFIELDS tmTimeFields;
		oTempTime.GetTime( &tmTimeFields );

		CEMSTime  oCurrentTime( tmTimeFields.nYear, tmTimeFields.nMonth, tmTimeFields.nDay, tmTimeFields.nHour );
		std::string szHour = _NumberToString(tmTimeFields.nHour);

		std::string filename = CDBFPSConfig::Instance().PhaseOutputDir() + "DBFBufferVect_";
		filename += szHour;
		filename += ".csv";

		std::fstream file;
		//file.open( "C:\\HGT\\DBFBufferPhase\\BufferVect.csv" , std::ios::out /*| std::ios::app*/ );
		file.open( filename , std::ios::out );
		file << "DataSetType," << "timestamp," << "satid," << "fPlateAzimuth," << "fPlateElevation," << "ulMaxFreqIndex," << "dMaxPowerlevel," << "PhaseArray[32]," << std::endl;
		for( int i = 0; i < m_vectDbfBuffer.size(); i++ )	
		{													
			EMSDBFBUFFER& buffer = m_vectDbfBuffer[i];		 
			file << buffer.iDataSetType << "," << buffer.timeStamp.intTime << "," << buffer.ulSatID << "," << buffer.fPlateAzimuth << "," << buffer.fPlateElevation
				<< "," << buffer.ulMaxFreqIndex << "," << buffer.dMaxPowerlevel << "," ;
			for ( int j = 0; j < 32; j++ )
				file << buffer.PhaseArray[j] << ",";
			file << std::endl;
	
	
		}
		EMSDBFBUFFER& buffer = m_vectDbfBuffer[0];		 
		file << "99," << buffer.timeStamp.intTime << "," << m_vectDbfBuffer.size() << ",0,0,0,0,";
		for ( int j = 0; j < 32; j++ )
			file << m_BiasOffset[j] * c_dRadToDeg << ",";
		file << std::endl;
		
		file << "98," << buffer.timeStamp.intTime << "," << m_vectDbfBuffer.size() << ",0,0,0,0,";
		for ( int j = 0; j < 32; j++ )
			file << m_PhaseStdDev[j] * c_dRadToDeg << ",";
		file << std::endl;

		file.close();
	}
}
bool CDBFPhaseHandling::ReadDBFBufferPhaseFromCsv(std::string &inFileName)
{
	bool ret = true;

	std::ifstream       file(inFileName);


	m_vectDbfBuffer.clear();

	EMSDBFBUFFER dbfBuff;

	int recCount = 0;
	try
	{
		if(file.good())
		{
			CSVRow	row;
			file >> row;  //Get the header

			while(file >> row)
			{
				memset(&dbfBuff, 0, sizeof(EMSDBFBUFFER));
				int rowid = 0;

				recCount++;


				std::string szDataSetTypestr(row[rowid++]);
				dbfBuff.iDataSetType = _ConvertStringToInt(szDataSetTypestr.c_str());

				std::string timeStr(row[rowid++]);
				dbfBuff.timeStamp.intTime = _ConvertStringToInt64(timeStr.c_str());
				dbfBuff.ulSatID = CEMSConversionUtil::ConvertToULong(row[rowid++].c_str());
				dbfBuff.fPlateAzimuth = CEMSConversionUtil::ConvertToFloat(row[rowid++].c_str());
				dbfBuff.fPlateElevation = CEMSConversionUtil::ConvertToFloat(row[rowid++].c_str());
				dbfBuff.ulMaxFreqIndex = CEMSConversionUtil::ConvertToULong(row[rowid++].c_str());
				dbfBuff.dMaxPowerlevel = CEMSConversionUtil::ConvertToDouble(row[rowid++].c_str());

				for(int i = 0; i< 32; i++)
					dbfBuff.PhaseArray[i] = CEMSConversionUtil::ConvertToDouble(row[rowid++].c_str()); //Phase

				m_vectDbfBuffer.push_back(dbfBuff);
			}
		}
		else
		{
			ret = false;
		}
	}
	catch ( ... )
	{
		ret = false;
	}

	return ret;
}

bool 
CDBFPhaseHandling::MoveProcessedFile(std::string szProcessedFile, std::string szBkUpFile)
{
	bool bRet = true;
	remove(szBkUpFile.c_str());
	if (rename(szProcessedFile.c_str(), szBkUpFile.c_str()) )
		bRet = false;
	return bRet;
}

const UINT MIN_BUFF_SIZE = 50 ;
BOOL 
CDBFPhaseHandling::GenerateBiasOffset(  CDBFPointingAngles& opa)
{
	BOOL bRet = FALSE;
	EMSDBFAZEL aAzEl;
	memset( &aAzEl, 0, sizeof( EMSDBFAZEL ) );
	UINT uiCnt = 0;
	if( m_vectDbfBuffer.size() > MIN_BUFF_SIZE )
	{
		for( int i = 0; i < m_vectDbfBuffer.size(); /*i++ */)
		{
			EMSDBFBUFFER& buffer = m_vectDbfBuffer[i];
			if( buffer.iDataSetType == 2 && buffer.ulSatID < 200 && buffer.ulSatID != 10 )
			{
				aAzEl.degPlateAzimuth =  buffer.fPlateAzimuth;
				aAzEl.degPlateElevation = buffer.fPlateElevation;
				opa.CellPhaseAngles( &aAzEl );
				for( int j = 0; j < DBF_MAX_CHANNELS; j++ )
				{
					//buffer.PhaseArray[j] -=  atan2(aAzEl.acmplxPhase[j].im,aAzEl.acmplxPhase[j].re);
					buffer.PhaseArray[j] +=  atan2(aAzEl.acmplxPhase[j].im,aAzEl.acmplxPhase[j].re);
					buffer.PhaseArray[j] = fmod(buffer.PhaseArray[j], c_dTwoPI );
					if( buffer.PhaseArray[j] >  c_dPI ) buffer.PhaseArray[j] -= c_dTwoPI;
					if( buffer.PhaseArray[j] < -c_dPI ) buffer.PhaseArray[j] += c_dTwoPI;
				}
				++uiCnt;
				++i;
			}
			else
			{
				m_vectDbfBuffer.erase(m_vectDbfBuffer.begin() + i );
				//i--;
			}
		}
		if( uiCnt > MIN_BUFF_SIZE )
		{
			for( int j = 0; j < DBF_MAX_CHANNELS; j++ )
			{
				{
					CEMSOutlier emsOutlier;
					emsOutlier.Init( uiCnt ) ;
					double y = 0.0;
					for( int i = 0; i < m_vectDbfBuffer.size(); i++ )
					{
						EMSDBFBUFFER& buffer = m_vectDbfBuffer[i];
						if ( i==0 )
						{
							y = buffer.PhaseArray[j];
						}
						else
						{
							y = 0.99*y + 0.01*buffer.PhaseArray[j];
						}
						//emsOutlier.AddRecord( buffer.PhaseArray[j] );
						emsOutlier.AddRecord( y );
					}
					emsOutlier.Calculate();
					double dMedian = emsOutlier.GetMedian();
					double dThreshold = 2.0 * emsOutlier.GetStddev() + 0.001;
					emsOutlier.SetThreshold( dThreshold, dMedian );
					
					emsOutlier.Calculate();
					m_BiasOffset[j] = emsOutlier.GetMedian();
					m_PhaseStdDev[j] = emsOutlier.GetStddev();
				}
			}
			bRet = TRUE;
		}
	}
	_OutputTOCSV();
	return bRet;
}
