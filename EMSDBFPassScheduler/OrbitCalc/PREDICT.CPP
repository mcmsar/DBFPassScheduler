/*      DRIVER                                                 3 NOV 80
 *
 *   WGS-72 PHYSICAL AND GEOPOTENTIAL CONSTANTS
 *          CK2= .5*J2*AE**2     CK4=-.375*J4*AE**4
 */

//---------------------------------------------------------------------------
//#include <condefs.h>
#include <math.h>
#include "SgpxStr.h"
#include "Predict.h"
#include "sgp4ext.h"    // for several misc routines
#include "sgp4unit.h"   // for sgp4init and getgravconst

#pragma warning ( disable : 4244 )

//---------------------------------------------------------------------------
// This is the TOrbit constructor which performs all the initialization
// required before a predict can be determined
TOrbit::TOrbit(TTLEVector *TVec, int ept)
{

	iept = ept;
	InitFlag = 1;

	TLEVec.epoch = TVec->epoch;
	TLEVec.xndt2o = TVec->xndt2o;
	TLEVec.xndd6o = TVec->xndd6o;
	TLEVec.iexp = TVec->iexp;
	TLEVec.bstar = TVec->bstar;
	TLEVec.ibexp = TVec->ibexp;
	TLEVec.xincl = TVec->xincl;
	TLEVec.xnodeo = TVec->xnodeo;
	TLEVec.eo = TVec->eo;
	TLEVec.omegao = TVec->omegao;
	TLEVec.xmo = TVec->xmo;
	TLEVec.xno = TVec->xno;
	TLEVec.norbt = TVec->norbt;
			 
	m_propogatedTLEVec = TLEVec;

	// Special test case for debug purposes
	iept = 0;

	if (iept==0)
	{
		//    opsmode     - mode of operation afspc or improved 'a', 'i'
		char opsmode = 'a';
		//    whichconst  - which set of constants to use  72, 84
		m_whichconst = wgs84;
		struct elsetrec satrec;
		const double deg2rad  =   pi / 180.0;         //   0.0174532925199433
		const double xpdotp   =  1440.0 / (2.0 *pi);  // 229.1831180523293
		double sec, mu, radiusearthkm, tumin, xke, j2, j3, j4, j3oj2;
		int year = 0;
		int mon, day, hr, minute;

		memset( &m_satrec, 0, sizeof(elsetrec) );
		satrec = m_satrec;

		getgravconst( m_whichconst, tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2 );

		satrec.error = 0;

		// ---- find no, ndot, nddot ----
		satrec.no   = TVec->xno / xpdotp; //* rad/min
		satrec.nddot= TVec->xndd6o * pow(10.0, TVec->iexp);
		satrec.bstar= TVec->bstar * pow(10.0, TVec->ibexp);

		// ---- convert to sgp4 units ----
		satrec.a    = pow( satrec.no*tumin , (-2.0/3.0) );
		satrec.ndot = TVec->xndt2o  / (xpdotp*1440.0);  //* ? * minperday
		satrec.nddot= TVec->xndd6o / (xpdotp*1440.0*1440);

		// ---- find standard orbital elements ----
		satrec.inclo = TVec->xincl   * deg2rad;
		satrec.nodeo = TVec->xnodeo  * deg2rad;
		satrec.argpo = TVec->omegao  * deg2rad;
		satrec.mo    = TVec->xmo     * deg2rad;
		satrec.ecco  = TVec->eo;
		satrec.alta = satrec.a*(1.0 + TVec->eo) - 1.0;
		satrec.altp = satrec.a*(1.0 - TVec->eo) - 1.0;

		// ----------------------------------------------------------------
		// find sgp4epoch time of element set
		// remember that sgp4 uses units of days from 0 jan 1950 (sgp4epoch)
		// and minutes from the epoch (time)
		// ----------------------------------------------------------------

		// ---------------- temp fix for years from 2000 -------------------
		satrec.epochyr = (int) (TVec->epoch / 1000.0 );
		satrec.epochdays = TVec->epoch - (double) satrec.epochyr * 1000;
		year= satrec.epochyr + 2000;

		days2mdhms ( year,satrec.epochdays, mon,day,hr,minute,sec );
		jday( year,mon,day,hr,minute,sec, satrec.jdsatepoch );

		satrec.satnum = TVec->norbt;

		// ---------------- initialize the orbit at sgp4epoch -------------------
		sgp4init( m_whichconst, opsmode, satrec.satnum, satrec.jdsatepoch-2433281.5, satrec.bstar,
				satrec.ecco, satrec.argpo, satrec.inclo, satrec.mo, satrec.no,
				satrec.nodeo, satrec);
		m_satrec = satrec;

	}
//	else
//	{
//		double  qo = (double)120.0, so = (double)78.0, xj2 = (double)1.082616e-3, xj4 = (double)-1.65597e-6;
//		int   iexp, ibexp, ideep;
//		double temp, a1, del1, ao, delo, xnodp, inittime;
//
//		e6a = (double)1.e-6;
//		tothrd = 2.0 / 3.0;
//		xj3 = (double)-.253881e-5;
//		xke = (double).743669161e-1;
//		xkmper = (double)6378.135;
//		xmnpda = 1440.;
//		ae = 1.;
//
//		ck2 = .5 * xj2 * pow (ae, 2);
//		ck4 = -.375 * xj4 * pow (ae, 4);
//		qoms2t = pow ((qo-so) * ae / xkmper, 4);
//		s   = ae * (1. + so / xkmper);
//
//		epoch  =  TLEVec.epoch;
//		xndt2o =  TLEVec.xndt2o;
//		xndd6o =  TLEVec.xndd6o;
//		iexp   =  TLEVec.iexp;
//		bstar  =  TLEVec.bstar;
//		ibexp  =  TLEVec.ibexp;
//		xincl  =  TLEVec.xincl;
//		xnodeo =  TLEVec.xnodeo;
//		eo     =  TLEVec.eo;
//		omegao =  TLEVec.omegao;
//		xmo    =  TLEVec.xmo;
//		xno    =  TLEVec.xno;
//
//	  
//		if (xno <= 0.)
//			throw UnexError( EMS_ORBIT_BAD_TLEXNO );
//
//		xndd6o = xndd6o * pow (10., iexp);
//		xnodeo = xnodeo * DE2RA;
//		omegao = omegao * DE2RA;
//		xmo    = xmo    * DE2RA;
//		xincl  = xincl  * DE2RA;
//		temp   = TWOPI / xmnpda / xmnpda;
//		xno    = xno * temp * xmnpda;
//		xndt2o = xndt2o * temp;
//		xndd6o = xndd6o * temp / xmnpda;
//		bstar  = bstar * pow (10., ibexp) / ae;
//
//		a1     = pow (xke / xno, tothrd);
//		temp   = 1.5 * ck2 * (3. * pow(cos(xincl),2) - 1.) / pow(1.-eo*eo,1.5);
//		del1   = temp / (a1 * a1);
//		ao     = a1 * (1. - del1 * (.5*tothrd + del1*(1.+134./81.*del1)));
//		delo   = temp / (ao * ao);
//		xnodp  = xno / (1. + delo);
//		//
//		//     Input check for period vs ephemeris selected.
//		//     Period >= 225 minutes is deep space.
//		//
//		if ((TWOPI / xnodp / xmnpda) >= .15625)
//			ideep = 1;
//		else
//			ideep = 0;
//
//		// don't do these checks for NOW 2002 Sept 18
//		//     if (ideep EQ 1 AND (iept EQ 1 OR iept EQ 2 OR iept EQ 4))
//		//          throw UnexError( EMS_ORBIT_USEDEEPSPACE );
//		//
//		//      else if (ideep EQ 0 AND (iept EQ 3 OR iept EQ 5))
//		//          throw UnexError( EMS_ORBIT_USENEAREARTH );
//
//		inittime = 0e0;
//		if (iept EQ 1)
//			Sgp (&InitFlag, inittime);
//		else if (iept EQ 2)
//			Sgp4 (&InitFlag, inittime);
//		else if (iept EQ 3)
//			Sdp4 (&InitFlag, inittime);
//		else if (iept EQ 4)
//			Sgp8 (&InitFlag, inittime);
//		else if (iept EQ 5)
//			Sdp8 (&InitFlag, inittime);
//
//	}
}
TINVector TOrbit::Predict ()
{
   double ptime;
   TINVector IVec;

	m_propogatedTLEVec = TLEVec;

   ptime = (double) TLEVec.epoch;           //  default to epoch time
   IVec = Predict( 0.0 );
//   IVec = Predict(ptime);
   return(IVec);
}

