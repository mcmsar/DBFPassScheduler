/********************************************************************
*	Module:			emsDBFtypes.h
*	Process ID:
*	S/W Platforms:
*	H/W Platforms:
*	Compiler:
*	Description: 	
*
*	Usage:
*	Entry Point:
*	Input Files:
*	Output Files:
*	Comments:
*
*********************************************************************
*	              Copyright (c) 2006 by EMS Technologies, Inc.,
*										All rights reserved
*	This program is unpublished software and contains the trade secrets
*	and confidential information of EMS Technologies, Inc.  It may not be 
* reproduced in whole or in part, in any form or by any means whatsoever 
* without the express written permission of EMS Technologies, Inc.
*
********************************************************************/

/*******************************************************************

  Revision Record

	Rev	Date			Auth	Changes
	===	====			====	=======

	0.0	20/01/10		rcr		start
	1.0	25/10/23		rcr		ref beacon additions

********************************************************************/
#ifndef INC_EMSDBFTYPES2
#define INC_EMSDBFTYPES2

#include "sigproc.h"
#include "emsorbit.h"

#define DBF_MAX_CHANNELS    (32)	// maximum number of input channels (based upon ADC board capabilities)
#define DBF_MAX_ELEMENTS    (31)	// maximum number of antenna elements
#define DBF_MAX_SATELLITES  (5)		// maximum number of DBF tracked satellites per face

typedef struct tagEMSDBFAZEL
{
	ULONG ulSatID;
	ULONG ulSatPriority;
	ULONG ulPassID;
	EMSDEGREES	degMinSeparation;
	EMSDEGREES	degSatElevation;
	EMSDEGREES	degSatAzimuth;
	EMSDEGREES	degPlateElevation;
	EMSDEGREES	degPlateAzimuth;
	EMSCOMPLEX	acmplxPhase[DBF_MAX_CHANNELS];
	EMSTIMECOORD timeCoord;
	double dGain;
} EMSDBFAZEL;

typedef struct tagEMSDBFAZEL __RPC_FAR *LPEMSDBFAZEL;

typedef struct tagEMSDBFCELLCONFG
{
	WORD  wCellID;// RF cell number associated with the physical element in the array.
	WORD  wChannelID;// RF Channel number associated with A/D channel number (in sequential series)
	float fXcoord;// Cell X coordinate relative to plate phase centre (metres)
	float fYcoord;// Cell Y coordinate relative to plate phase centre (metres)
	float fRHCP;// Cell RHCP phase bias (deg)
	float fLHCP;// Cell LHCP phase bias (deg)
} EMSDBFCELLCONFG;

typedef struct tagEMSDBFCELLCONFG __RPC_FAR *LPEMSDBFCELLCONFG;


typedef struct tagEMSDBFFIXPOINTCONFG
{
	WORD  wFixedPointID;			// Fixed Point Identifier
	float fFixPointAzimuth;			// Fixed point azimuth angle (degN)
	float fFixPointElevation;		// Fixed point elevation angle (deg)
	float fFixedPointDistance;		// Distance from the transmitter to the center of the plate (cm).
	WORD  wFixPointPolarization;	// Fixed Point Polarization Flag ( Polarization = 0 for LHCP, Polarization = 1 for RHCP )
	bool  bEnabled;					// Enable flag (true or false)
	float fPhaseAdjust[DBF_MAX_ELEMENTS]; // Near field phase adjustments 
} EMSDBFFIXPOINTCONFG;

typedef struct tagEMSDBFFIXPOINTCONFG __RPC_FAR *LPEMSDBFFIXPOINTCONFG;


typedef struct _tagEMSDBFARRAY
{
	WORD  wPlateID;				// Plate Identifier number, defining an array plate and associated combiner board ( up to 8 plates)
	WORD  wPlatePolarization;	// Plate Polarization Flag ( Polarization = 0 for LHCP, Polarization = 1 for RHCP )
	WORD  wCellNumber;			// Number of active cells
	float fPlateFaceAzimuth;	// Antenna Plate Face azimuth angle (degN)
	float fPlateFaceElevation;	// Antenna Plate Face elevation angle (deg)
	float fPlateFaceRotation;	// Antenna Plate Face rotation angle of face major axis relative to face perpendicular and up direction (deg)
	WORD  wFixedPointNumber;    // number of fixed points (zero means don't use fixed points).
	
	EMSDBFCELLCONFG emsDBFCells[DBF_MAX_ELEMENTS];
	EMSDBFFIXPOINTCONFG emsFixedPoints[10];

	ULONG ulReserved;			// Reserved for future growth
}	EMSDBFARRAY;

typedef struct _tagEMSDBFARRAY __RPC_FAR *LPEMSDBFARRAY;

typedef struct _tagREFERENCEBEACON
{
	char  cBeaconID[15];	// Reference Beacon 15 Hex
	char  cBeaconMess[36];  // Reference Beacon 36 Hex
	EMSLOCATION refLocation;	// Reference Beacon Location
	float fFrequency;		// Transmit frequency (MHz)
	int   nRrate;			// Transmit rate (sec)
	int   nStartTime;		// Start time from 00:00:00
}	REFERENCEBEACON;

