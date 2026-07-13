/*
 *      SDP8                                              14 NOV 80
 */

//---------------------------------------------------------------------------
#include <math.h>
#include "SgpxStr.h"
#include "Predict.h"

#pragma warning ( disable : 4244 )

//---------------------------------------------------------------------------
void TOrbit::Sdp8 (int *piflag, double tsince)
{
    static double a1, cosi, theta2, tthmun, eosq, betao2, betao, del1, ao, 
          delo, aodp, xnodp, b, po, pom2, sini, sing, cosg, temp, 
          sinio2, cosio2, theta4, unm5th, unmth2, a3cof, pardt1, pardt2, 
          pardt4, xmdt1, xgdt1, xhdt1, xlldot, omgdt, xnodot, tsi, eta, eta2, 
          psim2, alpha2, eeta, cos2g, d5, d1, d2, d3, d4, b1, b2, b3, 
          c0, c1, c4, c5, xndt, xndtn, edot, z1, z7, xmam, zc2, sine, cose, 
          zc5, cape, am, beta2m, sinos, cosos, axnm, aynm, pm, g1, g2, g3, 
          beta, g4, g5, snf, csf, fm, snfg, csfg, sn2f2g, cs2f2g, ecosf, g10, 
          rm , aovr, g13, g14, dr, diwc, di, sini2, sni2du, xlamb, y4, y5, 
          r, rdot, rvdot, snlamb, cslamb, ux, vx, uy, vy, uz, vz;
    static double xmamdf, omgasm, xnodes, em, xinc, xn;
    int   i;

    if (*piflag EQ 0) goto L100;

/*
 *  Recover original mean motion (xnodp) and semimajor axis (aodp)
 *  from input elements --------- Calculate ballistic coefficient
 *  (b term) from input b* drag term.
 */
    a1=pow(xke/xno,tothrd);
    cosi=cos(xincl);
    theta2=cosi*cosi;
    tthmun=3.*theta2-1.;
    eosq=eo*eo;
    betao2=1.-eosq;
    betao=sqrt(betao2);
    del1=1.5*ck2*tthmun/(a1*a1*betao*betao2);
    ao=a1*(1.-del1*(.5*tothrd+del1*(1.+134./81.*del1)));
    delo=1.5*ck2*tthmun/(ao*ao*betao*betao2);
    aodp=ao/(1.-delo);
    xnodp=xno/(1.+delo);
    b=2.*bstar/RHO;

/*
 *  Initialization.
 */
    po=aodp*betao2;
    pom2=1./(po*po);
    sini=sin(xincl);
    sing=sin(omegao);
    cosg=cos(omegao);
    temp=.5*xincl;
    sinio2=sin(temp);
    cosio2=cos(temp);
    theta4=pow(theta2,2);
    unm5th=1.-5.*theta2;
    unmth2=1.-theta2;
    a3cof=-xj3/ck2*pow(ae,3);
    pardt1=3.*ck2*pom2*xnodp;
    pardt2=pardt1*ck2*pom2;
    pardt4=1.25*ck4*pom2*pom2*xnodp;
    xmdt1=.5*pardt1*betao*tthmun;
    xgdt1=-.5*pardt1*unm5th;
    xhdt1=-pardt1*cosi;
    xlldot=xnodp+xmdt1+
               .0625*pardt2*betao*(13.-78.*theta2+137.*theta4);
    omgdt=xgdt1+
          .0625*pardt2*(7.-114.*theta2+395.*theta4)+pardt4*(3.-36.*
             theta2+49.*theta4);
    xnodot=xhdt1+
           (.5*pardt2*(4.-19.*theta2)+2.*pardt4*(3.-7.*theta2))*cosi;
    tsi=1./(po-s);
    eta=eo*s*tsi;
    eta2=pow(eta,2);
    psim2=fabs(1./(1.-eta2));
    alpha2=1.+eosq;
    eeta=eo*eta;
    cos2g=2.*pow(cosg,2)-1.;
    d5=tsi*psim2;
    d1=d5/po;
    d2=12.+eta2*(36.+4.5*eta2);
    d3=eta2*(15.+2.5*eta2);
    d4=eta*(5.+3.75*eta2);
    b1=ck2*tthmun;
    b2=-ck2*unmth2;
    b3=a3cof*sini;
    c0=.5*b*RHO*qoms2t*xnodp*aodp*pow(tsi,4)*pow(psim2,3.5)/sqrt(alpha2);
    c1=1.5*xnodp*pow(alpha2,2)*c0;
    c4=d1*d3*b2;
    c5=d5*d4*b3;
    xndt=c1*(
      (2.+eta2*(3.+34.*eosq)+5.*eeta*(4.+eta2)+8.5*eosq)+
       d1*d2*b1+   c4*cos2g+c5*sing);
    xndtn=xndt/xnodp;
    edot=-tothrd*xndtn*(1.-eo);
    *piflag=0;
/*    CALL DPINIT(EOSQ,SINI,COSI,BETAO,AODP,THETA2,SING,COSG,
   1          BETAO2,XLLDOT,OMGDT,XNODOT,XNODP) */
    Deep (1,eosq,sini,cosi,betao,aodp,theta2,
          sing,cosg,betao2,xlldot,omgdt,xnodot,xnodp,
          &xmamdf,&omgasm,&xnodes,&em,&xinc,&xn,tsince);

/*
 *  Update for secular gravity and atmospheric drag.
 */
L100: z1=.5*xndt*tsince*tsince;
    z7=3.5*tothrd*z1/xnodp;
    xmamdf=xmo+xlldot*tsince;
    omgasm=omegao+omgdt*tsince+z7*xgdt1;
    xnodes=xnodeo+xnodot*tsince+z7*xhdt1;
    xn=xnodp;
/*    CALL DPSEC(XMAMDF,OMGASM,XNODES,EM,XINC,XN,TSINCE,OMGDT) */
    Deep (2,eosq,sini,cosi,betao,aodp,theta2,
          sing,cosg,betao2,xlldot,omgdt,xnodot,xnodp,
          &xmamdf,&omgasm,&xnodes,&em,&xinc,&xn,tsince);
    xn=xn+xndt*tsince;
    em=em+edot*tsince;
    xmam=xmamdf+z1+z7*xmdt1;
/*    CALL DPPER(EM,XINC,OMGASM,XNODES,XMAM,SINI,COSI,TSINCE) */
    Deep (3,eosq,sini,cosi,betao,aodp,theta2,
          sing,cosg,betao2,xlldot,omgdt,xnodot,xnodp,
          &xmam,&omgasm,&xnodes,&em,&xinc,&xn,tsince);
    xmam=Fmod2p(xmam);

/*
 *  Solve Kepler's equation.
 */
    zc2=xmam+em*sin(xmam)*(1.+em*cos(xmam));
    for (i = 0; i < 10; i++)
    {
        sine=sin(zc2);
        cose=cos(zc2);
        zc5=1./(1.-em*cose);
        cape=(xmam+em*sine-zc2)*zc5+zc2;
        if(fabs(cape-zc2) <= e6a) goto L140;
        zc2=cape;
    }

/*
 *  Short period preliminary quantities.
 */
L140: am=pow(xke/xn,tothrd);
    beta2m=1.-em*em;
    sinos=sin(omgasm);
    cosos=cos(omgasm);
    axnm=em*cosos;
    aynm=em*sinos;
    pm=am*beta2m;
    g1=1./pm;
    g2=.5*ck2*g1;
    g3=g2*g1;
    beta=sqrt(beta2m);
    g4=.25*a3cof*sini;
    g5=.25*a3cof*g1;
    snf=beta*sine*zc5;
    csf=(cose-em)*zc5;
    fm=Actan(snf,csf);
    snfg=snf*cosos+csf*sinos;
    csfg=csf*cosos-snf*sinos;
    sn2f2g=2.*snfg*csfg;
    cs2f2g=2.*pow(csfg,2)-1.;
    ecosf=em*csf;
    g10=fm-xmam+em*snf;
    rm=pm/(1.+ecosf);
    aovr=am/rm;
    g13=xn*aovr;
    g14=-g13*aovr;
    dr=g2*(unmth2*cs2f2g-3.*tthmun)-g4*snfg;
    diwc=3.*g3*sini*cs2f2g-g5*aynm;
    di=diwc*cosi;
    sini2=sin(.5*xinc);

/*
 *  Update for short period periodics.
 */
    sni2du=sinio2*(
       g3*(.5*(1.-7.*theta2)*sn2f2g-3.*unm5th*g10)-g5*sini*csfg*(2.+
             ecosf))-.5*g5*theta2*axnm/cosio2;
    xlamb=fm+omgasm+xnodes+g3*(.5*(1.+6.*cosi-7.*theta2)*sn2f2g-3.*
          (unm5th+2.*cosi)*g10)+g5*sini*(cosi*axnm/(1.+cosi)-(2.
          +ecosf)*csfg);
    y4=sini2*snfg+csfg*sni2du+.5*snfg*cosio2*di;
    y5=sini2*csfg-snfg*sni2du+.5*csfg*cosio2*di;
    r=rm+dr;
    rdot=xn*am*em*snf/beta+g14*(2.*g2*unmth2*sn2f2g+g4*csfg);
    rvdot=xn*pow(am,2)*beta/rm+
          g14*dr+am*g13*sini*diwc;

/*
 *  Orientation vectors.
 */
    snlamb=sin(xlamb);
    cslamb=cos(xlamb);
    temp=2.*(y5*snlamb-y4*cslamb);
    ux=y4*temp+cslamb;
    vx=y5*temp-snlamb;
    temp=2.*(y5*cslamb+y4*snlamb);
    uy=-y4*temp+snlamb;
    vy=-y5*temp+cslamb;
    temp=2.*sqrt(1.-y4*y4-y5*y5);
    uz=y4*temp;
    vz=y5*temp;

/*
 *  Position and Velocity.
 */
    x=r*ux;
    y=r*uy;
    z=r*uz;
    xdot=rdot*ux+rvdot*vx;
    ydot=rdot*uy+rvdot*vy;
    zdot=rdot*uz+rvdot*vz;
}
