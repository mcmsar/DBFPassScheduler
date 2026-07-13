/*
 *      SGP8                                              14 NOV 80
 */

//---------------------------------------------------------------------------
#include <math.h>
#include "SgpxStr.h"
#include "Predict.h"

#pragma warning ( disable : 4244 )

//---------------------------------------------------------------------------

void TOrbit::Sgp8 (int *piflag, double tsince)
{
    static double a1, cosi, theta2, tthmun, eosq, betao2, betao, del1, ao, 
          delo, aodp, xnodp, b, po, pom2, sini, sing, cosg, temp, 
          sinio2, cosio2, theta4, unm5th, unmth2, a3cof, 
          pardt1, pardt2, pardt4, xmdt1, xgdt1, xhdt1, xlldot, omgdt, xnodot, 
          tsi, eta, eta2, psim2, alpha2, eeta, cos2g, d5, d1, d2, d3, d4, 
          b1, b2, b3, c0, c1, c4, c5, xndt, xndtn, d6, d7, d8, c8, c9, edot, 
          d20, aldtal, tsdtts, etdt, psdtps, sin2g, c0dtc0, c1dtc1, 
          d9, d10, d11, d12, d13, d14, d15, d1dt, d2dt, d3dt, d4dt, d5dt, 
          c4dt, c5dt, d16, xnddt, eddot, d25, d17, tsddts, etddt, 
          d18, d19, d23, d1ddt, xntrdt, tmnddt, pp, gamma, xnd, qq, ed, ovgpp, 
          xmam, omgasm, xnodes, temp1, xn, em, z1, z7, zc2, sine, cose, zc5, 
          cape, am, beta2m, sinos, cosos, axnm, aynm, pm, g1, g2, g3, beta, 
          g4, g5, snf, csf, fm, snfg, csfg, sn2f2g, cs2f2g, ecosf, g10, rm, 
          aovr, g13, g14, dr, diwc, di, sni2du, xlamb, y4, y5, r, rdot, rvdot, 
          snlamb, cslamb, ux, vx, uy, vy, uz, vz;
    static int   isimp;
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
    isimp=0;
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
    xlldot=xnodp+xmdt1+.0625*pardt2*betao*(13.-78.*theta2+137.*theta4);
    omgdt=xgdt1+.0625*pardt2*(7.-114.*theta2+395.*theta4)+
                pardt4*(3.-36.*theta2+49.*theta4);
    xnodot=xhdt1+(.5*pardt2*(4.-19.*theta2)+2.*pardt4*(3.-7.*theta2))*cosi;
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
    xndt=c1*((2.+eta2*(3.+34.*eosq)+5.*eeta*(4.+eta2)+8.5*eosq)+
             d1*d2*b1+   c4*cos2g+c5*sing);
    xndtn=xndt/xnodp;

/*
 *  If drag is very small, the isimp flag is set and the
 *  equations are truncated to linear variation in mean
 *  motion and quadratic variation in mean anomaly.
 */
    if(fabs(xndtn*xmnpda) < 2.16e-3) goto L50;
    d6=eta*(30.+22.5*eta2);
    d7=eta*(5.+12.5*eta2);
    d8=1.+eta2*(6.75+eta2);
    c8=d1*d7*b2;
    c9=d5*d8*b3;
    edot=-c0*(eta*(4.+eta2+eosq*(15.5+7.*eta2))+eo*(5.+15.*eta2)+
              d1*d6*b1 + c8*cos2g+c9*sing);
    d20=.5*tothrd*xndtn;
    aldtal=eo*edot/alpha2;
    tsdtts=2.*aodp*tsi*(d20*betao2+eo*edot);
    etdt=(edot+eo*tsdtts)*tsi*s;
    psdtps=-eta*etdt*psim2;
    sin2g=2.*sing*cosg;
    c0dtc0=d20+4.*tsdtts-aldtal-7.*psdtps;
    c1dtc1=xndtn+4.*aldtal+c0dtc0;
    d9=eta*(6.+68.*eosq)+eo*(20.+15.*eta2);
    d10=5.*eta*(4.+eta2)+eo*(17.+68.*eta2);
    d11=eta*(72.+18.*eta2);
    d12=eta*(30.+10.*eta2);
    d13=5.+11.25*eta2;
    d14=tsdtts-2.*psdtps;
    d15=2.*(d20+eo*edot/betao2);
    d1dt=d1*(d14+d15);
    d2dt=etdt*d11;
    d3dt=etdt*d12;
    d4dt=etdt*d13;
    d5dt=d5*d14;
    c4dt=b2*(d1dt*d3+d1*d3dt);
    c5dt=b3*(d5dt*d4+d5*d4dt);
    d16= d9*etdt+d10*edot +
         b1*(d1dt*d2+d1*d2dt) +
         c4dt*cos2g+c5dt*sing+xgdt1*(c5*cosg-2.*c4*sin2g);
    xnddt=c1dtc1*xndt+c1*d16;
    eddot=c0dtc0*edot-c0*(
         (4.+3.*eta2+30.*eeta+eosq*(15.5+21.*eta2))*etdt+(5.+15.*eta2
             +eeta*(31.+14.*eta2))*edot +
         b1*(d1dt*d6+d1*etdt*(30.+67.5*eta2))  +
         b2*(d1dt*d7+d1*etdt*(5.+37.5*eta2))*cos2g+
         b3*(d5dt*d8+d5*etdt*eta*(13.5+4.*eta2))*sing+xgdt1*(c9*
             cosg-2.*c8*sin2g));
    d25=pow(edot,2);
    d17=xnddt/xnodp-pow(xndtn,2);
    tsddts=2.*tsdtts*(tsdtts-d20)+aodp*tsi*(tothrd*betao2*d17-4.*d20*
             eo*edot+2.*(d25+eo*eddot));
    etddt =(eddot+2.*edot*tsdtts)*tsi*s+tsddts*eta;
    d18=tsddts-pow(tsdtts,2);
    d19=-pow(psdtps,2)/eta2-eta*etddt*psim2-pow(psdtps,2);
    d23=etdt*etdt;
    d1ddt=d1dt*(d14+d15)+d1*(d18-2.*d19+tothrd*d17+2.*(alpha2*d25
             /betao2+eo*eddot)/betao2);
    xntrdt=xndt*(2.*tothrd*d17+3.*
      (d25+eo*eddot)/alpha2-6.*pow(aldtal,2) +
      4.*d18-7.*d19 )   +
      c1dtc1*xnddt+c1*(c1dtc1*d16+
      d9*etddt+d10*eddot+d23*(6.+30.*eeta+68.*eosq)+
      etdt*edot*(40.+30.*
      eta2+272.*eeta)+d25*(17.+68.*eta2) +
        b1*(d1ddt*d2+2.*d1dt*d2dt+d1*(etddt*d11+d23*(72.+54.*eta2))) +
        b2*(d1ddt*d3+2.*d1dt*d3dt+d1*(etddt*d12+d23*(30.+30.*eta2))) *
        cos2g+
          b3*((d5dt*d14+d5*(d18-2.*d19)) *
     d4+2.*d4dt*d5dt+d5*(etddt*d13+22.5*eta*d23)) *sing+xgdt1*
             ((7.*d20+4.*eo*edot/betao2)*
             (c5*cosg-2.*c4*sin2g)
             +((2.*c5dt*cosg-4.*c4dt*sin2g)-xgdt1*(c5*sing+4.*
             c4*cos2g))));
    tmnddt=xnddt*1.e9;
    temp=pow(tmnddt,2)-xndt*1.e18*xntrdt;
    pp=(temp+pow(tmnddt,2))/temp;
    gamma=-xntrdt/(xnddt*(pp-2.));
    xnd=xndt/(pp*gamma);
    qq=1.-eddot/(edot*gamma);
    ed=edot/(qq*gamma);
    ovgpp=1./(gamma*(pp+1.));
    goto L70;
L50: isimp=1;
    edot=-tothrd*xndtn*(1.-eo);
L70: *piflag=0;

/*
 *  Update for secular gravity and atmospheric drag.
 */
L100: xmam=Fmod2p(xmo+xlldot*tsince);
    omgasm=omegao+omgdt*tsince;
    xnodes=xnodeo+xnodot*tsince;
    if(isimp EQ 1) goto L105;
    temp=1.-gamma*tsince;
    temp1=pow(temp,pp);
    xn=xnodp+xnd*(1.-temp1);
    em=eo+ed*(1.-pow(temp,qq));
    z1=xnd*(tsince+ovgpp*(temp*temp1-1.));
    goto L108;
L105: xn=xnodp+xndt*tsince;
    em=eo+edot*tsince;
    z1=.5*xndt*tsince*tsince;
L108: z7=3.5*tothrd*z1/xnodp;
    xmam=Fmod2p(xmam+z1+z7*xmdt1);
    omgasm=omgasm+z7*xgdt1;
    xnodes=xnodes+z7*xhdt1;

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

/*
 *  Update for short period periodics.
 */
    sni2du=sinio2*(
       g3*(.5*(1.-7.*theta2)*sn2f2g-3.*unm5th*g10)-g5*sini*csfg*(2.+
             ecosf))-.5*g5*theta2*axnm/cosio2;
    xlamb=fm+omgasm+xnodes+g3*(.5*(1.+6.*cosi-7.*theta2)*sn2f2g-3.*
          (unm5th+2.*cosi)*g10)+g5*sini*(cosi*axnm/(1.+cosi)-(2.
          +ecosf)*csfg);
    y4=sinio2*snfg+csfg*sni2du+.5*snfg*cosio2*di;
    y5=sinio2*csfg-snfg*sni2du+.5*csfg*cosio2*di;
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
