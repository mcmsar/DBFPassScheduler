// EMSDBFPassScheduleCfg.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "EMSDBFPassScheduleCfg.h"
#include "LogHelper.h"
#include <iostream>

CEMSDBFPassScheduleCfg::CEMSDBFPassScheduleCfg()
{
}
CEMSDBFPassScheduleCfg::~CEMSDBFPassScheduleCfg()
{
}

WORD 
CEMSDBFPassScheduleCfg::_convertStringToWord(std::string str)
{
	WORD wPlateID = 0;
	std::stringstream ss(str);
	ss >> wPlateID;

	return wPlateID;
}

EMS_RESULT 
CEMSDBFPassScheduleCfg::ReadDBFPlateConfigFromXML(const std::string szDBFPlateCfgFile)
{

	EMS_RESULT hr = EMS_OK;

	EMSDBFARRAY  	  Plate;
	memset(&Plate, 0, sizeof(EMSDBFARRAY));

	try
	{
		::std::auto_ptr< ::dbfplateconfig::plateconfig > cfgPtr (::dbfplateconfig::plateconfig_(szDBFPlateCfgFile.c_str(), xml_schema::flags::dont_validate));

		::dbfplateconfig::dbfplates::dbfplate_sequence seq = cfgPtr->dbfplates().dbfplate();
	
		for(::dbfplateconfig::dbfplates::dbfplate_const_iterator iter = seq.begin();
			iter != seq.end(); iter++)
		{
			std::string PlateIdstr = iter->id();

			Plate.wPlateID = _convertStringToWord(PlateIdstr);
			Plate.fPlateFaceAzimuth = iter->platefaceazimuth();
			Plate.fPlateFaceElevation = iter->platefaceelevation();
			Plate.fPlateFaceRotation = iter->platefacerotation();

			// Polarization = 0 for LHCP, Polarization = 1 for RHCP
			std::string szPlatePolarization = iter->platepolarization();
			if( szPlatePolarization == "RHCP" )
			{
				Plate.wPlatePolarization = 1;
			}
			else
				Plate.wPlatePolarization = 0; 

			Plate.wFixedPointNumber = iter->fixedpointnumber();

			ULONG ulFixedPoints = iter->fixedpoints().fixedpoint().size();

			::dbfplateconfig::fixedpoints::fixedpoint_const_iterator iterFixPoint = iter->fixedpoints().fixedpoint().begin();
			for(ULONG i = 0; (i < ulFixedPoints) && (iterFixPoint != iter->fixedpoints().fixedpoint().end()); 
																							i++, iterFixPoint++)
			{
				std::string szFixedPointId = iterFixPoint->id();
				Plate.emsFixedPoints[i].wFixedPointID = _convertStringToWord(szFixedPointId);

				Plate.emsFixedPoints[i].fFixPointAzimuth = iterFixPoint->fixedpointaz();
				Plate.emsFixedPoints[i].fFixPointElevation = iterFixPoint->fixedpointel();

				std::string szPol = iterFixPoint->fixedpointpol();
				Plate.emsFixedPoints[i].wFixPointPolarization =  _convertStringToWord(szPol); 

				Plate.emsFixedPoints[i].fFixedPointDistance = iterFixPoint->fixedpointdistance();
				Plate.emsFixedPoints[i].bEnabled = iterFixPoint->fixedpointenabled();
			}

			Plate.wCellNumber = iter->cellnumber();

			// khalid: cell numbers are 1 - 31 cell zero is channel 0 that is  1PPS. 
			if( Plate.wCellNumber > 31) Plate.wCellNumber = 31;

			::dbfplateconfig::cells::cell_const_iterator iterCell = iter->cells().cell().begin();
			int iCellCount = iter->cells().cell().size();

			for( int i = 1; (i < ( iCellCount + 1 ) && (i < 32) ) && 
											(iterCell != iter->cells().cell().end()); i++ , iterCell++)

			{
				std::string szCellId = iterCell->id();
				Plate.emsDBFCells[i].wCellID = _convertStringToWord(szCellId);

				Plate.emsDBFCells[i].wChannelID = iterCell->channelid();

				Plate.emsDBFCells[i].fXcoord = iterCell->xcoord();
				Plate.emsDBFCells[i].fYcoord = iterCell->ycoord();
			
				Plate.emsDBFCells[i].fRHCP = iterCell->rhcpbias();
				Plate.emsDBFCells[i].fLHCP = iterCell->lhcpbias();
			}

		}

		m_aDBFplate.push_back(Plate);
	}
	catch(const xml_schema::exception& e)
	{
		//std::cout<<"Exception is "<<e<<std::endl;
		hr = EMS_FALSE;
	}

	return hr;
} 

