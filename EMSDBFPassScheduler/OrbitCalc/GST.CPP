//---------------------------------------------------------------------------
#include "SgpxStr.h"
#include "Predict.h"

//---------------------------------------------------------------------------
long double Gst (long double t)
{
    long double t80, td, tc, ldres;

    t80   = (t + TDELTA80) * 60.0;    /* in seconds from 1980 */
    td    = t80 / 86400.0;
    tc    = td / 36525.0;
    ldres = DMOD (99.8135 + 360.0*(SOL/86400.0)*td + 0.000387*tc*tc + Dnuc (t), 360.0);

    ldres = ldres * PI / 180.0;

    return (ldres);     /* in radians */
}
