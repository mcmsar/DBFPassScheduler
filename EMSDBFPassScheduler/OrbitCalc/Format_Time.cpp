//---------------------------------------------------------------------------
#include <stdio.h>
#include "SgpxStr.h"
#include "Predict.h"

//---------------------------------------------------------------------------
void Format_Time (double time, char *ptimbf)
{
    int    iyr, imonth, iday, ihour, iminute;
    double t, t1yr, t1month, t1day, second;
    int    ndays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    t = 0e0;
    iyr = 1990;   /* since 1990 */
    if (time >= 0e0)
    {
        t1yr = 365 * 1440e0;
        if (LEAP_YEAR (iyr))
            t1yr += 1440e0;
        while (t + t1yr <= time)
        {
            iyr += 1;
            t += t1yr;
            t1yr = 365 * 1440e0;
            if (LEAP_YEAR (iyr))
                t1yr += 1440e0;
        }
    }
    else
    {
        do
        {
            iyr -= 1;
            t1yr = 365 * 1440e0;
            if (LEAP_YEAR (iyr))
                t1yr += 1440e0;
            t -= t1yr;
        } while (t > time);
    }

    time -= t;
    t = 0e0;
    imonth = 1;     /* January */
    t1month = ndays[imonth-1] * 1440e0;
    while (t + t1month <= time)
    {
        imonth += 1;
        t += t1month;
        t1month = ndays[imonth-1] * 1440e0;
        if (LEAP_YEAR (iyr) AND imonth EQ 2)
            t1month += 1440e0;
    }

    time   -= t;
    t = 0e0;
    iday = 1;       /* 1st day of the month */
    t1day = 1440e0;
    while (t + t1day <= time)
    {
        iday += 1;
        t += t1day;
    }

    time   -= t;
    ihour   = (int)(time / 60e0);
    time   -= ihour * 60e0;
    iminute = (int)time;
    time   -= iminute;
    second = time * 60e0;

    sprintf (ptimbf, "%4d-%02d-%02d %02d:%02d:%05.2lf",
        iyr, imonth, iday, ihour, iminute, second);
}
