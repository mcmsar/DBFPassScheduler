//---------------------------------------------------------------------------
#include <math.h>
#include "SgpxStr.h"
#include "Predict.h"

//---------------------------------------------------------------------------
long double Dnuc (long double t)
{
    long double t80, td, ldres;
    double      f, d, w;

    t80 = (t + TDELTA80) * 60.0;    /* in seconds from 1980 */
    td  = t80/86400.0 + 29219.0;
    f     = DMOD ( 11.25089 + 13.229350449*td, 360.0)/RDG;
    d     = DMOD (350.73749 + 12.190749191*td, 360.0)/RDG;
    w     = DMOD (259.18328 -  0.052953922*td, 360.0)/RDG;
    ldres = (-17.2327*sin(w) - 1.2729*sin(2.0*(f + w - d)) +
             0.2088*sin(2.0*w) - 0.2037*sin(2.0*(f + w))) / 3600.0;
    w     = OBLIQ / RDG;
    ldres = ldres * cos (w);

    return (ldres);     /* in degrees */
}
