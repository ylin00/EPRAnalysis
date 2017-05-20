#include <math.h>
#include <mex.h>

#include "jjj.h"

bool Display = false;

__inline int isodd(int k) { return (k % 2); }
__inline int parity(int k) { return (isodd(k) ? -1 : +1); }
__inline int mini(int a,int b) { return ((a<b) ? a : b); }

long maxElements = 5000000;
int maxRows = 100000;

const double sqrt12 = 0.70710678118655; /* sqrt(1.0/2.0); */
const double sqrt13 = 0.57735026918963; /* sqrt(1.0/3.0); */
const double sqrt23 = 0.81649658092773; /* sqrt(2.0/3.0); */

double *ridx, *cidx;
double *MatrixRe, *MatrixIm;

int nRows, nElements;

int Lemax, Lomax, Kmax, Mmax;
int jKmin, pSmin, pImax, deltaK;
int MeirovitchSymm;

struct SystemStruct {
  double I, NZI0, HFI0, *ReHFI2, *ImHFI2;
  double EZI0, *ReEZI2, *ImEZI2;
  double *d2psi;
  double DirTilt;
};

struct DiffusionStruct {
  double* xlk;
  double Rxx, Ryy, Rzz;
  double Exchange;
  int maxL;
};

struct SystemStruct Sys;
struct DiffusionStruct Diff;

