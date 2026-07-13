//---------------------------------------------------------------------------
#include <math.h>
#include "SgpxStr.h"
#include "Predict.h"

#pragma warning ( disable : 4244 )

//---------------------------------------------------------------------------

TGSVector TConvert::ToGS (TINVector *IVec, TGSRef GS)
{
    double GxIC, GyIC, GzIC;
    double th, sth, cth, sla, cla, re2, rhx, rhy, rhz, rht, w = (double)(PI*2.0/SID);
    double thetag;

    thetag = double(Gst(IVec->VTime));
    TGSVector OVec;
/*
 *  Transform the Ground station Lat/Long and altitude into Inertial space.
 */
      th   = GS.lon/RDG + thetag;
      sth  = sin (th);
      cth  = cos (th);
      sla  = sin (GS.lat/RDG);
      cla  = cos (GS.lat/RDG);
      re2  = RE / sqrt (1.0 - E2*sla*sla);

      GxIC = (re2 + GS.alt)*cla*cth;
      GyIC = (re2 + GS.alt)*cla*sth;
      GzIC = (re2*(1.0 - E2) + GS.alt)*sla;
/*
 *  Transform satellite Inertial space into Ground Station space.
 */
      rhx  = IVec->Rx - GxIC;
      rhy  = IVec->Ry - GyIC;
      rhz  = IVec->Rz - GzIC;
      rht  = cth*rhx + sth*rhy;

      OVec.Rx = cth*rhy - sth*rhx;
      OVec.Ry = cla*rhz - sla*rht;
      OVec.Rz = cla*rht + sla*rhz;
/*
 *  Calculate the satellite azimuth and elevation at the Ground Station.
 */
      OVec.Az = RDG * Actan (OVec.Rx, OVec.Ry);
      if (OVec.Az < 0.0)
          OVec.Az += 360.0;
      OVec.El = RDG * atan (OVec.Rz / sqrt (OVec.Rx*OVec.Rx + OVec.Ry*OVec.Ry));
/*
 *  Calculate the range and range-rate.
 */
      OVec.RGS = sqrt (rhx*rhx + rhy*rhy + rhz*rhz);
      OVec.VGS = (rhx*(IVec->Vx + w*IVec->Ry) + rhy*(IVec->Vy - w*IVec->Rx) + rhz*IVec->Vz) / OVec.RGS;
      return (OVec);
}

TGSVector* TConvert::ToGS(TINVector *IVec, TGSRef GS, int NumVec)
{
  int i;
  TGSVector *OutVec;
  TGSVector *SaveVec;
  i = 0;

  OutVec = new TGSVector[NumVec];
  SaveVec = OutVec;
  for (i=0; i < NumVec; i++)
  {
    *OutVec++ = ToGS(IVec++, GS);
  }
  return(SaveVec);
}
