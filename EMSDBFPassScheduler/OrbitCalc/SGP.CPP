/*
 *      SGP                                             31 OCT 80
 */

//---------------------------------------------------------------------------
#include <math.h>
#include "SgpxStr.h"
#include "Predict.h"

#pragma warning ( disable : 4244 )

//---------------------------------------------------------------------------

void TOrbit::Sgp (int *piflag, double tsince)
{
    static double c1, c2, c3, c4, cosio, sinio, a1, d1, ao, po, qo, xlo,
          d1o, d2o, d3o, d4o, po2no, omgdt, xnodot, c5, c6, a, e, p,
          xnodes, omgas, xls, axnsl, aynsl, xl, u, eo1, tem5, sineo1, coseo1,
          tem2, ecose, esine, el2, pl, pl2, r, rdot, rvdot, temp,
          sinu, cosu, su, sin2u, cos2u, rk, uk, xnodek, xinck, sinuk, cosuk,
          sinnok, cosnok, sinik, cosik, xmx, xmy, ux, uy, uz, vx, vy, vz;
    int   item3;

    if(*piflag EQ 0) goto L19;

/*
 *  Initialization.
 */
    c1= ck2*1.5;
    c2= ck2/4.0;
    c3= ck2/2.0;
    c4= xj3*pow(ae,3)/(4.0*ck2);
    cosio=cos(xincl);
    sinio=sin(xincl);
    a1=pow(xke/xno,tothrd);
    d1= c1/a1/a1*(3.*cosio*cosio-1.)/pow(1.-eo*eo,1.5);
    ao=a1*(1.-1./3.*d1-d1*d1-134./81.*d1*d1*d1);
    po=ao*(1.-eo*eo);
    qo=ao*(1.-eo);
    xlo=xmo+omegao+xnodeo;
    d1o= c3 *sinio*sinio;
    d2o= c2 *(7.*cosio*cosio-1.);
    d3o=c1*cosio;
    d4o=d3o*sinio;
    po2no=xno/(po*po);
    omgdt=c1*po2no*(5.*cosio*cosio-1.);
    xnodot=-2.*d3o*po2no;
    c5=.5*c4*sinio*(3.+5.*cosio)/(1.+cosio);
    c6=c4*sinio;
    *piflag=0;

/*
 *  Update for secular gravity and atmospheric drag.
 */
L19: a=xno+(2.*xndt2o+3.*xndd6o*tsince)*tsince;
    a=ao*pow(xno/a,tothrd);
    e=e6a;
    if(a > qo) e=1.-qo/a;
    p=a*(1.-e*e);
    xnodes= xnodeo+xnodot*tsince;
    omgas= omegao+omgdt*tsince;
    xls=Fmod2p(xlo+(xno+omgdt+xnodot+(xndt2o+xndd6o*tsince)*tsince)*tsince);

/*
 *  Long period periodics.
 */
    axnsl=e*cos(omgas);
    aynsl=e*sin(omgas)-c6/p;
    xl=Fmod2p(xls-c5/p*axnsl);

/*
 *  Solve Kepler's equation.
 */
    u=Fmod2p(xl-xnodes);
    item3=0;
    eo1=u;
    tem5=1.;
L20: sineo1=sin(eo1);
    coseo1=cos(eo1);
    if(fabs(tem5) < e6a) goto L30;
    if(item3 >= 10) goto L30;
    item3=item3+1;
    tem5=1.-coseo1*axnsl-sineo1*aynsl;
    tem5=(u-aynsl*coseo1+axnsl*sineo1-eo1)/tem5;
    tem2=fabs(tem5);
    if(tem2 > 1.) tem5=tem2/tem5;
    eo1=eo1+tem5;
    goto L20;

/*
 *  Short period preliminary quantities.
 */
L30: ecose=axnsl*coseo1+aynsl*sineo1;
    esine=axnsl*sineo1-aynsl*coseo1;
    el2=axnsl*axnsl+aynsl*aynsl;
    pl=a*(1.-el2);
    pl2=pl*pl;
    r=a*(1.-ecose);
    rdot=xke*sqrt(a)/r*esine;
    rvdot=xke*sqrt(pl)/r;
    temp=esine/(1.+sqrt(1.-el2));
    sinu=a/r*(sineo1-aynsl-axnsl*temp);
    cosu=a/r*(coseo1-axnsl+aynsl*temp);
    su=Actan(sinu,cosu);

/*
 *  Update for short periodics.
 */
    sin2u=(cosu+cosu)*sinu;
    cos2u=1.-2.*sinu*sinu;
    rk=r+d1o/pl*cos2u;
    uk=su-d2o/pl2*sin2u;
    xnodek=xnodes+d3o*sin2u/pl2;
    xinck =xincl+d4o/pl2*cos2u;

/*
 *  Orientation vectors.
 */
    sinuk=sin(uk);
    cosuk=cos(uk);
    sinnok=sin(xnodek);
    cosnok=cos(xnodek);
    sinik=sin(xinck);
    cosik=cos(xinck);
    xmx=-sinnok*cosik;
    xmy=cosnok*cosik;
    ux=xmx*sinuk+cosnok*cosuk;
    uy=xmy*sinuk+sinnok*cosuk;
    uz=sinik*sinuk;
    vx=xmx*cosuk-cosnok*sinuk;
    vy=xmy*cosuk-sinnok*sinuk;
    vz=sinik*cosuk;

/*
 *  Position and Velocity.
 */
    x=rk*ux;
    y=rk*uy;
    z=rk*uz;
    xdot=rdot*ux;
    ydot=rdot*uy;
    zdot=rdot*uz;
    xdot=rvdot*vx+xdot;
    ydot=rvdot*vy+ydot;
    zdot=rvdot*vz+zdot;
}