/*============================================================================ */
/*============================================================================ */
/*============================================================================ */
/*============================================================================ */
void makematrix()
{

const double EZI0 = Sys.EZI0;
const double *ReEZI2 = Sys.ReEZI2;
const double *ImEZI2 = Sys.ImEZI2;

const double I = Sys.I;
const double NZI0 = Sys.NZI0;
const double HFI0 = Sys.HFI0;
const double *ReHFI2 = Sys.ReHFI2;
const double *ImHFI2 = Sys.ImHFI2;

/* Diffusion parameters */
/*--------------------------------------------- */
const double DirTilt = Sys.DirTilt;
const double *d2psi = Sys.d2psi;
const double Rxx = Diff.Rxx;
const double Ryy = Diff.Ryy;
const double Rzz = Diff.Rzz;

const double ExchangeFreq = Diff.Exchange;
const double *xlk = Diff.xlk;

/* Lband = (lptmx>=2) ? lptmx*2 : 2; */
const int Lband = 8;
/* Kband = kptmx*2; */
const int Kband = 8;
/*========================================================== */

/* Loop variables */
int L1, jK1, K1, M1, pS1, qS1, pI1, qI1;
int L2, jK2, K2, M2, pS2, qS2, pI2, qI2;
int Ld, Ls, jKd, Kd, Ks, KK;

/* Min and max values for loop variables */
int K1max, M1max, qS1max, qI1max;
int L2max, jK2min, K2min, K2max, M2min, M2max;
int pS2min, qS2min, qS2max, pI2min, qI2min, qI2max;

int iRow = 0, iCol = 0, iElement = 0;

bool Potential = Diff.maxL>=0;
bool ExchangePresent = (ExchangeFreq!=0);

double IsoDiffKdiag, IsoDiffKm2, IsoDiffKp2, PotDiff;
bool diagRC, Ld2, diagLK, diagS, diagI;
double N_L, N_K, NormFactor;
double R_EZI2, R_HFI2, a1, g1, a2, g2;
int parityLK2, L, pId, qId;
double Term1, Term2, X;
int pd;
bool includeRank0;
double LiouvilleElement, GammaElement, t;
double d2jjj;

const bool RhombicDiff = (Rxx!=Ryy);

if (Display) {
  mexPrintf("   Potential: %d   Exchange: %d\n",Potential, ExchangePresent);
mexPrintf("   Lemax Lomax:   %d  %d  %d\n",Lemax,Lomax);
mexPrintf("   jKmin Kmax Mmax:   %d  %d  %d\n",jKmin,Kmax,Mmax);
mexPrintf("   pSmin  pImax:   %d  %d\n",pSmin, pImax);
}

/* All equation numbers refer to Meirovich et al, J.Chem.Phys. 77 (1982) */

for (L1=0;L1<=Lemax;L1++) {
  if (isodd(L1) && (L1>Lomax)) continue;
  for (jK1=jKmin;jK1<=1;jK1+=2) {
    K1max = mini(Kmax,L1);
    for (K1=0;K1<=K1max;K1+=deltaK) {
      if ((K1==0)&&(parity(L1)!=jK1)) continue;

      /* Potential-independent part of diffusion operator */
      /*-------------------------------------------------------- */
      /* depends only on L and K and is diagonal in all except K */
      IsoDiffKdiag = (Rxx+Ryy)/2*(L1*(L1+1))+K1*K1*(Rzz-(Rxx+Ryy)/2);
      if (RhombicDiff) {
        KK = K1-2;
        IsoDiffKm2 = (Rxx-Ryy)/4*sqrt((L1-KK-1)*(L1-KK)*(L1+KK+1)*(L1+KK+2));
        KK = K1+2;
        IsoDiffKp2 = (Rxx-Ryy)/4*sqrt((L1+KK-1)*(L1+KK)*(L1-KK+1)*(L1-KK+2));
      }
      else {
        IsoDiffKp2 = IsoDiffKm2 = 0;
      }
      /*-------------------------------------------------------- */
      
      
      M1max = mini(Mmax,L1);
      for (M1=-M1max;M1<=M1max;M1++) {
        
        for (pS1=pSmin;pS1<=1;pS1++) {
          qS1max = 1 - abs(pS1);
          for (qS1=-qS1max;qS1<=qS1max;qS1+=2) {
            for (pI1 = -pImax;pI1<=pImax;pI1++) {
              if ((MeirovitchSymm) && (DirTilt==0) && ((pI1+pS1-M1)!=1)) continue;  /* Eq. (A47) */
              qI1max = ((int)(2*I)) - abs(pI1);
              for (qI1=-qI1max;qI1<=qI1max;qI1+=2) {
                iCol = iRow;
                diagRC = true;
                L2max = mini(Lemax,L1+Lband);
                for (L2=L1;L2<=L2max;L2++) {
                  if (isodd(L2)&&(L2>Lomax)) continue;
                  Ld = L1 - L2; Ls = L1 + L2;
                  Ld2 = abs(Ld)<=2;

                  /* N_L normalisation factor, see after Eq. (A11) */
                  N_L = sqrt((double)((2.0*L1+1.0)*(2.0*L2+1.0)));

                  jK2min  = (diagRC) ?  jK1 : jKmin;
                  for (jK2=jK2min;jK2<=1;jK2+=2) {
                    jKd = jK1 - jK2;
                    K2max = mini(Kmax,L2);
                    K2min = (diagRC) ? K1 : 0;
                    for (K2=K2min;K2<=K2max;K2+=deltaK) {
                      if ((K2==0)&&(parity(L2)!=jK2)) continue;
                      diagLK = (L1==L2) && (K1==K2);
                      Kd = K1 - K2;
                      Ks = K1 + K2;
                      parityLK2 = parity(L2+K2);
                      
                      /*---------------------------------------------------------------------- */
                      /* Pre-calculations for Liouville matrix elements, Eq. (A41)             */
                      /*---------------------------------------------------------------------- */
                      /* R(mu=EZI,HFI;l=2), see Eq. (A42) and (A44) */
                      /*---------------------------------------------- */
                      R_EZI2 = 0; R_HFI2 = 0;
                      if (Ld2) {
                        g1 = 0; a1 = 0;
                        if (abs(Kd)<=2) {
                          const double coeff = jjj(L1,2,L2,K1,-Kd,-K2);
                          /*mexPrintf("[wigner3j(%d,%d,%d,%d,%d,%d) %g]\n",L1,2,L2,K1,-Kd,-K2,coeff); */
                          if (jK1==jK2) {
                            g1 = coeff*ReEZI2[Kd+2];
                            a1 = coeff*ReHFI2[Kd+2];
                          }
                          else {
                            if (ImEZI2) g1 = coeff*ImEZI2[Kd+2]*jK1;
                            if (ImHFI2) a1 = coeff*ImHFI2[Kd+2]*jK1;
                          }
                        }
                        g2 = 0; a2 = 0;
                        if (abs(Ks)<=2) {
                          const double coeff = jjj(L1,2,L2,K1,-Ks,K2);
                          if (jK1==jK2) {
                            g2 = coeff*ReEZI2[Ks+2];
                            a2 = coeff*ReHFI2[Ks+2];
                          }
                          else {
                            if (ImEZI2) g2 = coeff*ImEZI2[Ks+2]*jK1;
                            if (ImHFI2) a2 = coeff*ImHFI2[Ks+2]*jK1;
                          }
                        }
                        R_EZI2 = g1 + jK2*parityLK2*g2;
                        R_HFI2 = a1 + jK2*parityLK2*a2;
                      }

                      /* N_K(K_1,K_2)  normalization factor, Eq. (A43) */
                      N_K = 1.0;
                      if (K1==0) N_K /= sqrt(2.0);
                      if (K2==0) N_K /= sqrt(2.0);
                      
                      /* Normalisation prefactor in Eq.(A40) and Eq.(A41) */
                      NormFactor = N_L*N_K*parity(M1+K1);
                      /*---------------------------------------------------------------------- */
                      
                      /*---------------------------------------------------------------------- */
                      /* Potential-dependent term of diffusion operator, Eq. (A40)             */
                      /*---------------------------------------------------------------------- */
                      PotDiff = 0;
                      if (Potential) {
                        if ((abs(Ld)<=Lband)&&(parity(Ks)==1)&&
                            (jKd==0)&&(abs(Kd)<=Kband)&&(abs(Ks)<=Kband)) {
                          for (L=0; L<=Lband; L+=2) {
                            Term1 = 0;
                            if (Kd>=-L) {
                              X = xlk[(Kd+L)*(Diff.maxL+1) + L]; /* X^L_{K1-K2} */
                              if (X!=0)
                                Term1 = X * jjj(L1,L,L2,K1,-Kd,-K2);
                            }
                            Term2 = 0;
                            if (Ks<=L) {
                              X = xlk[(Ks+L)*(Diff.maxL+1) + L]; /* X^L_{K1+K2} */
                              if (X!=0)
                                Term2 = parityLK2*jK2* X * jjj(L1,L,L2,K1,-Ks,K2);
                            }
                            if (Term1 || Term2)
                              PotDiff += (Term1+Term2) * jjj(L1,L,L2,M1,0,-M1);
                          }
                          PotDiff *= NormFactor;
                          
                          if (Display)
                            if (abs(PotDiff)>1e-10)
                              mexPrintf(" pot (%d,%d;%d,%d): %e\n",L1,L2,K1,K2,PotDiff);
                        }
                      }
                      /*---------------------------------------------------------------------- */
                      
                      M2max = mini(Mmax,L2);
                      M2min = (diagRC) ? M1 : -M2max;
                      for (M2=M2min;M2<=M2max;M2++) {
                        int Md = M1 - M2;
                        
                        bool diagLKM = diagLK && (jKd==0) && (Md==0);

                        /* Pre-compute 3j symbol in Eq. (A41) for l = 2 */
                        const double Liou3j = (Ld2) ? jjj(L1,2,L2,M1,-Md,-M2) : 0;
                        
                        pS2min = (diagRC) ? pS1 : pSmin;
                        for (pS2=pS2min;pS2<=1;pS2++) {
                          int pSd = pS1 - pS2;
                          qS2max = 1 - abs(pS2);
                          qS2min = (diagRC) ? qS1 : -qS2max;
                          for (qS2=qS2min;qS2<=qS2max;qS2+=2) {
                            int qSd = qS1 - qS2;
                            diagS = (pS1==pS2) && (qS1==qS2);
                            
                            pI2min = (diagRC) ? pI1 : -pImax;
                            for (pI2=pI2min;pI2<=pImax;pI2++) {
                              if ((MeirovitchSymm) && (DirTilt==0) && ((pI2+pS2-M2)!=1)) continue;  /* Eq. (A47) */
                              pId = pI1 - pI2;
                              qI2max = ((int)(2*I)) - abs(pI2);
                              qI2min = (diagRC) ? qI1 : -qI2max;
                              for (qI2=qI2min;qI2<=qI2max;qI2+=2) {
                                qId = qI1 - qI2;
                                diagI = (pId==0) && (qId==0);
                                pd = pSd + pId;

                                /*---------------------------------------------------------------------------- */
                                /* Matrix element of Liouville operator (Hamiltonian superoperator)            */
                                /*---------------------------------------------------------------------------- */
                                /* For the rank-0 terms in (A41), the following simplifcations hold. */
                                /* - L1==L2, K1==K2 and M1==M2, otherwise the 3j are zero */
                                /* - The prefactor N_L*(-1)^(M1+K1) is canceled by the product of */
                                /*   wigner3j(L,M;0,0;L,-M) within the sum and wigner3j(L,K;0,0;L,-K) from R. */
                                /*    ( wigner3j(L,M;0,0;L,-M) = (-1)^(-M-L)/sqrt(2L+1)    */
                                /* - The N_K factor is not needed because */
                                /*   the l=0 ISTO components have not been transformed by */
                                /*   the K-symmetrization. (following the formula, N_K is */
                                /*   cancelled by R_0) */
                                includeRank0 = diagLKM && (pd==0);
                                
                                LiouvilleElement = 0;
                                
                                if (Ld2 && (abs(pSd)<=1) && (abs(pId)<=1) && 
                                   /*((DirTilt!=0) || (pd==Md)) &&*/
                                   (abs(Md)<=2) &&
                                   (abs(pSd)==abs(qSd)) && (abs(pId)==abs(qId))) {
                                  
                                  d2jjj = d2psi[(pd+2)+(Md+2)*5]*Liou3j;
                                  
                                  /* Electronic Zeeman interaction */
                                  /*----------------------------------------- */
                                  if (diagI) { /* i.e. pId==0 and qId==0 for all nuclei */
                                    /* Rank-2 term, Eq. (B7) */
                                    /* Compute Clebsch-Gordan coeffs and S_g Eq. (B8) */
                                    double C2, S_g;
                                    if (pSd==0) {
                                      C2 = +sqrt23; /* (112|000) */
                                      S_g = pS1;
                                    }
                                    else {
                                      C2 = +sqrt12;  /* (112|-10-1), (112|101) */
                                      S_g = -qSd/sqrt(2.0);
                                    }
                                    LiouvilleElement += NormFactor*d2jjj*R_EZI2*(C2*S_g);
                                    /* Rank-0 term */
                                    if (includeRank0) {
                                      const double C0 = -sqrt13; /* (110|000) */
                                      LiouvilleElement += EZI0*(C0*pS1);
                                    }
                                  }
                                  
                                  /* Hyperfine interaction, nucleus 1 */
                                  /*----------------------------------------- */
                                  if ((I>0) && (pSd*pId==qSd*qId)) {
                                    /* Compute Clebsch-Gordan coeffs and S_A from Eq. (B7) */
                                    double C0, C2, S_A;
                                    if (pId==0) {
                                      if (pSd==0) {
                                        S_A = (pS1*qI1+pI1*qS1)/2.0;
                                        C0 = -sqrt13; /* (110|000) */
                                        C2 = +sqrt23; /* (112|000) */
                                      }
                                      else {
                                        S_A = -(pI1*pSd+qI1*qSd)/sqrt(8.0);
                                        C0 = 0; /* no rank 0 term */
                                        C2 = +sqrt12; /* (112|101), (112|-10-1) */
                                      }
                                    }
                                    else {
                                      int t = qI1*qId + pI1*pId;
                                      double KI = sqrt(I*(I+1.0)-t*(t-2.0)/4.0);
                                      if (pSd==0) {
                                        S_A = -(pS1*pId+qS1*qId)*KI/sqrt(8.0);
                                        C0 = 0; /* no rank 0 term */
                                        C2 = +sqrt12; /* (112|011), (112|0-1-1) */
                                      }
                                      else {
                                        S_A = pSd*qId*KI/2.0;
                                        C0 = +sqrt13; /* (110|1-10), (110|-110) */
                                        if (pd==0)
                                          C2 = +sqrt(1.0/6.0); /* (112|1-10), (112|-110) */
                                        else
                                          C2 = +1.0; /* (112|112), (112|-1-1-2) */
                                      }
                                    }
                                    /* Rank-2 term, Eq. (A41) */
                                    LiouvilleElement += NormFactor*d2jjj*R_HFI2 * C2*S_A; /* Eq. (A41) and (B7)*/
                                    /* Rank-0 term */
                                    if (includeRank0)
                                      LiouvilleElement += HFI0*C0*S_A;
                                  }
                                                                    
                                  /* Nuclear Zeeman interaction */
                                  /*----------------------------------------- */
                                  /* has only a rank-0 component */
                                  if (diagS && diagI && includeRank0) {
                                    const double C0 = -sqrt13; /* (110|000) */
                                    LiouvilleElement += NZI0*C0*pI1;
                                  }
                                  
                                }
                                /*------------------------------------------- */
                                
                                
                                /*------------------------------------------------------ */
                                /* Matrix element of diffusion superoperator             */
                                /*------------------------------------------------------ */
                                GammaElement = 0;
                                if (diagS && diagI) { /* all potential terms are diagonal in the spin space */
                                  /* Potential-independent terms, Eq. (A15) */
                                  if ((Ld==0) && (Md==0) && (jKd==0)) {
                                    if (Kd==0) GammaElement += IsoDiffKdiag;
                                    else if (Kd==+2) GammaElement += IsoDiffKm2/N_K;
                                    else if (Kd==-2) GammaElement += IsoDiffKp2/N_K;
                                  }                        
                                  /* Potential-dependent terms, Eq. (A40) */
                                  if (Potential) {
                                    if ((Md==0) && (jKd==0))
                                      GammaElement += PotDiff;
                                  }
                                }
                                
                                /* Exchange term */
                                if (ExchangePresent) {
                                  if ((pSd==0) && (pId==0) && diagLKM) {
                                    t = 0;
                                    if ((qId==0) && (qSd==0)) t += 1.0;
                                    if ((qId==0) && (pS1==0)) t -= 0.5;
                                    if ((pI1==0) && (qSd==0)) t -= 1.0/(2.0*I+1);
                                    GammaElement += t*ExchangeFreq;
                                  }
                                }
                                /*------------------------------------------- */

                                
                                /*------------------------------------------- */
                                /* Store element values and indices           */
                                /*------------------------------------------- */
                                if ((GammaElement!=0) || (LiouvilleElement!=0)) {
                                  MatrixRe[iElement] = GammaElement;
                                  MatrixIm[iElement] = -LiouvilleElement;
                                  ridx[iElement] = iRow;
                                  cidx[iElement] = iCol;
                                  iElement++;
                                  if (!diagRC) {
                                    MatrixRe[iElement] = GammaElement;
                                    MatrixIm[iElement] = -LiouvilleElement;
                                    ridx[iElement] = iCol;
                                    cidx[iElement] = iRow;
                                    iElement++;
                                  }
                                  if (iElement>=maxElements)
                                    mexErrMsgTxt("Number of non-zero elements too large. Increase Opt.Allocation(1).");
                                }
                                iCol++;
                                diagRC = false;
                                if (iCol>=maxRows)
                                  mexErrMsgTxt("Dimension too large. Increase Opt.Allocation(2).");
                                
                              } /* qI2 */
                            } /* pI2 */ 
                          } /* qS2 */
                        } /* pS2 */
                      } /* M2 */
                    } /* K2 */
                  } /* jK2 */
                } /* L2 */ /* all column index loops */

                iRow++;
                if (iRow>=maxRows)
                  mexErrMsgTxt("Dimension too large. Increase Opt.Allocation(2).");
              } /* qI1 */
            } /* pI1 */
          } /* qS1 */
        } /* pS1 */
      } /* M1 */
    } /* K1 */
  } /* jK1 */
} /* L1; all row index loops */
nRows = iRow;
nElements = iElement;

}


