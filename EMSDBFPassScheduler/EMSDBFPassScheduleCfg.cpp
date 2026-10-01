// EMSDBFPassScheduleCfg.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "EMSDBFPassScheduleCfg.h"
#include "LogHelper.h"
#include "DBFPSConfig.h"
#include <iostream>
#include <fstream>
#include <iterator>
#include <sstream>
#include <cstdarg>
#include <cctype>
#include <cstdlib>

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


// ---------------------------------------------------------------------------
// Plate file formats.
//
// The pass scheduler and the DBF collector read the same plate description,
// and it exists in two XML shapes:
//
//   scheduler  <ns2:plateconfig xmlns:ns2="urn:dbfplateconfig">
//                <dbfplates><dbfplate id="1"><platefaceazimuth>...<cells><cell id="1">
//                <channelid>..<xcoord>..<rhcpbias>..
//   collector  <DBFplates><Plate id="1"><PlateFaceAzimuth>...<Cells><Cell id="1">
//                <ChannelID>..<Xcoord>..<RHCPbias>..   (<PlatePolarization> optional)
//
// Same fields, different root and element case. The generated reader
// (dbfPlateConfig.hxx) only accepts the first, and on 2026-09-24 a collector-
// format plate deployed to DBFPSConfig made it throw on every start; the
// exception was swallowed, no plate was loaded, and SetPlate() copied from an
// empty vector - an access violation in VCRUNTIME140 on every restart.
//
// ReadDBFPlateConfigFromXML now tries the generated reader first (unchanged
// behaviour for the scheduler's own format) and, if that throws, reads either
// shape with the tolerant reader below: element names matched without regard
// to case, comments ignored, same field-by-field mapping as the generated
// path so both produce the same EMSDBFARRAY for the same content.
// ---------------------------------------------------------------------------

namespace
{
	void _PlateTrace( const char* pszFmt, ... )
	{
		FILE* fp = CDBFPSConfig::Instance().OpenTraceLog();
		if( !fp )
			return;
		va_list args;
		va_start( args, pszFmt );
		vfprintf( fp, pszFmt, args );
		va_end( args );
		fputc( '\n', fp );
		fclose( fp );
	}

	std::string _Trim( const std::string& s )
	{
		size_t b = 0, e = s.size();
		while( b < e && isspace( (unsigned char)s[b] ) ) b++;
		while( e > b && isspace( (unsigned char)s[e - 1] ) ) e--;
		return s.substr( b, e - b );
	}

	std::string _Lower( const std::string& s )
	{
		std::string o( s );
		for( size_t i = 0; i < o.size(); i++ ) o[i] = (char)tolower( (unsigned char)o[i] );
		return o;
	}

	// Same conversion as CEMSDBFPassScheduleCfg::_convertStringToWord.
	WORD _Word( const std::string& str )
	{
		WORD w = 0;
		std::stringstream ss( str );
		ss >> w;
		return w;
	}

	bool _Bool( const std::string& str )
	{
		const std::string cs = _Lower( _Trim( str ) );
		return ( cs == "true" ) || ( cs == "1" );
	}

	// Blank out <!-- ... --> spans. Replaced with spaces rather than removed, so
	// offsets in the text and in its lower-cased copy still line up.
	void _BlankComments( std::string& s )
	{
		size_t p = 0;
		while( ( p = s.find( "<!--", p ) ) != std::string::npos )
		{
			size_t q = s.find( "-->", p + 4 );
			q = ( q == std::string::npos ) ? s.size() : q + 3;
			for( size_t i = p; i < q; i++ )
				if( s[i] != '\n' ) s[i] = ' ';
			p = q;
		}
	}

	// Position of the next opening <tag within [b, e) whose name is exactly tag
	// (so <cell does not match <cells or <cellnumber, and <plate does not match
	// <platefaceazimuth). lo is the lower-cased text; tag is lower case.
	size_t _FindOpen( const std::string& lo, size_t b, size_t e, const std::string& tag )
	{
		const std::string cso = "<" + tag;
		size_t p = b;
		while( ( p = lo.find( cso, p ) ) != std::string::npos && p < e )
		{
			const size_t cn = p + cso.size();
			const char c = ( cn < lo.size() ) ? lo[cn] : '\0';
			if( c == '>' || c == '/' || isspace( (unsigned char)c ) )
				return p;
			p = cn;
		}
		return std::string::npos;
	}

