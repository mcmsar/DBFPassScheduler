//---------------------------------------------------------------------------
#include <stdio.h>
#include "SgpxStr.h"
#include "Predict.h"

//---------------------------------------------------------------------------
double JD_Value (char *ptimbf)
{
    int    iyear, imonth, iday, ihour, iminute, idoy;
    double second, fdays, dres;

    sscanf (&ptimbf[ 0], "%4d",  &iyear  );
    sscanf (&ptimbf[ 5], "%2d",  &imonth );
    sscanf (&ptimbf[ 8], "%2d",  &iday   );
    sscanf (&ptimbf[11], "%2d",  &ihour  );
    sscanf (&ptimbf[14], "%2d",  &iminute);
    sscanf (&ptimbf[17], "%5lf", &second );

    idoy  = Kelso_DOY (iyear, imonth, iday);
    fdays = (double)ihour/24.0 + (double)iminute/1440.0 + second/86400.0;

    dres  = (double)(iyear%100)* 1000.0 + (double)idoy + fdays;

    return (dres);
}



int Kelso_DOY (int iyear, int imonth, int iday)
{
    int days_array[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int day, i, ires;

    day = 0;
    for (i = 0; i < imonth-1; i++)
        day += days_array[i];
    day += iday;

    if (LEAP_YEAR (iyear) AND imonth > 2)
        day += 1;

    ires = day;

    return (ires);
}