/*======================================================================= */
/* Determine basis size */
int BasisSize()
/*======================================================================= */
{

/* Loop variables */
int L1, jK1, K1, M1, pS1, qS1, pI1, qI1;

/* Min and max values for loop variables */
int K1max, M1max, qS1max, qI1max;

int iRow = 0;
double I = Sys.I;

for (L1=0;L1<=Lemax;L1++) {
  if (isodd(L1) && (L1>Lomax)) continue;
  for (jK1=jKmin;jK1<=1;jK1+=2) {
    K1max = mini(Kmax,L1);
    for (K1=0;K1<=K1max;K1+=deltaK) {
      if ((K1==0)&(parity(L1)!=jK1)) continue;
      M1max = mini(Mmax,L1);
      for (M1=-M1max;M1<=M1max;M1++) {
        for (pS1=pSmin;pS1<=1;pS1++) {
          qS1max = 1 - abs(pS1);
          for (qS1=-qS1max;qS1<=qS1max;qS1+=2) {
            for (pI1 = -pImax;pI1<=pImax;pI1++) {
              if ((MeirovitchSymm) && (Sys.DirTilt==0)&&((pI1+pS1-1)!=M1)) continue;
              qI1max = (int)(2*I) - abs(pI1);
              for (qI1=-qI1max;qI1<=qI1max;qI1+=2) {
                /*mexPrintf("%3d %3d %3d %3d    %2d %2d %2d %2d\n",L1,jK1,K1,M1,pS1,qS1,pI1,qI1);*/
                iRow++;
              } /* qI1 */
            } /* pI1 */
          } /* qS1 */
        } /* pS1 */
		
      } /* M1 */
    } /* K1 */
  } /* jK1 */
} /* L1 */

return iRow; /* number of rows */

}