typedef struct _tagREFERENCEBEACON __RPC_FAR *LPREFERENCEBEACON;

typedef struct _tagREFERENCEBEACONS
{
	REFERENCEBEACON rec[60]; // Maximum number based on transmission second within 1 minute
}	REFERENCEBEACONS;

typedef struct _tagEMSDBFPASSRECORDS __RPC_FAR *LPEMSDBFPASSRECORDS;

// Wire-format precision for EMSDBFPASSRECORD2::fDownFrequency. EMSSPServer.dll
// (the downstream Signal Conditioner) expects this field as a 4-byte float --
// widening it to double changes the record's byte layout and crashes that
// process (STATUS_STACK_BUFFER_OVERRUN, confirmed 2026-07-30). Only flip this
// to 1 once every consumer of the wire format -- scheduler, collector, AND the
// SP Server -- has been rebuilt to agree on the wider layout. This must be
// defined identically here and in the collector's emsDBFtypes2.h.
#ifndef EMSDBF_FREQUENCY_DOUBLE
#define EMSDBF_FREQUENCY_DOUBLE 0
#endif

#if EMSDBF_FREQUENCY_DOUBLE
typedef double EMSDBFFREQ;
#else
typedef float EMSDBFFREQ;
#endif

typedef struct _tagEMSDBFPASSRECORD2
{
	EMSTIME timestamp;		// Time stamp in nanoseconds.
	ULONG ulLutID;			// LUT Identifier number, according to Cospas-Sarsat.
	ULONG ulSatID;			// Satellite Identifier number, according to Cospas-Sarsat.
	ULONG ulPassID;			// Satellite orbit revolution number.
	WORD  wProcessID;		// Process Identifier: 0 to 5  (see process definition).
	WORD  wPlateID;			// Plate Identifier number, defining an array plate ( up to 8 plates)
	WORD  wAntennaID;		// Antenna Identifier number, defining a beam pattern ( up to 4 beams allowed)
	EMSDBFFREQ fDownFrequency;	// Constellation downlink frequency (same unit as configured in DBFPassScheduler.xml)
	WORD  wPolarization;	// Signal polarization: 0 = LHCP, 1 = RHCP
	float fAzimuth;			// Site azimuth pointing angle to satellite (degN)
	float fElevation;		// Site elevation pointing angle to satellite (deg)
	float fPlateAzimuth;	// Azimuth pointing angle to satellite relative to plate face (degN)
	float fPlateElevation;	// Elevation pointing angle to satellite relative to plate face (deg)
	float fBeaconElevation; // Reference Beacon elevation angle to satellite (deg)
	char  cBeaconID[15];	// Reference Beacon 15 Hex
	char  cBeaconMess[36];  // Reference Beacon 36 Hex
	EMSLOCATION RefLocation; // Reference Beacon Location (lat/long/alt)
	float fRefTxFrequency;  // Reference Beacon Transmit Frequency
	float fFOA;				// Baseband Freqency of Arrival (0 to 100,000 Hz)
	float fTOA;				// Time of Arrival (fraction of a second)
	float fPhase[DBF_MAX_CHANNELS];	// Phase correction (deg) (up to 31 channels)
	EMSTIMECOORD timeCoord;
}	EMSDBFPASSRECORD2;

typedef struct _tagEMSDBFPASSRECORD2 __RPC_FAR *LPEMSDBFPASSRECORD2;

typedef struct _tagEMSDBFPASSRECORDS2
{
	EMSDBFPASSRECORD2 rec[DBF_MAX_SATELLITES];
}	EMSDBFPASSRECORDS2;

typedef struct _tagEMSDBFPASSRECORDS __RPC_FAR *LPEMSDBFPASSRECORDS;

//	PROCESS Definition

#define PROCESS_EIGENVECTOR           0x0001;	// Eigenvector phase corrections
#define PROCESS_PREDICT_PHASE         0x0002;   // Predicted phase estimates with nulling
#define PROCESS_CARRIER_TRACK         0x0004;   // Carrier tracking assuming a single satellite in view
#define PROCESS_RAW_DATA              0x0008;   // Raw data collection (32 channel outputs)

#define PROCESS_OUTPUT_TIME           0x0100;   // WAV file output in time series (complex format, default real format)
#define PROCESS_OUTPUT_FREQUENCY      0x0200;   // WAV file output in frequency bins (complex format, default time series)
#define PROCESS_INPUT_SAMPLERATE      0x0400;   // Input samplerate (1 MHz/1 sec, default 500 kHz/2 sec)
#define PROCESS_OUTPUT_SAMPLERATE     0x0800;   // Output samplerate (200 kHz, default 400 kHz)
#define PROCESS_OUTPUT_WAVE           0x1000;   // Output WAV format (disk file, default stream) 

#define PROCESS_DEFAULT               0x0001;   // Eigenvector processing,
										        // time series (real format),
                                                // Input samplerate 500 kHz/2 sec
                                                // Output samplerate 400 kHz (200 kHz bandwidth)
                                                // Output WAV files


#endif // INC_EMSDBFTYPES2