	// One element instance: [open, end of its closing tag), and its content.
	struct Elem { size_t open, tagEnd, inner, innerEnd, end; };

	bool _FindElem( const std::string& lo, size_t b, size_t e, const std::string& tag, Elem& r )
	{
		const size_t p = _FindOpen( lo, b, e, tag );
		if( p == std::string::npos ) return false;
		const size_t gt = lo.find( '>', p );
		if( gt == std::string::npos || gt >= e ) return false;
		r.open = p; r.tagEnd = gt; r.inner = gt + 1;
		if( lo[gt - 1] == '/' ) { r.innerEnd = r.inner; r.end = gt + 1; return true; }	// <tag/>
		const std::string csc = "</" + tag + ">";
		const size_t q = lo.find( csc, r.inner );
		if( q == std::string::npos || q > e ) return false;
		r.innerEnd = q; r.end = q + csc.size();
		return true;
	}

	// Trimmed text of the first <tag> directly inside [b, e), or "" (bFound false).
	std::string _Value( const std::string& txt, const std::string& lo, size_t b, size_t e,
	                    const std::string& tag, bool* pbFound = NULL )
	{
		Elem r;
		const bool cb = _FindElem( lo, b, e, tag, r );
		if( pbFound ) *pbFound = cb;
		return cb ? _Trim( txt.substr( r.inner, r.innerEnd - r.inner ) ) : std::string();
	}

	// The id="..." attribute of the opening tag at [open, tagEnd).
	std::string _IdAttr( const std::string& txt, const std::string& lo, const Elem& r )
	{
		size_t p = r.open;
		while( ( p = lo.find( "id", p ) ) != std::string::npos && p < r.tagEnd )
		{
			// A whole attribute name: preceded by whitespace, followed by '='.
			size_t q = p + 2;
			while( q < r.tagEnd && isspace( (unsigned char)lo[q] ) ) q++;
			if( isspace( (unsigned char)lo[p - 1] ) && q < r.tagEnd && lo[q] == '=' )
			{
				const size_t qa = txt.find_first_of( "\"'", q );
				if( qa == std::string::npos || qa >= r.tagEnd ) break;
				const size_t qb = txt.find( txt[qa], qa + 1 );
				if( qb == std::string::npos || qb > r.tagEnd ) break;
				return _Trim( txt.substr( qa + 1, qb - qa - 1 ) );
			}
			p += 2;
		}
		return std::string();
	}

	float _Float( const std::string& s ) { return (float)atof( s.c_str() ); }
}

