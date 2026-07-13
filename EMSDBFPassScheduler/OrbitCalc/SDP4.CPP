/*
 *      SDP4                                               3 NOV 80
 */

//---------------------------------------------------------------------------
#include <math.h>
#include "SgpxStr.h"
#include "Predict.h"

#pragma warning ( disable : 4244 )

//---------------------------------------------------------------------------
void TOrbit::Sdp4 (int *piflag, double tsince)
{
    static double a1, cosio, theta2, x3thm1, eosq, betao2, betao, del1, ao, 
          delo, xnodp, aodp, s4, qoms24, perige, pinvsq, sing, cosg, tsi, 
          eta, etasq, eeta, psisq, coef, coef1, c2, c1, sinio, a3ovk2, 
          x1mth2, c4, theta4, temp1, temp2, temp3, xmdot, x1m5th, omgdot, 
          xhdot1, xnodot, xnodcf, t2cof, xlcof, aycof, x7thm1, xnoddf, tsq, 
          tempa, tempe, templ, a, e, xmam, xl, beta, axn, temp, xll, aynl, 
          xlt, ayn, capu, sinepw, cosepw, temp4, temp5, temp6, epw, 
          ecose, esine, elsq, pl, r, rdot, rfdot, betal, cosu, sinu, u, 
          sin2u, cos2u, rk, uk, xnodek, xinck, rdotk, rfdotk, sinuk, cosuk, 
          sinik, cosik, sinnok, cosnok, xmx, xmy, ux, uy, uz, vx, vy, vz;
    static double xmdf, omgadf, xnode, em, xinc, xn;
    int   i;

    if  (*piflag EQ 0) goto L100;

/*
 *  Recover original mean motion (xnodp) and semimajor axis (aodp)
 *  from input elements.
 */
    a1=pow(xke/xno,tothrd);
    cosio=cos(xincl);
    theta2=cosio*cosio;
    x3thm1=3.*theta2-1.;
    eosq=eo*eo;
    betao2=1.-eosq;
    betao=sqrt(betao2);
    del1=1.5*ck2*x3thm1/(a1*a1*betao*betao2);
    ao=a1*(1.-del1*(.5*tothrd+del1*(1.+134./81.*del1)));
    delo=1.5*ck2*x3thm1/(ao*ao*betao*betao2);
    xnodp=xno/(1.+delo);
    aodp=ao/(1.-delo);

/*
 *  Initialization.
 *
 *  For perigee below 156 km, the values of s and qoms2t are altered.
 */
    s4=s;
    qoms24=qoms2t;
    perige=(aodp*(1.-eo)-ae)*xkmper;
    if(perige >= 156.) goto L10;
    s4=perige-78.;
    if(perige > 98.) goto L9;
    s4=20.;
L9: qoms24=pow((120.-s4)*ae/xkmper,4);
    s4=s4/xkmper+ae;
L10: pinvsq=1./(aodp*aodp*betao2*betao2);
    sing=sin(omegao);
    cosg=cos(omegao);
    tsi=1./(aodp-s4);
    eta=aodp*eo*tsi;
    etasq=eta*eta;
    eeta=eo*eta;
    psisq=fabs(1.-etasq);
    coef=qoms24*pow(tsi,4);
    coef1=coef/pow(psisq,3.5);
    c2=coef1*xnodp*(aodp*(1.+1.5*etasq+eeta*(4.+etasq))+.75*
             ck2*tsi/psisq*x3thm1*(8.+3.*etasq*(8.+etasq)));
    c1=bstar*c2;
    sinio=sin(xincl);
    a3ovk2=-xj3/ck2*pow(ae,3);
    x1mth2=1.-theta2;
    c4=2.*xnodp*coef1*aodp*betao2*(eta*
             (2.+.5*etasq)+eo*(.5+2.*etasq)-2.*ck2*tsi/
             (aodp*psisq)*(-3.*x3thm1*(1.-2.*eeta+etasq*
             (1.5-.5*eeta))+.75*x1mth2*(2.*etasq-eeta*
             (1.+etasq))*cos(2.*omegao)));
    theta4=theta2*theta2;
    temp1=3.*ck2*pinvsq*xnodp;
    temp2=temp1*ck2*pinvsq;
    temp3=1.25*ck4*pinvsq*pinvsq*xnodp;
    xmdot=xnodp+.5*temp1*betao*x3thm1+.0625*temp2*betao*
             (13.-78.*theta2+137.*theta4);
    x1m5th=1.-5.*theta2;
    omgdot=-.5*temp1*x1m5th+.0625*temp2*(7.-114.*theta2+
             395.*theta4)+temp3*(3.-36.*theta2+49.*theta4);
    xhdot1=-temp1*cosio;
    xnodot=xhdot1+(.5*temp2*(4.-19.*theta2)+2.*temp3*(3.-
             7.*theta2))*cosio;
    xnodcf=3.5*betao2*xhdot1*c1;
    t2cof=1.5*c1;
    xlcof=.125*a3ovk2*sinio*(3.+5.*cosio)/(1.+cosio);
    aycof=.25*a3ovk2*sinio;
    x7thm1=7.*theta2-1.;
    *piflag=0;
/*    CALL DPINIT(EOSQ,SINIO,COSIO,BETAO,AODP,THETA2,
     1          SING,COSG,BETAO2,XMDOT,OMGDOT,XNODOT,XNODP) */
    Deep (1,eosq,sinio,cosio,betao,aodp,theta2,
          sing,cosg,betao2,xmdot,omgdot,xnodot,xnodp,
          &xmdf,&omgadf,&xnode,&em,&xinc,&xn,tsince);

/*
 *  Update for secular gravity and atmospheric drag.
 */
L100: xmdf=xmo+xmdot*tsince;
    omgadf=omegao+omgdot*tsince;
    xnoddf=xnodeo+xnodot*tsince;
    tsq=tsince*tsince;
    xnode=xnoddf+xnodcf*tsq;
    tempa=1.-c1*tsince;
    tempe=bstar*c4*tsince;
    templ=t2cof*tsq;
    xn=xnodp;
/*    CALL DPSEC(XMDF,OMGADF,XNODE,EM,XINC,XN,TSINCE,OMGDOT) */
    Deep (2,eosq,sinio,cosio,betao,aodp,theta2,
          sing,cosg,betao2,xmdot,omgdot,xnodot,xnodp,
          &xmdf,&omgadf,&xnode,&em,&xinc,&xn,tsince);
    a=pow(xke/xn,tothrd)*pow(tempa,2);
    e=em-tempe;
    xmam=xmdf+xnodp*templ;
/*    CALL DPPER(E,XINC,OMGADF,XNODE,XMAM,SINIO,COSIO,TSINCE) */
    Deep (3,eosq,sinio,cosio,betao,aodp,theta2,
          sing,cosg,betao2,xmdot,omgdot,xnodot,xnodp,
          &xmam,&omgadf,&xnode,&e,&xinc,&xn,tsince);
    xl=xmam+omgadf+xnode;
    beta=sqrt(1.-e*e);
    xn=xke/pow(a,1.5);

/*
 *  Long period periodics.
 */
    axn=e*cos(omgadf);
    temp=1./(a*beta*beta);
    xll=temp*xlcof*axn;
    aynl=temp*aycof;
    xlt=xl+xll;
    ayn=e*sin(omgadf)+aynl;

/*
 *  Solve Kepler's equation.
 */
    capu=Fmod2p(xlt-xnode);
    temp2=capu;
    for (i = 0; i < 10; i++)
    {
        sinepw=sin(temp2);
        cosepw=cos(temp2);
        temp3=axn*sinepw;
        temp4=ayn*cosepw;
        temp5=axn*cosepw;
        temp6=ayn*sinepw;
        epw=(capu-temp4+temp3-temp2)/(1.-temp5-temp6)+temp2;
        if(fabs(epw-temp2) <= e6a) goto L140;
        temp2=epw;
    }

/*
 *  Short period preliminary quantities.
 */
L140: ecose=temp5+temp6;
    esine=temp3-temp4;
    elsq=axn*axn+ayn*ayn;
    temp=1.-elsq;
    pl=a*temp;
    r=a*(1.-ecose);
    temp1=1./r;
    rdot=xke*sqrt(a)*esine*temp1;
    rfdot=xke*sqrt(pl)*temp1;
    temp2=a*temp1;
    betal=sqrt(temp);
    temp3=1./(1.+betal);
    cosu=temp2*(cosepw-axn+ayn*esine*temp3);
    sinu=temp2*(sinepw-ayn-axn*esine*temp3);
    u=Actan(sinu,cosu);
    sin2u=2.*sinu*cosu;
    cos2u=2.*cosu*cosu-1.;
    temp=1./pl;
    temp1=ck2*temp;
    temp2=temp1*temp;

/*
 *  Update for short periodics.
 */
    rk=r*(1.-1.5*temp2*betal*x3thm1)+.5*temp1*x1mth2*cos2u;
    uk=u-.25*temp2*x7thm1*sin2u;
    xnodek=xnode+1.5*temp2*cosio*sin2u;
    xinck=xinc+1.5*temp2*cosio*sinio*cos2u;
    rdotk=rdot-xn*temp1*x1mth2*sin2u;
    rfdotk=rfdot+xn*temp1*(x1mth2*cos2u+1.5*x3thm1);

/*
 *  Orientation vectors.
 */
    sinuk=sin(uk);
    cosuk=cos(uk);
    sinik=sin(xinck);
    cosik=cos(xinck);
    sinnok=sin(xnodek);
    cosnok=cos(xnodek);
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
    xdot=rdotk*ux+rfdotk*vx;
    ydot=rdotk*uy+rfdotk*vy;
    zdot=rdotk*uz+rfdotk*vz;
}
