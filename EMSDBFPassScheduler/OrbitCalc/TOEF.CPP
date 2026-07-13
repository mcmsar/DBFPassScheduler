//---------------------------------------------------------------------------
#include <math.h>
#include "SgpxStr.h"
#include "Predict.h"

#pragma warning ( disable : 4244 )

//---------------------------------------------------------------------------

TEFVector TConvert::ToEF (TINVector *IVec)
{
    double cosA, sinA, w = (double)(PI*2.0/SID);
    double RxIC, RyIC, RzIC, VxIC, VyIC, VzIC,
          RxEF, RyEF, RzEF, VxEF, VyEF, VzEF;
    double thetag;

    TEFVector OVec;	// create output vector

    thetag = (double)(Gst(IVec->VTime));
    RxIC = IVec->Rx;
    RyIC = IVec->Ry;
    RzIC = IVec->Rz;
    VxIC = IVec->Vx;
    VyIC = IVec->Vy;
    VzIC = IVec->Vz;

    cosA = cos (thetag);
    sinA = sin (thetag);

    RxEF =  cosA*RxIC + sinA*RyIC;
    RyEF = -sinA*RxIC + cosA*RyIC;
    RzEF =  RzIC;
    VxEF =  cosA*VxIC + sinA*VyIC + w*RyEF;
    VyEF = -sinA*VxIC + cosA*VyIC - w*RxEF;
    VzEF =  VzIC;

    OVec.VTime = IVec->VTime;
    OVec.Rx = RxEF;
    OVec.Ry = RyEF;
    OVec.Rz = RzEF;
    OVec.Vx = VxEF;
    OVec.Vy = VyEF;
    OVec.Vz = VzEF;

    return(OVec);
}

TEFVector* TConvert::ToEF(TINVector *IVec, int NumVec)
{
  int i;
  TEFVector *OutVec;
  TEFVector *SaveVec;
  i = 0;

  OutVec = new TEFVector[NumVec];
  SaveVec = OutVec;
  for (i=0; i < NumVec; i++)
  {
    *OutVec++ = ToEF(IVec++);
  }
  return(SaveVec);
}
