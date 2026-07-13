//---------------------------------------------------------------------------
#include <math.h>
#include "SgpxStr.h"
#include "Predict.h"

#pragma warning ( disable : 4244 )

//---------------------------------------------------------------------------

TCLVector TConvert::ToCL (TINVector *IVec)
{
    double ric, vic, rv, hx, hy, hz, hr, hv, hxy, h2, h, vsbm, ai, aa, bb, tmp;
    double RxIC, RyIC, RzIC, VxIC, VyIC, VzIC;

    TCLVector OVec;         // create output vector

    OVec.VTime = IVec->VTime;
    RxIC = IVec->Rx;
    RyIC = IVec->Ry;
    RzIC = IVec->Rz;
    VxIC = IVec->Vx;
    VyIC = IVec->Vy;
    VzIC = IVec->Vz;

    ric  = sqrt (RxIC*RxIC + RyIC*RyIC + RzIC*RzIC);
    vic  = sqrt (VxIC*VxIC + VyIC*VyIC + VzIC*VzIC);
    rv   = RxIC*VxIC + RyIC*VyIC + RzIC*VzIC;
    hx   = RyIC*VzIC - RzIC*VyIC;
    hy   = RzIC*VxIC - RxIC*VzIC;
    hz   = RxIC*VyIC - RyIC*VxIC;
    hr   =   hx*RyIC - RxIC*hy;
    hv   =   hx*VyIC - VxIC*hy;
    hxy  =   hx*hx   +   hy*hy;
    h2   =     hxy   +   hz*hz;
    h    = sqrt (h2);
    vsbm = vic*vic/MU;
    ai   = 2e0/ric - vsbm;
    aa   = vsbm - 1e0/ric;
    bb   = rv/MU;

    OVec.SMA  = 1e0/ai;
    OVec.ECC  = sqrt (fabs (aa*aa*ric*ric + bb*rv*ai));
    OVec.AINC = RDG * Actan (sqrt (hxy), hz);
    OVec.PERI = 0e0;
    OVec.ANOD = RDG * Actan (hx, -hy);

    if (hxy EQ 0e0) goto L10;
    if (OVec.ECC > 0e0)
        OVec.PERI = RDG * Actan (h*(aa*RzIC - bb*VzIC), aa*hr - bb*hv);
    OVec.ANOM = RDG * Actan (h*RzIC, hr) - OVec.PERI;
    goto L20;

L10: if (OVec.ECC > 0e0)
        OVec.PERI = RDG * Actan (SIGN (aa*RyIC - bb*VyIC, hz), aa*RxIC - bb*VxIC);
    OVec.ANOM = RDG * Actan (SIGN (RyIC, hz), RxIC) - OVec.PERI;

L20: if (OVec.ANOD < 0e0) OVec.ANOD += 360e0;
    if (OVec.PERI < 0e0) OVec.PERI += 360e0;
    if (OVec.ANOM < 0e0) OVec.ANOM += 360e0;

    tmp = 1.5e0 * J2 * (4e0 * sin (OVec.AINC/RDG) - 3e0) * pow (RE / OVec.SMA, 2);
    OVec.PERIOD = TWOPI * OVec.SMA * sqrt (OVec.SMA/MU) * (1e0 + tmp);

    return(OVec);
}

TCLVector* TConvert::ToCL(TINVector *IVec, int NumVec)
{
  int i;
  TCLVector *OutVec;
  TCLVector *SaveVec;
  i = 0;

  OutVec = new TCLVector[NumVec];
  SaveVec = OutVec;
  for (i=0; i < NumVec; i++)
  {
    *OutVec++ = ToCL(IVec++);
  }
  return(SaveVec);
}
