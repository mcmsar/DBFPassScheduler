//---------------------------------------------------------------------------
#include "SgpxStr.h"
#include "Predict.h"

//---------------------------------------------------------------------------
double Convert_Epoch (double ep)
{
    int iyear, iyr, idays;
    double days,dres;

    iyear = (int)(ep / 1000.0);
    days = ep - iyear*1000.0 - 1;
    if (iyear >= 80)
        iyear += 1900;
    else
        iyear += 2000;

    iyr = 1990;   /* since 1990 */
    idays = 0;
    if (iyear >= 1990)
    {
        while (iyr < iyear)
        {
            idays += 365;
            if (LEAP_YEAR (iyr))
                idays += 1;
            iyr   += 1;
        }
    }
    else
    {
        while (iyr > iyear)
        {
            iyr   -= 1;
            idays -= 365;
            if (LEAP_YEAR (iyr))
                idays -= 1;
        }
    }

    dres = (idays + days) * 1440.0;

    return (dres);
}
