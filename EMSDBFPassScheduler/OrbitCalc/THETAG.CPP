//---------------------------------------------------------------------------
#include "SgpxStr.h"
#include "Predict.h"

#pragma warning ( disable : 4244 )

//---------------------------------------------------------------------------

double TOrbit::Thetag (double ep)
{
    double d, theta, twopi, yr, temp;
    int    jy, n, i;
    double  res;

    twopi = 6.28318530717959e0;
    yr = (ep + 2.e-7) * 1.e-3;
    jy = yr;
    yr = jy;
    d  = ep - yr * 1.e3;
    if (jy < 10)
        jy = jy + 80;
    n = (jy - 69) / 4;
    if (jy < 70)
        n = (jy - 72) / 4;
    ds50  = 7305.e0 + 365.e0 * (jy - 70) + n + d;
    theta = 1.72944494e0 + 6.3003880987e0 * ds50;
    temp  = theta / twopi;
    i     = temp;
    temp  = i;
    res   = theta - temp * twopi;
    if (res < 0.e0)
        res = res + twopi;

    return (res);
}
