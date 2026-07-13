#ifndef TORbit_H
#define TORbit_H

#define EQ     ==
#define AND    &&
#define OR     ||

#define DMOD(a,b)      ((a) - (long int)((a)/(b)) * (b))
#define SIGN(a,b)      ((b) >= 0 ? fabs (a) : -fabs (a))
#define LEAP_YEAR(iyr) ((iyr) % 4 EQ 0 AND ((iyr) % 100 != 0 OR (iyr) % 400 EQ 0))

#define PI     3.14159265
#define PIO2   1.57079633
#define TWOPI  6.2831853
#define X3PIO2 4.71238898
#define DE2RA   .174532925e-1
#define RHO     .15696615

#define TDELTA80  5260320.0    /* 1980 - 1990 period in minutes */
#define OBLIQ        23.442    /* Inclination of Equator over Earth's orbital plane (degs) */
#define RDG 57.295779513  /* Degrees / radian */
#define RE  6378.135      /* Earth's radius (km) */
#define E2  0.00669438002 /* oblateness**2 of Earth */
#define J2  -1082.63e-6   /* 2nd Tesserel harmonic coefficient */
#define SID 86164.09054   /* Sidereal day (secs) */
#define SOL 86636.55536   /* Solar day (secs) */
#define MU  3.986005e5    /* Gravitation constant (km**3/sec**2) */


/*
 * Prototypes.
 */

TINVector  Predict (double tsince);
double  Convert_Epoch (double ep);
double  JD_Value (char *ptimbf);
void  Format_Time (double time, char *ptimbf);


void  Sgp  (int *piflag, double tsince);
void  Sgp4 (int *piflag, double tsince);
void  Sdp4 (int *piflag, double tsince);
void  Sgp8 (int *piflag, double tsince);
void  Sdp8 (int *piflag, double tsince);
void  Deep (int iflg, double eqsq, double siniq, double cosiq, double rteqsq, double ao, double cosq2,
double sinomo, double cosomo, double bsq, double xlldot, double omgdt, double xnodot, double xnodp,
double *pxll, double *pomgasm, double *pxnodes, double *pem, double *pxinc, double *pxn, double t);
double Actan (double sinx, double cosx);
double Fmod2p (double x);
double Thetag (double ep);
long double Dnuc (long double t);


int  Kelso_DOY (int iyear, int imonth, int iday);
long double Gst (long double t);
long double Dnuc (long double t);
long double Kelso_Gst (long double t);

#endif
