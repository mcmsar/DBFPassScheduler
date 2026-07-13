//---------------------------------------------------------------------------
#include "SgpxStr.h"
#include "Predict.h"

//---------------------------------------------------------------------------
long double Kelso_Gst (long double t)
{
    long double th, ut, tu, gmst, ldres;

    th    = t / 1440.0;     /* in days from 1990 */
    ut = DMOD (th, 1.0);    /* Hours (in days) */
    th = th - ut;           /* Days (in days) */
    tu = (th - 3652.5) / 36525.0; /* Since Jan 1, 2000 12:00 UTC */
    gmst = 24110.54841 + tu * (8640184.812866 + tu * (0.093104 - tu * 6.2E-6));  /* 1992 Astronomical Almanac, page B6 */
                                                                                 /* (gives THETAg(0h) */
    gmst = gmst + 86400.0*1.00273790934*ut;  /* Add the hours */
                                             /* 86400*W/PI2=1.00273790934 */
                                             /* (Earth's rotation rate: w=7.29211510E-5 rad/sec) */
    gmst = DMOD (gmst, 86400.0);    /* GST in secs */
    if (gmst < 0.0)
        gmst = gmst + 86400.0;

    ldres = PI * 2.0 * gmst / 86400.0;    /* GST in rads */

    return (ldres);     /* in radians */
}
