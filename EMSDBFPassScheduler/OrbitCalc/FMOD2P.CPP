//---------------------------------------------------------------------------
#include "SgpxStr.h"
#include "Predict.h"

#pragma warning ( disable : 4244 )

//---------------------------------------------------------------------------
double Fmod2p (double x)
{
    double res;
    int i;

    res = x;
    i = res / TWOPI;
    res = res - i*TWOPI;
    if (res < 0.0)
        res = res + TWOPI;

    return (res);
}