EMS_RESULT
CEMSDBFPassScheduleCfg::ReadDBFConfigFromXML(const std::string szInFile)
{
 	EMS_RESULT hr = EMS_OK;

	try
	{
		::std::auto_ptr< ::dbfconfig::config > cfgPtr (::dbfconfig::config_(szInFile.c_str(), xml_schema::flags::dont_validate));

		dbfconfig::dbfpassschedule::dbfpassschedulecfg_type passSchedCfg = cfgPtr->dbfpassschedule().dbfpassschedulecfg();

		m_dbfPassScheduleCfg.ulLutId = (ULONG)passSchedCfg.meolutid();

		for(dbfconfig::satellites::constellation_const_iterator iter = passSchedCfg.satellites().constellation().begin();
			iter != passSchedCfg.satellites().constellation().end(); iter++)
		{
		
			std::string str = iter->type();

			if(str == "LEO")
			{
				EMSDBFSATCFG oSatCfg;

				oSatCfg.szType = str;
				oSatCfg.fFrequency = (float)iter->frequency();
				oSatCfg.szPolarisation = iter->polarisation();

				for(dbfconfig::constellation::satellite_const_iterator satIter = iter->satellite().begin();
																		satIter != iter->satellite().end(); satIter++)
				{
					std::string szSatId = satIter->id();
					ULONG ulLeoSatId = std::stoul (szSatId,nullptr,0);
					oSatCfg.ulSatId = ulLeoSatId;
					oSatCfg.ulPriority = satIter->priority();
					m_dbfPassScheduleCfg.vectLeoSats.push_back(oSatCfg);
				}
			}
			else if(str == "GALILEO")
			{
				EMSDBFSATCFG oSatCfg;

				oSatCfg.szType = str;
				oSatCfg.fFrequency = (float)iter->frequency();
				oSatCfg.szPolarisation = iter->polarisation();

				for(dbfconfig::constellation::satellite_const_iterator satIter = iter->satellite().begin();
																		satIter != iter->satellite().end(); satIter++)
				{
					std::string szSatId = satIter->id();
					ULONG ulGaliLeoSatId = std::stoul (szSatId,nullptr,0);
					oSatCfg.ulSatId = ulGaliLeoSatId;
					oSatCfg.ulPriority = satIter->priority();
					m_dbfPassScheduleCfg.vectGalileoSats.push_back(oSatCfg);
				}
			}
			else if(str == "BEIDOU")
			{
				EMSDBFSATCFG oSatCfg;

				oSatCfg.szType = str;
				oSatCfg.fFrequency = (float)iter->frequency();
				oSatCfg.szPolarisation = iter->polarisation();

				for(dbfconfig::constellation::satellite_const_iterator satIter = iter->satellite().begin();
																		satIter != iter->satellite().end(); satIter++)
				{
					std::string szSatId = satIter->id();
					ULONG ulGaliLeoSatId = std::stoul (szSatId,nullptr,0);
					oSatCfg.ulSatId = ulGaliLeoSatId;
					oSatCfg.ulPriority = satIter->priority();
					m_dbfPassScheduleCfg.vectBeidouSats.push_back(oSatCfg);
				}
			}

		}


		dbfconfig::dbfpassschedule::dbfplates_type dbfPlates = cfgPtr->dbfpassschedule().dbfplates();

		for(dbfconfig::dbfplates::dbfplate_const_iterator iter = dbfPlates.dbfplate().begin();
														iter != dbfPlates.dbfplate().end(); iter++)
		{
			EMSDBFPLATECFG odbfPlateCfg;

			std::string szPlateId = iter->id();
			odbfPlateCfg.wPlateId = (WORD)(std::stoul (szPlateId,nullptr,0));

			odbfPlateCfg.szPlateCfgFile = iter->plateconfigfile();

			odbfPlateCfg.fPlateMinElevation = (float)iter->plateminelevation(); 

			odbfPlateCfg.fPlateMinSeparationAngle = (float)iter->plateminseparationangle();

			odbfPlateCfg.plateLocation.degLatitude = iter->location().latitude(); 
			odbfPlateCfg.plateLocation.degLongitude =  iter->location().longitude();
			odbfPlateCfg.plateLocation.fAltitude = (float)iter->location().altitude();

			for(dbfconfig::combiners::combiner_const_iterator iterCombiner = iter->combiners().combiner().begin();
																iterCombiner != iter->combiners().combiner().end();
																iterCombiner++)
			{
				EMSDBFCOMBINERCFG oDbfCombinerCfg;

				std::string szCombinerId = iterCombiner->id();
				oDbfCombinerCfg.wCombinerId = (WORD)(std::stoul (szCombinerId,nullptr,0));

				oDbfCombinerCfg.szConstellation = iterCombiner->constellation();
				oDbfCombinerCfg.szIpAddress = iterCombiner->ipaddress();

				odbfPlateCfg.vectDbfCombiner.push_back(oDbfCombinerCfg);
			}

			m_dbfPassScheduleCfg.vectDbfPlate.push_back(odbfPlateCfg);
		}
	}
	catch(const xml_schema::exception& e)
	{
		//std::cout<<"Exception is "<<e<<std::endl;
		hr = EMS_FALSE;
	}

	return hr;
}

