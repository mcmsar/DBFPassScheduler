/********************************************************************
*	Module:			OrbitObj.h
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
********************************************************************/

/*******************************************************************

  Revision Record

	Rev	Date			Auth	Changes
	===	====			====	=======

	0.0	99/07/30		rvw	start

********************************************************************/
#ifndef INC_ORBITOBJ
#define INC_ORBITOBJ

#define INITGUID
#include <initguid.h>
#include "aobjbase.h"
#include "emsorbit.h"
#include "emsatinf.h"
#include "emsmutex.h"

const TCHAR c_szComponentName[]	= TEXT("Orbit");
const TCHAR c_szComponentDesc[]	= TEXT("EMS Orbit Determination");

#define EMS_MAX_SAT_NAME_LENGTH (32)

typedef struct _tagORBITDATA
{
	int			nSatelliteID;
	int			nNoradNumber;
	EMSTLEDATA2	emsTLE;
	TCHAR			szName[ EMS_MAX_SAT_NAME_LENGTH ];
	BOOL			bInUse;
	int			nElementNumber;
} ORBITDATA, FAR *LPORBITDATA;

class CEMSOrbitObject : public IEMSOrbit4, public IEMSOrbitInit, public CApiObjBase
{
public:
	CEMSOrbitObject( );

	HRESULT Init();
	

/*********************************************************************
*	IEMSOrbit Methods
*********************************************************************/

	HRESULT STDMETHODCALLTYPE EarthFixedOrbit( ULONG ulSatelliteID,
				 const EMSTIME __RPC_FAR *pvTime, ULONG cbCount,
				 EMSTIMECOORD __RPC_FAR *pvCoord, ULONG __RPC_FAR *pcbResult);
        
	HRESULT STDMETHODCALLTYPE InertialOrbit( ULONG ulSatelliteID,
				 const EMSTIME __RPC_FAR *pvTime, ULONG cbCount,
				 EMSTIMECOORD __RPC_FAR *pvCoord, ULONG __RPC_FAR *pcbResult);
        
	HRESULT STDMETHODCALLTYPE EarthFixedOrbitTLE( 
            /* [in] */ EMSTLEDATA __RPC_FAR *lpTLE,
            /* [size_is][in] */ const EMSTIME __RPC_FAR *pvTime,
            /* [in] */ ULONG cbCount,
            /* [length_is][size_is][out] */ EMSTIMECOORD __RPC_FAR *pvCoord,
            /* [out] */ ULONG __RPC_FAR *pcbResult );
        
	HRESULT STDMETHODCALLTYPE InertialOrbitTLE( 
            /* [in] */ EMSTLEDATA __RPC_FAR *lpTLE,
            /* [size_is][in] */ const EMSTIME __RPC_FAR *pvTime,
            /* [in] */ ULONG cbCount,
            /* [length_is][size_is][out] */ EMSTIMECOORD __RPC_FAR *pvCoord,
            /* [out] */ ULONG __RPC_FAR *pcbResult );
        
	HRESULT STDMETHODCALLTYPE SetSatelliteTLE( 
            /* [in] */ ULONG ulSatelliteID,
            /* [in] */ const EMSTIME eTime,
            /* [in] */ const EMSTLEDATA __RPC_FAR *pTLEData );

	HRESULT STDMETHODCALLTYPE GetSatelliteTLE( 
            /* [in] */ ULONG ulSatelliteID,
            /* [in] */ const EMSTIME eTime,
            /* [out] */ EMSTLEDATA __RPC_FAR *lpTLEData );

	HRESULT STDMETHODCALLTYPE GetOrbitNumber( 
            /* [in] */ ULONG ulSatelliteID,
            /* [in] */ const EMSTIME eTime,
            /* [out] */ ULONG __RPC_FAR *lpulOrbitNumber );

	HRESULT STDMETHODCALLTYPE PropogateTLE( 
            /* [in] */ EMSTLEDATA __RPC_FAR *lpTLEData,
            /* [in] */ const EMSTIME eTime,
            /* [out] */ EMSTLEDATA __RPC_FAR *lpPropogatedTLE );
        
	HRESULT STDMETHODCALLTYPE Reload( /* [in] */ DWORD dwFlags );

/*********************************************************************
*	IEMSOrbit2 Methods
*********************************************************************/

	HRESULT STDMETHODCALLTYPE SetSatelliteTLE2( /* [in] */ ULONG ulSatelliteID,
            /* [in] */ const EMSTLEDATA2 __RPC_FAR *lpTLEData );
        
	HRESULT STDMETHODCALLTYPE GetSatelliteTLE2( /* [in] */ ULONG ulSatelliteID,
            /* [in] */ const EMSTIME eTime,
            /* [out] */ EMSTLEDATA2 __RPC_FAR *lpTLEData );
        
	HRESULT STDMETHODCALLTYPE ConvertOrbitVectorToTLE( 
            /* [in] */ ULONG ulSatelliteID,
            /* [in] */ EMSTIMECOORD timeCoord,
            /* [out] */ EMSTLEDATA __RPC_FAR *lpTLEData );

/*********************************************************************
*	IEMSOrbit3 Methods
*********************************************************************/

