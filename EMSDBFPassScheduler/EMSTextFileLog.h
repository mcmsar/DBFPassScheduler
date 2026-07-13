/*********************************************************************
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

	$Log:
	$
********************************************************************/

#ifndef __EMS_TEXT_FILE_H__
#define __EMS_TEXT_FILE_H__

#include <string>
#include "emstime.h"
#include "EMSCLOCK.H"

class CEMSTextFile  
{
public:
	CEMSTextFile();
	virtual ~CEMSTextFile();

	bool OpenTextFile( const char* szFileName, const char* szMode );
	void Close();

	bool WriteText( std::string szText );

	bool IsFileOpen(){ return (NULL != m_lpFile);}

private:
	FILE*	    m_lpFile;

	std::string m_szFileName;

};

#endif
