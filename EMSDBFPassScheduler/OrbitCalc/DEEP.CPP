/*
 *      DEEP SPACE                                         31 OCT 80
 */

//---------------------------------------------------------------------------
#include <stdio.h>
#include <math.h>
#include "SgpxStr.h"
#include "Predict.h"

#pragma warning ( disable : 4244 )

#define LABEL30  30
#define LABEL40  40
#define LABEL100 100
#define LABEL125 125
#define LABEL140 140
#define LABEL165 165

//---------------------------------------------------------------------------
void TOrbit::Deep (int iflg, double eqsq, double siniq, double cosiq, double rteqsq, double ao, double cosq2,
           double sinomo, double cosomo, double bsq, double xlldot, double omgdt, double xnodot, double xnodp,
           double *pxll, double *pomgasm, double *pxnodes, double *pem, double *pxinc, double *pxn, double t)
{
    static double day, preep, xnodce, atime, delt, savtsn, step2, stepn, stepp;
    static double 
          zns    = (double)1.19459e-5,   c1ss   = (double)2.9864797e-6, zes    = (double).01675, 
          znl    = (double)1.5835218e-4, c1l    = (double)4.7968065e-7, zel    = (double).05490, 
          zcosis = (double).91744867,    zsinis = (double).39785416,    zsings = (double)-.98088458,
          zcosgs = (double).1945905;
    static double
          q22 = (double)1.7891679e-6, q31 = (double)2.1460748e-6, q33 = (double)2.2123015e-7, 
          g22 = (double)5.7686396,    g32 = (double)0.95240898, 
          g44 = (double)1.8014998,    g52 = (double)1.0508330, 
          g54 = (double)4.4108898, 
          root22 = (double)1.7891679e-6, root32 = (double)3.7393792e-7, 
          root44 = (double)7.3636953e-9, root52 = (double)1.1428639e-7, 
          root54 = (double)2.1765803e-9, 
          thdt   = (double)4.3752691e-3;
    static double thgr, eq, xnq, aqnv, xqncl, xmao, xpidot, sinq, cosq, omegaq, 
          stem, ctem, zcosil, zsinil, zsinhl, zcoshl, c, gam, zmol, zx, zy, 
          zcosgl, zsingl, zmos, zcosg, zsing, zcosi, zsini, zcosh, zsinh, 
          cc, zn, ze, zmo, xnoi, a1, a3, a7, a8, a9, a10, a2, a4, a5, a6, 
          x1, x2, x3, x4, x5, x6, x7, x8, z31, z32, z33, z1, z2, z3, 
          z11, z12, z13 , z21, z22, z23, s3, s2, s4, s1, s5, s6, s7, se, si, 
          sl, sgh, sh, ee2, e3, xi2, xi3, xl2, xl3, xl4, xgh2, xgh3, xgh4, 
          xh2, xh3, sse, ssi, ssl, ssh, ssg, se2, si2, sl2, sgh2, sh2, se3, 
          si3, sl3, sgh3, sh3, sl4, sgh4, eoc, g201, g211, g310, g322, g410, 
          g422, g520, g533, g521, g532, sini2, f220, f221, f321, f322, 
          f441, f442, f522, f523, f542, f543, xno2, ainv2, temp1, temp, 
          d2201, d2211, d3210, d3222, d4410, d4422, d5220, d5232, d5421, 
          d5433, xlamo, bfact, g200, g300, f311, f330, del1, del2, del3, 
          fasx2, fasx4, fasx6, xfact, xli, xni, ft, xndot, xnddt, xl, xldot, 
          xomi, x2omi, x2li, sinis, cosis, zm, zf, sinzf, f2, f3, ses, sis, 
          sls, sghs, shs, sel, sil, sll, sghl, shl, pe, pinc, pl, pgh, ph, 
          sinok, cosok, alfdp, betdp, dalf, dbet, xls, dls;
     static int   ls, iresfl, isynfl, iret, iretn;

    if (iflg EQ 1)
    {
/*
 *      Entrance for deep space initialization.
 */
/*        ENTRY DPINIT(EQSQ,SINIQ,COSIQ,RTEQSQ,AO,COSQ2,SINOMO,COSOMO,
   1             BSQ,XLLDOT,OMGDT,XNODOT,XNODP) */
        thgr=Thetag(epoch);
        eq = eo;
        xnq = xnodp;
        aqnv = 1./ao;
        xqncl = xincl;
        xmao=xmo;
        xpidot=omgdt+xnodot;
        sinq = sin(xnodeo);
        cosq = cos(xnodeo);
        omegaq = omegao;

/*
 *      Initialize lunar solar terms.
 */
        day=ds50+18261.5e0;
        if (day EQ preep)    goto L10;
        preep = day;
        xnodce=4.5236020-9.2422029e-4*day;
        stem=sin (xnodce);
        ctem=cos (xnodce);
        zcosil=.91375164-.03568096*ctem;
        zsinil=sqrt (1.-zcosil*zcosil);
        zsinhl= .089683511*stem/zsinil;
        zcoshl=sqrt (1.-zsinhl*zsinhl);
        c=4.7199672+.22997150*day;
        gam=5.8351514+.0019443680*day;
        zmol = Fmod2p(c-gam);
        zx= .39785416*stem/zsinil;
        zy= zcoshl*ctem+0.91744867*zsinhl*stem;
        zx=Actan(zx,zy);
        zx=gam+zx-xnodce;
        zcosgl=cos (zx);
        zsingl=sin (zx);
        zmos=6.2565837e0+.017201977e0*day;
        zmos=Fmod2p(zmos);

/*
 *      Do solar terms.
 */
L10:    ls = 0;
        savtsn=1.e20;
        zcosg=zcosgs;
        zsing=zsings;
        zcosi=zcosis;
        zsini=zsinis;
        zcosh=cosq;
        zsinh=sinq;
        cc=c1ss;
        zn=zns;
        ze=zes;
        zmo=zmos;
        xnoi=1./xnq;
/*        assign L30 to ls; */
        ls = LABEL30;
L20:    a1=zcosg*zcosh+zsing*zcosi*zsinh;
        a3=-zsing*zcosh+zcosg*zcosi*zsinh;
        a7=-zcosg*zsinh+zsing*zcosi*zcosh;
        a8=zsing*zsini;
        a9=zsing*zsinh+zcosg*zcosi*zcosh;
        a10=zcosg*zsini;
        a2= cosiq*a7+ siniq*a8;
        a4= cosiq*a9+ siniq*a10;
        a5=- siniq*a7+ cosiq*a8;
        a6=- siniq*a9+ cosiq*a10;

        x1=a1*cosomo+a2*sinomo;
        x2=a3*cosomo+a4*sinomo;
        x3=-a1*sinomo+a2*cosomo;
        x4=-a3*sinomo+a4*cosomo;
        x5=a5*sinomo;
        x6=a6*sinomo;
        x7=a5*cosomo;
        x8=a6*cosomo;

        z31=12.*x1*x1-3.*x3*x3;
        z32=24.*x1*x2-6.*x3*x4;
        z33=12.*x2*x2-3.*x4*x4;
        z1=3.*(a1*a1+a2*a2)+z31*eqsq;
        z2=6.*(a1*a3+a2*a4)+z32*eqsq;
        z3=3.*(a3*a3+a4*a4)+z33*eqsq;
        z11=-6.*a1*a5+eqsq *(-24.*x1*x7-6.*x3*x5);
        z12=-6.*(a1*a6+a3*a5)+eqsq *(-24.*(x2*x7+x1*x8)-
             6.*(x3*x6+x4*x5));
        z13=-6.*a3*a6+eqsq *(-24.*x2*x8-6.*x4*x6);
        z21=6.*a2*a5+eqsq *(24.*x1*x5-6.*x3*x7);
        z22=6.*(a4*a5+a2*a6)+eqsq *(24.*(x2*x5+x1*x6)-
            6.*(x4*x7+x3*x8));
        z23=6.*a4*a6+eqsq *(24.*x2*x6-6.*x4*x8);
        z1=z1+z1+bsq*z31;
        z2=z2+z2+bsq*z32;
        z3=z3+z3+bsq*z33;
        s3=cc*xnoi;
        s2=-.5*s3/rteqsq;
        s4=s3*rteqsq;
        s1=-15.*eq*s4;
        s5=x1*x3+x2*x4;
        s6=x2*x3+x1*x4;
        s7=x2*x4-x1*x3;
        se=s1*zn*s5;
        si=s2*zn*(z11+z13);
        sl=-zn*s3*(z1+z3-14.-6.*eqsq);
        sgh=s4*zn*(z31+z33-6.);
        sh=-zn*s2*(z21+z23);
        if(xqncl < 5.2359877e-2) sh=0.0;
        ee2=2.*s1*s6;
        e3=2.*s1*s7;
        xi2=2.*s2*z12;
        xi3=2.*s2*(z13-z11);
        xl2=-2.*s3*z2;
        xl3=-2.*s3*(z3-z1);
        xl4=-2.*s3*(-21.-9.*eqsq)*ze;
        xgh2=2.*s4*z32;
        xgh3=2.*s4*(z33-z31);
        xgh4=-18.*s4*ze;
        xh2=-2.*s2*z22;
        xh3=-2.*s2*(z23-z21);
/*        goto ls; */
        if (ls EQ LABEL30)
            goto L30;
        else if (ls EQ LABEL40)
            goto L40;
        else
		{
			throw UnexError ( EMS_ORBIT_UNEXPECTEDDEEPERR1 );
		}

/*
 *      Do lunar terms.
 */
L30:    sse = se;
        ssi=si;
        ssl=sl;
        ssh=sh/siniq;
        ssg=sgh-cosiq*ssh;
        se2=ee2;
        si2=xi2;
        sl2=xl2;
        sgh2=xgh2;
        sh2=xh2;
        se3=e3;
        si3=xi3;
        sl3=xl3;
        sgh3=xgh3;
        sh3=xh3;
        sl4=xl4;
        sgh4=xgh4;
        ls=1;
        zcosg=zcosgl;
        zsing=zsingl;
        zcosi=zcosil;
        zsini=zsinil;
        zcosh=zcoshl*cosq+zsinhl*sinq;
        zsinh=sinq*zcoshl-cosq*zsinhl;
        zn=znl;
        cc=c1l;
        ze=zel;
        zmo=zmol;
/*        assign L40 to ls; */
        ls = LABEL40;
        goto L20;
L40:    sse = sse+se;
        ssi=ssi+si;
        ssl=ssl+sl;
        ssg=ssg+sgh-cosiq/siniq*sh;
        ssh=ssh+sh/siniq;

/*
 *      Geopotential resonance initialization for 12 hour orbits.
 */
        iresfl=0;
        isynfl=0;
        if(xnq < (.0052359877) AND xnq > (.0034906585)) goto L70;
        if (xnq < (8.26e-3) OR xnq > (9.24e-3))    return;
        if (eq < 0.5)    return;
        iresfl =1;
        eoc=eq*eqsq;
        g201=-.306-(eq-.64)*.440;
        if(eq > (.65)) goto L45;
        g211=3.616-13.247*eq+16.290*eqsq;
        g310=-19.302+117.390*eq-228.419*eqsq+156.591*eoc;
        g322=-18.9068+109.7927*eq-214.6334*eqsq+146.5816*eoc;
        g410=-41.122+242.694*eq-471.094*eqsq+313.953*eoc;
        g422=-146.407+841.880*eq-1629.014*eqsq+1083.435*eoc;
        g520=-532.114+3017.977*eq-5740*eqsq+3708.276*eoc;
        goto L55;
L45:    g211=-72.099+331.819*eq-508.738*eqsq+266.724*eoc;
        g310=-346.844+1582.851*eq-2415.925*eqsq+1246.113*eoc;
        g322=-342.585+1554.908*eq-2366.899*eqsq+1215.972*eoc;
        g410=-1052.797+4758.686*eq-7193.992*eqsq+3651.957*eoc;
        g422=-3581.69+16178.11*eq-24462.77*eqsq+12422.52*eoc;
        if(eq > (.715)) goto L50;
        g520=1464.74-4664.75*eq+3763.64*eqsq;
        goto L55;
L50:    g520=-5149.66+29936.92*eq-54087.36*eqsq+31324.56*eoc;
L55:    if(eq >= (.7)) goto L60;
        g533=-919.2277+4988.61*eq-9064.77*eqsq+5542.21*eoc;
        g521 = -822.71072+4568.6173*eq-8491.4146*eqsq+5337.524*eoc;
        g532 = -853.666+4690.25*eq-8624.77*eqsq+5341.4*eoc;
        goto L65;
L60:    g533=-37995.78+161616.52*eq-229838.2*eqsq+109377.94*eoc;
        g521 = -51752.104+218913.95*eq-309468.16*eqsq+146349.42*eoc;
        g532 = -40023.88+170470.89*eq-242699.48*eqsq+115605.82*eoc;
L65:    sini2=siniq*siniq;
        f220=.75*(1.+2.*cosiq+cosq2);
        f221=1.5*sini2;
        f321=1.875*siniq*(1.-2.*cosiq-3.*cosq2);
        f322=-1.875*siniq*(1.+2.*cosiq-3.*cosq2);
        f441=35.*sini2*f220;
        f442=39.3750*sini2*sini2;
        f522=9.84375*siniq*(sini2*(1.-2.*cosiq-5.*cosq2)
             +.33333333*(-2.+4.*cosiq+6.*cosq2));
        f523 = siniq*(4.92187512*sini2*(-2.-4.*cosiq+10.*cosq2)
              +6.56250012*(1.+2.*cosiq-3.*cosq2));
        f542 = 29.53125*siniq*(2.-8.*cosiq+cosq2*(-12.+8.*cosiq
              +10.*cosq2));
        f543=29.53125*siniq*(-2.-8.*cosiq+
                             cosq2*(12.+8.*cosiq-10.*cosq2));
        xno2=xnq*xnq;
        ainv2=aqnv*aqnv;
        temp1 = 3.*xno2*ainv2;
        temp = temp1*root22;
        d2201 = temp*f220*g201;
        d2211 = temp*f221*g211;
        temp1 = temp1*aqnv;
        temp = temp1*root32;
        d3210 = temp*f321*g310;
        d3222 = temp*f322*g322;
        temp1 = temp1*aqnv;
        temp = 2.*temp1*root44;
        d4410 = temp*f441*g410;
        d4422 = temp*f442*g422;
        temp1 = temp1*aqnv;
        temp = temp1*root52;
        d5220 = temp*f522*g520;
        d5232 = temp*f523*g532;
        temp = 2.*temp1*root54;
        d5421 = temp*f542*g521;
        d5433 = temp*f543*g533;
        xlamo = xmao+xnodeo+xnodeo-thgr-thgr;
        bfact = xlldot+xnodot+xnodot-thdt-thdt;
        bfact=bfact+ssl+ssh+ssh;
        goto L80;

/*
 *      Synchronous resonance terms initialization.
 */
L70:    iresfl=1;
        isynfl=1;
        g200=1.0+eqsq*(-2.5+.8125*eqsq);
        g310=1.0+2.0*eqsq;
        g300=1.0+eqsq*(-6.0+6.60937*eqsq);
        f220=.75*(1.+cosiq)*(1.+cosiq);
        f311=.9375*siniq*siniq*(1.+3.*cosiq)-.75*(1.+cosiq);
        f330=1.+cosiq;
        f330=1.875*f330*f330*f330;
        del1=3.*xnq*xnq*aqnv*aqnv;
        del2=2.*del1*f220*g200*q22;
        del3=3.*del1*f330*g300*q33*aqnv;
        del1=del1*f311*g310*q31*aqnv;
        fasx2=(double).13130908;
        fasx4=(double)2.8843198;
        fasx6=(double).37448087;
        xlamo=xmao+xnodeo+omegao-thgr;
        bfact = xlldot+xpidot-thdt;
        bfact=bfact+ssl+ssg+ssh;
L80:    xfact=bfact-xnq;

/*
 *      Initialize integrator.
 */
        xli=xlamo;
        xni=xnq;
        atime=0.e0;
        stepp=720.e0;
        stepn=-720.e0;
        step2 = 259200.e0;
        return;
    }
    else if (iflg EQ 2)
    {
/*
 *      Entrance for deep space secular effects.
 */
/*        ENTRY DPSEC(XLL,OMGASM,XNODES,EM,XINC,XN,T,OMGDT) */
        *pxll=*pxll+ssl*t;
        *pomgasm=*pomgasm+ssg*t;
        *pxnodes=*pxnodes+ssh*t;
        *pem=eo+sse*t;
        *pxinc=xincl+ssi*t;
        if(*pxinc >= 0.) goto L90;
        *pxinc = -*pxinc;
        *pxnodes = *pxnodes + PI;
        *pomgasm = *pomgasm - PI;
L90:    if(iresfl EQ 0) return;
L100:   if (atime EQ 0.e0)    goto L170;
        if(t >= (0.e0) AND atime < (0.e0)) goto L170;
        if(t < (0.e0) AND atime >= (0.e0)) goto L170;
        if(fabs(t) >= fabs(atime)) goto L120;
        delt=stepp;
        if (t >= 0.e0)    delt = stepn;
/*        assign L100 to iret; */
        iret = LABEL100;
        goto L160;
L120:   delt=stepn;
        if (t > 0.e0)    delt = stepp;
L125:   if (fabs((double)t-atime) < stepp)    goto L130;
/*        assign L125 to iret; */
        iret = LABEL125;
        goto L160;
L130:   ft = t-atime;
/*        assign L140 to iretn; */
        iretn = LABEL140;
        goto L150;
L140:   *pxn = xni+xndot*ft+xnddt*ft*ft*0.5;
        xl = xli+xldot*ft+xndot*ft*ft*0.5;
        temp = -*pxnodes+thgr+t*thdt;
        *pxll = xl-*pomgasm+temp;
        if (isynfl EQ 0)    *pxll = xl+temp+temp;
        return;

/*
 *      Dot terms calculated.
 */
L150:   if (isynfl EQ 0)    goto L152;
        xndot=del1*sin (xli-fasx2)+del2*sin (2.*(xli-fasx4))
             +del3*sin (3.*(xli-fasx6));
        xnddt = del1*cos(xli-fasx2)
               +2.*del2*cos(2.*(xli-fasx4))
               +3.*del3*cos(3.*(xli-fasx6));
        goto L154;
L152:   xomi = omegaq+omgdt*atime;
        x2omi = xomi+xomi;
        x2li = xli+xli;
        xndot = d2201*sin(x2omi+xli-g22)
               +d2211*sin(xli-g22)
               +d3210*sin(xomi+xli-g32)
               +d3222*sin(-xomi+xli-g32)
               +d4410*sin(x2omi+x2li-g44)
               +d4422*sin(x2li-g44)
               +d5220*sin(xomi+xli-g52)
               +d5232*sin(-xomi+xli-g52)
               +d5421*sin(xomi+x2li-g54)
               +d5433*sin(-xomi+x2li-g54);
        xnddt = d2201*cos(x2omi+xli-g22)
               +d2211*cos(xli-g22)
               +d3210*cos(xomi+xli-g32)
               +d3222*cos(-xomi+xli-g32)
               +d5220*cos(xomi+xli-g52)
               +d5232*cos(-xomi+xli-g52)
               +2.*(d4410*cos(x2omi+x2li-g44)
               +d4422*cos(x2li-g44)
               +d5421*cos(xomi+x2li-g54)
               +d5433*cos(-xomi+x2li-g54));
L154:   xldot=xni+xfact;
        xnddt = xnddt*xldot;
/*        goto iretn; */
        if (iretn EQ LABEL140)
            goto L140;
        else if (iretn EQ LABEL165)
            goto L165;
        else
		{
            throw UnexError ( EMS_ORBIT_UNEXPECTEDDEEPERR2 );
		}

          
/*
 *      Integrator.
 */
L160:   ;
/*        assign L165 to iretn; */
        iretn = LABEL165;
        goto L150;
L165:   xli = xli+xldot*delt+xndot*step2;
        xni = xni+xndot*delt+xnddt*step2;
        atime=atime+delt;
/*        goto iret; */
        if (iret EQ LABEL100)
            goto L100;
        else if (iret EQ LABEL125)
            goto L125;
        else
			throw UnexError ( EMS_ORBIT_UNEXPECTEDDEEPERR1 );
 
/*
 *      Epoch restart.
 */
L170:   if (t >= 0.e0)    goto L175;
        delt=stepn;
        goto L180;
L175:   delt = stepp;
L180:   atime = 0.e0;
        xni=xnq;
        xli=xlamo;
        goto L125;
    }
    else if (iflg EQ 3)
    {
/*
 *      Entrances for lunar-solar periodics.
 */
/*        ENTRY DPPER(EM,XINC,OMGASM,XNODES,XLL,SINIQ,COSIQ,T) */
        sinis = sin(*pxinc);
        cosis = cos(*pxinc);
        if (fabs(savtsn-(double)t) < (30.e0))    goto L210;
        savtsn=t;
        zm=zmos+zns*t;
        zf=zm+2.*zes*sin (zm);
        sinzf=sin (zf);
        f2=.5*sinzf*sinzf-.25;
        f3=-.5*sinzf*cos (zf);
        ses=se2*f2+se3*f3;
        sis=si2*f2+si3*f3;
        sls=sl2*f2+sl3*f3+sl4*sinzf;
        sghs=sgh2*f2+sgh3*f3+sgh4*sinzf;
        shs=sh2*f2+sh3*f3;
        zm=zmol+znl*t;
        zf=zm+2.*zel*sin (zm);
        sinzf=sin (zf);
        f2=.5*sinzf*sinzf-.25;
        f3=-.5*sinzf*cos (zf);
        sel=ee2*f2+e3*f3;
        sil=xi2*f2+xi3*f3;
        sll=xl2*f2+xl3*f3+xl4*sinzf;
        sghl=xgh2*f2+xgh3*f3+xgh4*sinzf;
        shl=xh2*f2+xh3*f3;
        pe=ses+sel;
        pinc=sis+sil;
        pl=sls+sll;
L210:   pgh=sghs+sghl;
        ph=shs+shl;
        *pxinc = *pxinc+pinc;
        *pem = *pem+pe;
        if(xqncl < (.2)) goto L220;
        goto L218;

/*
 *      Apply periodics directly.
 */
L218:   ph=ph/siniq;
        pgh=pgh-cosiq*ph;
        *pomgasm=*pomgasm+pgh;
        *pxnodes=*pxnodes+ph;
        *pxll = *pxll+pl;
        goto L230;

/*
 *      Apply periodics with LYDDANE modification.
 */
L220:   sinok=sin(*pxnodes);
        cosok=cos(*pxnodes);
        alfdp=sinis*sinok;
        betdp=sinis*cosok;
        dalf=ph*cosok+pinc*cosis*sinok;
        dbet=-ph*sinok+pinc*cosis*cosok;
        alfdp=alfdp+dalf;
        betdp=betdp+dbet;
        xls = *pxll+*pomgasm+cosis*(*pxnodes);
        dls=pl+pgh-pinc*(*pxnodes)*sinis;
        xls=xls+dls;
        *pxnodes=Actan(alfdp,betdp);
        *pxll = *pxll+pl;
        *pomgasm = xls-*pxll-cos(*pxinc)*(*pxnodes);
L230:   return;
    }
}
