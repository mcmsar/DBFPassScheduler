//---------------------------------------------------------------------------
#include <math.h>
#include "SgpxStr.h"
#include "Predict.h"

#pragma warning ( disable : 4244 )

//---------------------------------------------------------------------------

double Actan (double sinx, double cosx)
{
    double res, temp;

    if (cosx EQ 0.0)
    {
        if (sinx EQ 0.0)
            res = 0.0;
        else if (sinx > 0.0)
            res = (double)PIO2;
        else
            res = (double)X3PIO2;
    }
    else if (cosx > 0.0)
    {
        if (sinx >= 0.0)
        {
            temp = sinx / cosx;
            res  = atan (temp);
        }
        else
        {
            temp = sinx / cosx;
            res  = TWOPI + atan (temp);
        }
    }
    else
    {
        temp = sinx / cosx;
        res  = PI + atan (temp);
    }

    return (res);
}