	HRESULT STDMETHODCALLTYPE SetSatelliteTLE3( /* [in] */ ULONG ulSatelliteID,
            /* [in] */ const EMSTLEDATA2 __RPC_FAR *lpTLEData,
            /* [in] */ DWORD dwFlags );

/*********************************************************************
*	IEMSOrbit4 Methods
*********************************************************************/

	HRESULT STDMETHODCALLTYPE EarthFixedOrbitDouble( ULONG ulSatelliteID,
				 const EMSTIME __RPC_FAR *pvTime, ULONG cbCount,
				 EMSTIMECOORDD __RPC_FAR *pvCoord, ULONG __RPC_FAR *pcbResult);
        
	HRESULT STDMETHODCALLTYPE InertialOrbitDouble( ULONG ulSatelliteID,
				 const EMSTIME __RPC_FAR *pvTime, ULONG cbCount,
				 EMSTIMECOORDD __RPC_FAR *pvCoord, ULONG __RPC_FAR *pcbResult);
        
	HRESULT STDMETHODCALLTYPE EarthFixedOrbitTLEDouble( 
            /* [in] */ EMSTLEDATA __RPC_FAR *lpTLE,
            /* [size_is][in] */ const EMSTIME __RPC_FAR *pvTime,
            /* [in] */ ULONG cbCount,
            /* [length_is][size_is][out] */ EMSTIMECOORDD __RPC_FAR *pvCoord,
            /* [out] */ ULONG __RPC_FAR *pcbResult );
        
	HRESULT STDMETHODCALLTYPE InertialOrbitTLEDouble( 
            /* [in] */ EMSTLEDATA __RPC_FAR *lpTLE,
            /* [size_is][in] */ const EMSTIME __RPC_FAR *pvTime,
            /* [in] */ ULONG cbCount,
            /* [length_is][size_is][out] */ EMSTIMECOORDD __RPC_FAR *pvCoord,
            /* [out] */ ULONG __RPC_FAR *pcbResult );
        
	HRESULT STDMETHODCALLTYPE ConvertOrbitVectorToTLEDouble( 
            /* [in] */ ULONG ulSatelliteID,
            /* [in] */ EMSTIMECOORDD timeCoord,
            /* [out] */ EMSTLEDATA __RPC_FAR *lpTLEData );

/*********************************************************************
*	IEMSOrbitInit Methods
*********************************************************************/
	HRESULT STDMETHODCALLTYPE Initialize( void ) { return Init(); }

/*********************************************************************
*	IUnknown Methods
*********************************************************************/
	STDMETHOD(QueryInterface)		(THIS_ REFIID, LPVOID FAR *);
	STDMETHOD_(ULONG, AddRef)		(THIS);
	STDMETHOD_(ULONG, Release)		(THIS);

private: // methods
	virtual ~CEMSOrbitObject();

	int _FindOrbitData( int nID, BOOL bUseNorad=FALSE );
	BOOL _GetOrbitData( int nID, LPORBITDATA lpOrbitData, BOOL bUseNorad=FALSE );
	int _FindEmptySlot( void );
	HRESULT _InitData( void );
	HRESULT _InitDataFromFile( LPCTSTR lpcszFilename );
	int _SatIDFromNoradNumber( int nNoradNumber );
	int _RemoveTrailingUnprintables( LPTSTR lpszText );
	EMSTIME _ConvertEpochTime( double dTime );
	EMS_RESULT _AllocateOrbitData( void );
	EMS_RESULT _AllocateOrbitDataX( void );
	void _DeallocateOrbitData( void );
	EMS_RESULT _UpdateTLERecord( LPORBITDATA lpSatData, LPVOID lpFile );
	EMS_RESULT _UpdateTLEFile( void );
	TCHAR _CalcChecksum( LPCTSTR lpcszBuffer ); 
	void _ToTextExpForm( double dValue, LPTSTR lpszResult );

	EMS_RESULT _ReadTLEFromFile( ULONG ulSatID, LPORBITDATA lpData );
	EMS_RESULT _WriteTLEToFile( ULONG ulSatID, LPORBITDATA lpData );
	EMS_RESULT _SetPath( void );
	BOOL _ValidName( LPCTSTR lpcszSatelliteName );
	HRESULT _ConvertEF2TLE( EMSTIMECOORD *pvCoord, EMSTLEDATA  *lpTLE );
	HRESULT _ConvertEF2TLEDouble( EMSTIMECOORDD *pvCoord, EMSTLEDATA  *lpTLE );
	
private: // data
	BOOL				m_bInitialized;
	LPVOID				m_lpOrbitData;
	LPEMSSATELLITEINFO 	m_lpSatelliteInfo;
	CEMSMutex			m_mutex;
	LPTSTR				m_lpszFilespec;
	LPTSTR				m_lpszPath;
	HANDLE              m_hMapFile;
};

#endif // INC_ORBITOBJ