TINVector TOrbit::Predict (double tsince)

{
    long double t;

	double ro[3], vo[3];
	m_propogatedTLEVec = TLEVec;

    // Create an instance for the output type
    TINVector INVec;
    t = (long double)(Convert_Epoch (TLEVec.epoch)) + (long double)tsince;

	if (iept EQ 0)
	{
 	    sgp4 (m_whichconst, m_satrec,  tsince, ro,  vo);
		INVec.Rx = ro[0];
		INVec.Ry = ro[1];
		INVec.Rz = ro[2];
		INVec.Vx = vo[0];
		INVec.Vy = vo[1];
		INVec.Vz = vo[2];
		INVec.VTime = t;
	}
//	else
//	{
//		if (iept EQ 1)
//			Sgp (&InitFlag, tsince);
//		else if (iept EQ 2)
//			Sgp4 (&InitFlag, tsince);
//		else if (iept EQ 3)
//			Sdp4 (&InitFlag, tsince);
//		else if (iept EQ 4)
//			Sgp8 (&InitFlag, tsince);
//		else if (iept EQ 5)
//			Sdp8 (&InitFlag, tsince);
// 
//	
//		x = x * xkmper / ae;
//		y = y * xkmper / ae;
//		z = z * xkmper / ae;
//		xdot = xdot * xkmper / ae * xmnpda / 86400.;
//		ydot = ydot * xkmper / ae * xmnpda / 86400.;
//		zdot = zdot * xkmper / ae * xmnpda / 86400.;
//
//		INVec.Rx = x;
//		INVec.Ry = y;
//		INVec.Rz = z;
//		INVec.Vx = xdot;
//		INVec.Vy = ydot;
//		INVec.Vz = zdot;
//		INVec.VTime = t;
//	}
//
   InitFlag = 0;
   return (INVec);
}

TINVector* TOrbit::Predict(double *intime, int NumTimes)
{
  int i;
  double *timepointer;
  TINVector *INPointer;
  TINVector *SavePointer;

	m_propogatedTLEVec = TLEVec;

  // allocate the amount of memory we need to hold the output
  INPointer = new TINVector[NumTimes];     //unsigned char[ObjSize * NewSize];
  SavePointer = INPointer;
  timepointer = intime;
  for (i = 0; i < NumTimes; i++)
     *INPointer++ = Predict(*timepointer++);

  return(SavePointer);
}

