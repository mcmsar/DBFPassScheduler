/*
 * Structs and classes
 */
//--------------------------------------------------------------
#ifndef SGPXSTR_H
#define SGPXSTR_H

#include <string.h>
#include "LangResources.h"
#include "sgp4ext.h"    // for several misc routines
#include "sgp4unit.h"   // for sgp4init and getgravconst

class TGSRef	/* Ground station reference information */
   {
   public:
      double lat;
      double lon;
      double alt;

      // constructors
      TGSRef(void)
      {}
   };

class TVector		// vector type
   {
   public:
      double VTime;
      double Rx;
      double Ry;
      double Rz;
      double Vx;
      double Vy;
      double Vz;


      // constructors
      TVector(void)
      { }
  /*
      // constructor which performs initialization
      TVector(double t, double x, double y, double z, double dx, double dy, double dz)
      : VTime(t), RX(x), RY(y), Rz(z), Vx(dx), Vy(dy), Vz(dz)
      {
      }
      */
   };

class TINVector : public TVector	// Inertial coordinates
   {
   };



class TEFVector : public TVector	// Earth Fixed coordinates
   {
   };

class TGSVector			// Ground station vector type
   {
   public:
      double Rx;
      double Ry;
      double Rz;
      double Az;
      double El;
      double RGS;
      double VGS;

      // constructors
      TGSVector(void)
      { }

      // constructor which performs initialization
 /*     TGSVector(double x, double y, double z, double a, double e, double r, double v)
      : RX(x), RY(y), Rz(z), Az(a), El(e), RGS(r), VGS(v)
      {
      }
*/
   };

class TCLVector		// Classical vector type
   {
   public:
      double VTime;
      double SMA;
      double ECC;
      double AINC;
      double ANOD;
      double PERI;
      double ANOM;
      double PERIOD;

      // constructors
      TCLVector(void)
      { }

      // constructor which performs initialization
      TCLVector(double t, double s, double e, double ain, double an, double p, double m, double per)
      : VTime(t), SMA(s), ECC(e), AINC(ain), ANOD(an), PERI(p), ANOM(m), PERIOD(per)
      {
      }
   };

class TTLEVector		// two line elements data from NORAD
   {
   public:
      double epoch;     // last two digits of year, day of year, fractional portion of day)
      double xndt2o;  	// First time derivative of the Mean Motion
      double xndd6o;  	// Second time derivative of Mean motion
      int iexp; 	//
      double bstar;   	// BSTAR drag term
      int ibexp;
      double xincl; 	// Inclination (degs)
      double xnodeo;  	// Right ascension of the ascending node (degs)
      double eo;	    	// eccentricity
      double omegao;  	// argument of perigee (degs)
      double xmo;	// mean anomaly (degs)
      double xno;	// mean motion (degs)
      int norbt;	// revolution number at epoch (revs)

      //constructors
      TTLEVector(void)
      {}
   };


class TOrbit            // orbit prediction returns IN coordinates
		                        // as the default type
   {
   private:
      int InitFlag;
      TTLEVector TLEVec, m_propogatedTLEVec;
      int iept;         // propogation type
      
	  struct elsetrec m_satrec;
	  gravconsttype m_whichconst;

	  // old global variables are moved into the class
	  double epoch, ds50;
      double  xmo, xnodeo, omegao, eo, xincl, xno, xndt2o, xndd6o, bstar,
      x, y, z, xdot, ydot, zdot;
      double  ck2, ck4, e6a, qoms2t, s, tothrd,
      xj3, xke, xkmper, xmnpda, ae;

      void Sgp  (int*, double);
      void Sgp4 (int*, double);
      void Sdp4 (int*, double);
      void Sgp8 (int*, double);
      void Sdp8 (int*, double);
      void Deep (int iflg, double eqsq, double siniq, double cosiq, double rteqsq, double ao, double cosq2,
           double sinomo, double cosomo, double bsq, double xlldot, double omgdt, double xnodot, double xnodp,
           double *pxll, double *pomgasm, double *pxnodes, double *pem, double *pxinc, double *pxn, double t);
      double Thetag (double ep);

   public:

// *** public UnexError.message defined as char array.  Cannot find code that uses this so cannot convert. Will not support all languages.
      class UnexError              // exception class
         {
         public:
           char message[150];

           UnexError(DWORD msgID)
           {
			   if ( SUCCEEDED( CEMSLangResources::Init( EMSAPP_MSGTABLE ) ) )
			   {
					strncpy( message, CEMSLangResources::GetStringA( msgID ).c_str(), 149 );
			   }
			   else
			   {
				   sprintf( message, "Unexpected error (%d)", msgID );
			   }
		   }
           UnexError(char* msg)
           {
				strcpy(message, msg); 
           }
         };
   
      // constructor - performs initialization
      TOrbit(TTLEVector *TVec, int ept);

         // predict to epoch time
      TINVector Predict();

      // predict to time = timval
      TINVector Predict(double timeval);

      // predict to array of times NumTimes long
      TINVector* Predict(double *intime, int NumTimes);

		TTLEVector GetPropogatedTLE( void ) { return m_propogatedTLEVec; } 

     };

class TConvert
    {
    protected:
        TConvert() {}
    public :


   static TCLVector ToCL(TINVector *Ivec);	// from inertial to CL
   static TCLVector* ToCL(TINVector *Ivec, int NumVec);

//   static TCLVector ToCL(TEFVector *Ivec);    // from earth fixed to CL
//   static TCLVector* ToCL(TEFVector *Ivec, int NumVec);

//   static TCLVector ToCL(TGSVector *Ivec);    // From ground station to CL
//   static TCLVector* ToCL(TGSVector *Ivec, int NumVec);

   static TEFVector ToEF(TINVector *Ivec);	// from inertial to EF
   static TEFVector* ToEF(TINVector *Ivec, int NumVec);

//   static TEFVector ToEF(TCLVector *Ivec);	// from classical to EF
//   static TEFVector* ToEF(TCLVector *Ivec, int NumVec);

//   static TEFVector ToEF(TGSVector *Ivec);	// from ground station to EF
//   static TEFVector* ToEF(TGSVector *Ivec, int NumVec);

   static TGSVector ToGS(TINVector *Ivec, TGSRef GS);    	// from inertial to GS
   static TGSVector* ToGS(TINVector *Ivec, TGSRef GS, int NumVec);

//   static TGSVector ToGS(TCLVector *Ivec, TGSRef GS);	// from classical to GS
//   static TGSVector ToGS(TCLVector *Ivec, TGSRef GS, int NumVec);

//   static TGSVector ToGS(TEFVector *Ivec, TGSRef GS);	// from earth fixed to GS
//   static TGSVector ToGS(TEFVector *Ivec, TGSRef GS, int NumVec);

   };

   #endif