// Tolerant reader for either plate format - see the note above. Returns false
// with a reason when the file cannot be read or holds no recognisable plate.
static bool _ReadPlateTolerant( const std::string& szFile, EMSDBFARRAY& Plate, std::string& sWhy )
{
	std::ifstream in( szFile.c_str(), std::ios::binary );
	if( !in )
	{
		sWhy = "cannot open file";
		return false;
	}
	std::string txt( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
	_BlankComments( txt );
	const std::string lo = _Lower( txt );

	// <dbfplate> in the scheduler's format, <Plate> in the collector's.
	const char* cszPlateTag = ( _FindOpen( lo, 0, lo.size(), "dbfplate" ) != std::string::npos ) ? "dbfplate" : "plate";

	int nPlates = 0;
	size_t pos = 0;
	Elem ePlate;
	while( _FindElem( lo, pos, lo.size(), cszPlateTag, ePlate ) )
	{
		pos = ePlate.end;
		nPlates++;
		const size_t b = ePlate.inner, e = ePlate.innerEnd;

		// Mirrors the generated-reader path in ReadDBFPlateConfigFromXML field for
		// field, including its indexing, so the two produce the same plate.
		Plate.wPlateID            = _Word( _IdAttr( txt, lo, ePlate ) );
		Plate.fPlateFaceAzimuth   = _Float( _Value( txt, lo, b, e, "platefaceazimuth" ) );
		Plate.fPlateFaceElevation = _Float( _Value( txt, lo, b, e, "platefaceelevation" ) );
		Plate.fPlateFaceRotation  = _Float( _Value( txt, lo, b, e, "platefacerotation" ) );

		// Absent in older collector files, which the collector reads as LHCP too.
		Plate.wPlatePolarization  = ( _Lower( _Value( txt, lo, b, e, "platepolarization" ) ) == "rhcp" ) ? 1 : 0;

		Plate.wFixedPointNumber   = _Word( _Value( txt, lo, b, e, "fixedpointnumber" ) );

		Elem eFps;
		if( _FindElem( lo, b, e, "fixedpoints", eFps ) )
		{
			size_t fp = eFps.inner;
			Elem eFp;
			ULONG i = 0;
			while( i < (ULONG)( sizeof(Plate.emsFixedPoints) / sizeof(Plate.emsFixedPoints[0]) ) &&
			       _FindElem( lo, fp, eFps.innerEnd, "fixedpoint", eFp ) )
			{
				fp = eFp.end;
				const size_t fb = eFp.inner, fe = eFp.innerEnd;
				Plate.emsFixedPoints[i].wFixedPointID         = _Word( _IdAttr( txt, lo, eFp ) );
				Plate.emsFixedPoints[i].fFixPointAzimuth      = _Float( _Value( txt, lo, fb, fe, "fixedpointaz" ) );
				Plate.emsFixedPoints[i].fFixPointElevation    = _Float( _Value( txt, lo, fb, fe, "fixedpointel" ) );
				Plate.emsFixedPoints[i].wFixPointPolarization = _Word( _Value( txt, lo, fb, fe, "fixedpointpol" ) );
				Plate.emsFixedPoints[i].fFixedPointDistance   = _Float( _Value( txt, lo, fb, fe, "fixedpointdistance" ) );
				Plate.emsFixedPoints[i].bEnabled              = _Bool( _Value( txt, lo, fb, fe, "fixedpointenabled" ) );
				i++;
			}
		}

		Plate.wCellNumber = _Word( _Value( txt, lo, b, e, "cellnumber" ) );
		if( Plate.wCellNumber > 31 ) Plate.wCellNumber = 31;

		// Cells go to emsDBFCells[1..31], exactly as the generated path stores
		// them (cell zero is channel 0, the 1PPS). NOTE this is the existing
		// layout, kept as is so both readers agree: emsDBFCells has 31 entries,
		// so the 31st cell lands one element past it, and the consumers read it
		// back from the same place.
		Elem eCells;
		int nCells = 0;
		if( _FindElem( lo, b, e, "cells", eCells ) )
		{
			size_t cp = eCells.inner;
			Elem eCell;
			for( int i = 1; ( i < 32 ) && _FindElem( lo, cp, eCells.innerEnd, "cell", eCell ); i++ )
			{
				cp = eCell.end;
				const size_t cb = eCell.inner, ce = eCell.innerEnd;
				Plate.emsDBFCells[i].wCellID    = _Word( _IdAttr( txt, lo, eCell ) );
				Plate.emsDBFCells[i].wChannelID = _Word( _Value( txt, lo, cb, ce, "channelid" ) );
				Plate.emsDBFCells[i].fXcoord    = _Float( _Value( txt, lo, cb, ce, "xcoord" ) );
				Plate.emsDBFCells[i].fYcoord    = _Float( _Value( txt, lo, cb, ce, "ycoord" ) );
				Plate.emsDBFCells[i].fRHCP      = _Float( _Value( txt, lo, cb, ce, "rhcpbias" ) );
				Plate.emsDBFCells[i].fLHCP      = _Float( _Value( txt, lo, cb, ce, "lhcpbias" ) );
				nCells++;
			}
		}

		if( 0 == nCells )
		{
			sWhy = "plate element found but it has no <cells>/<cell> entries";
			return false;
		}
	}

	if( 0 == nPlates )
	{
		sWhy = "no <dbfplate> or <Plate> element found";
		return false;
	}
	return true;
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
			// Trimmed: the files pad values (" RHCP "), and an untrimmed compare read
			// every padded RHCP plate as LHCP.
			std::string szPlatePolarization = _Trim( iter->platepolarization() );
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
		// Not the scheduler's own format (or not valid XML). Say why - this used to
		// be silent, and the empty plate list it left behind crashed the service -
		// then try the tolerant reader, which also accepts the collector's format.
		const std::string csWhat = e.what();
		_PlateTrace( "DBF PLATE: '%s' is not in the scheduler's plate schema (%s); trying the collector format",
		             szDBFPlateCfgFile.c_str(), csWhat.c_str() );
		hr = EMS_FALSE;
	}
	catch(...)
	{
		_PlateTrace( "DBF PLATE: '%s' could not be parsed by the schema reader (unknown exception); "
		             "trying the collector format", szDBFPlateCfgFile.c_str() );
		hr = EMS_FALSE;
	}

	if( EMS_OK != hr )
	{
		memset( &Plate, 0, sizeof(EMSDBFARRAY) );
		std::string sWhy;
		if( _ReadPlateTolerant( szDBFPlateCfgFile, Plate, sWhy ) )
		{
			m_aDBFplate.push_back( Plate );
			hr = EMS_OK;

			ULONG nCells = 0;
			for( int i = 1; i < 32; i++ ) if( 0 != Plate.emsDBFCells[i].wChannelID ) nCells++;
			_PlateTrace( "DBF PLATE: loaded '%s' with the tolerant reader - plate id %u, face az %.1f el %.1f "
			             "rot %.1f, polarization %s, %lu cells", szDBFPlateCfgFile.c_str(),
			             (unsigned)Plate.wPlateID, Plate.fPlateFaceAzimuth, Plate.fPlateFaceElevation,
			             Plate.fPlateFaceRotation, Plate.wPlatePolarization ? "RHCP" : "LHCP", nCells );
		}
		else
		{
			_PlateTrace( "DBF PLATE: *** could not load '%s' in either format (%s) - no plate configuration, "
			             "pass schedules for this plate will NOT be generated ***",
			             szDBFPlateCfgFile.c_str(), sWhy.c_str() );
		}
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
				oSatCfg.fFrequency = (double)iter->frequency();
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
				oSatCfg.fFrequency = (double)iter->frequency();
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
				oSatCfg.fFrequency = (double)iter->frequency();
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
			else if(str == "GLONASS")
			{
				EMSDBFSATCFG oSatCfg;

				oSatCfg.szType = str;
				oSatCfg.fFrequency = (double)iter->frequency();
				oSatCfg.szPolarisation = iter->polarisation();

				for(dbfconfig::constellation::satellite_const_iterator satIter = iter->satellite().begin();
																		satIter != iter->satellite().end(); satIter++)
				{
					std::string szSatId = satIter->id();
					ULONG ulGlonassSatId = std::stoul (szSatId,nullptr,0);
					oSatCfg.ulSatId = ulGlonassSatId;
					oSatCfg.ulPriority = satIter->priority();
					m_dbfPassScheduleCfg.vectGlonassSats.push_back(oSatCfg);
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

			// <plateminazimuth>/<platemaxazimuth> are optional. Absent means "no azimuth
			// limit", which we express as the full circle so the check downstream needs
			// no special case. Both are read independently, so a config may pin just one
			// edge of the window.
			odbfPlateCfg.fPlateMinAzimuth = iter->plateminazimuth().present()
											? (float)iter->plateminazimuth().get() : 0.0f;

			odbfPlateCfg.fPlateMaxAzimuth = iter->platemaxazimuth().present()
											? (float)iter->platemaxazimuth().get() : 360.0f;

			// Also optional, and defaulted to the values these thresholds carried while
			// they were compile-time constants in EMSDBFPassScheduler.cpp, so a config
			// that names none of them schedules exactly as before.
			odbfPlateCfg.fSatMinElevation = iter->satminelevation().present()
											? (float)iter->satminelevation().get() : 12.0f;

			odbfPlateCfg.fSatMaxElevation = iter->satmaxelevation().present()
											? (float)iter->satmaxelevation().get() : 85.0f;

			odbfPlateCfg.fPlateMinElevationLEO = iter->plateminelevationleo().present()
											? (float)iter->plateminelevationleo().get() : 12.0f;

			odbfPlateCfg.fPlateMinElevationMEO = iter->plateminelevationmeo().present()
											? (float)iter->plateminelevationmeo().get() : 12.0f;

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