/*============================================================================ */
/*============================================================================ */
/*============================================================================ */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{

  mxArray *T, *D;
  double *R;
  double *basisopts, *allocationOptions;
  int N, idxS, idxB, idxD;
  if (nrhs==1) {
    double *jm = mxGetPr(prhs[0]);
    /*mexPrintf("%0.10f\n",jjj(jm[0],jm[1],jm[2],jm[3],jm[4],jm[5])); */
    return;
  }

  if (nrhs!=4) mexErrMsgTxt("4 input arguments expected!");
  if (nlhs!=5) mexErrMsgTxt("3 output arguments expected!");

  if (Display) mexPrintf("Parsing system...\n");
  idxS = 0;
  Sys.I = mxGetScalar(mxGetField(prhs[idxS],0,"I"));
  Sys.EZI0 = mxGetScalar(mxGetField(prhs[idxS],0,"EZ0"));
  Sys.NZI0 = mxGetScalar(mxGetField(prhs[idxS],0,"NZ0"));
  Sys.HFI0 = mxGetScalar(mxGetField(prhs[idxS],0,"HF0"));
  Sys.DirTilt = mxGetScalar(mxGetField(prhs[idxS],0,"DirTilt"));
  Sys.d2psi = mxGetPr(mxGetField(prhs[idxS],0,"d2psi"));

  T = mxGetField(prhs[idxS],0,"EZ2");
  Sys.ReEZI2 = mxGetPr(T);
  Sys.ImEZI2 = mxGetPi(T);
  T = mxGetField(prhs[idxS],0,"HF2");
  Sys.ReHFI2 = mxGetPr(T);
  Sys.ImHFI2 = mxGetPi(T);
  
  /*mexPrintf("Parsing basis structure...\n"); */
  idxB = 1;
  /*Lemax = (int)mxGetScalar(mxGetField(prhs[idxB],0,"evenLmax"));
  Lomax = (int)mxGetScalar(mxGetField(prhs[idxB],0,"oddLmax"));
  Kmax = (int)mxGetScalar(mxGetField(prhs[idxB],0,"Kmax"));
  Mmax = (int)mxGetScalar(mxGetField(prhs[idxB],0,"Mmax"));
  jKmin = (int)mxGetScalar(mxGetField(prhs[idxB],0,"jKmin"));
  pSmin = (int)mxGetScalar(mxGetField(prhs[idxB],0,"pSmin"));
  pImax = (int)mxGetScalar(mxGetField(prhs[idxB],0,"pImax"));
  deltaK = (int)mxGetScalar(mxGetField(prhs[idxB],0,"deltaK"));
  */
  basisopts = mxGetPr(prhs[1]);
  
  Lemax = (int)basisopts[0];
  Lomax = (int)basisopts[1];
  Kmax = (int)basisopts[2];
  Mmax = (int)basisopts[3];
  jKmin = (int)basisopts[4];
  pSmin = (int)basisopts[5];
  deltaK = (int)basisopts[6];
  MeirovitchSymm = (int)basisopts[7];
  pImax = (int)basisopts[8];

  /*
  mexPrintf("(%d %d %d %d) jKmin %d, pSmin %d, pImax %d, deltaK %d\n",
    Lemax,Lomax,Kmax,Mmax,jKmin,pSmin,pImax,deltaK);
  */
  
  /*mexPrintf("Parsing diffusion structure...\n"); */
  idxD = 2;
  Diff.Exchange = mxGetScalar(mxGetField(prhs[idxD],0,"Exchange"));
  Diff.xlk = mxGetPr(mxGetField(prhs[idxD],0,"xlk"));
  Diff.maxL = mxGetScalar(mxGetField(prhs[idxD],0,"maxL"));
  
  R = mxGetPr(mxGetField(prhs[idxD],0,"Diff"));
  Diff.Rxx = R[0];
  Diff.Ryy = R[1];
  Diff.Rzz = R[2];

  allocationOptions = mxGetPr(prhs[3]);
  maxElements = (long)allocationOptions[0];
  maxRows = (long)allocationOptions[1];
  if (Display) mexPrintf("  max elements: %d,  max rows: %d\n",maxElements,maxRows);
  
  N = BasisSize();
  maxRows = N+1;

  if (Display) mexPrintf("  basis function count: %d\n",N);
  
  plhs[0] = mxCreateDoubleMatrix(maxElements,1,mxREAL);
  plhs[1] = mxCreateDoubleMatrix(maxElements,1,mxREAL);
  plhs[2] = mxCreateDoubleMatrix(maxElements,1,mxCOMPLEX);
  ridx = mxGetPr(plhs[0]);
  cidx = mxGetPr(plhs[1]);
  MatrixRe = mxGetPr(plhs[2]);
  MatrixIm = mxGetPi(plhs[2]);
  
  if (Display) mexPrintf("  starting matrix make...\n");

  makematrix();

  if (Display) mexPrintf("  finishing matrix make...\n");

  plhs[3] = mxCreateDoubleScalar(nRows);
  plhs[4] = mxCreateDoubleScalar(nElements);
  
  return;
}
