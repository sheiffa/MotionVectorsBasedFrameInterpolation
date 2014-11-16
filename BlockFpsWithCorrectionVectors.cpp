#include "BlockFpsWithCorrectionVectors.h"
#include "MaskFun.h"

BlockFpsWithCorrectionVectors::BlockFpsWithCorrectionVectors(PClip source, PClip super, PClip backwardVectors, PClip forwardVectors, PClip errorVectors, int sourceFps, int fpsMultiplier,int mode,IScriptEnvironment* env) :
	MVBlockFps(source,super,backwardVectors,forwardVectors,sourceFps*fpsMultiplier,1,mode,0,true,MV_DEFAULT_SCD1,MV_DEFAULT_SCD2,true,false,env),
	errorVectors(errorVectors),
	fpsMultiplier(fpsMultiplier)
{

}

BlockFpsWithCorrectionVectors::~BlockFpsWithCorrectionVectors(){
}

inline BYTE MEDIAN(BYTE a, BYTE b, BYTE c)
{
	BYTE mn = min(a, b);
	BYTE mx = max(a, b);
	BYTE m = min(mx, c);
	m = max(mn, m);
	return m;
}

void BlockFpsWithCorrectionVectors::ResultBlock(BYTE *pDst, int dst_pitch, const BYTE * pMCB, int MCB_pitch, const BYTE * pMCF, int MCF_pitch,
	const BYTE * pRef, int ref_pitch, const BYTE * pSrc, int src_pitch, BYTE *maskB, int mask_pitch, BYTE *maskF,
	BYTE *pOcc, int nBlkSizeX, int nBlkSizeY, int time256, int mode)
{
	if (mode==0)
	{
		for (int h=0; h<nBlkSizeY; h++)
		{
			for (int w=0; w<nBlkSizeX; w++)
			{
					int mca =   (pMCB[w]*time256 + pMCF[w]*(256-time256)) >> 8 ; // MC fetched average
					pDst[w] = mca;
			}
			pDst += dst_pitch;
			pMCB += MCB_pitch;
			pMCF += MCF_pitch;
		}
	}
	else if (mode==1) // default, best working mode
	{
		for (int h=0; h<nBlkSizeY; h++)
		{
			for (int w=0; w<nBlkSizeX; w++)
			{
					int mca =   (pMCB[w]*time256 + pMCF[w]*(256-time256)) >> 8 ; // MC fetched average
					int sta =  MEDIAN(pRef[w], pSrc[w], mca); // static median
					pDst[w] = sta;

			}
			pDst += dst_pitch;
			pMCB += MCB_pitch;
			pMCF += MCF_pitch;
			pRef += ref_pitch;
			pSrc += src_pitch;
		}
	}
	else if (mode==2) // default, best working mode
	{
		for (int h=0; h<nBlkSizeY; h++)
		{
			for (int w=0; w<nBlkSizeX; w++)
			{
					int avg =   (pRef[w]*time256 + pSrc[w]*(256-time256)) >> 8 ; // simple temporal non-MC average
					int dyn =	MEDIAN(avg, pMCB[w], pMCF[w]); // dynamic median
					pDst[w] = dyn;
			}
			pDst += dst_pitch;
			pMCB += MCB_pitch;
			pMCF += MCF_pitch;
			pRef += ref_pitch;
			pSrc += src_pitch;
		}
	}
	else if (mode==3)
	{
		for (int h=0; h<nBlkSizeY; h++)
		{
			for (int w=0; w<nBlkSizeX; w++)
			{
					pDst[w] =  ( ( (maskB[w]*pMCF[w] + (255-maskB[w])*pMCB[w] + 255)>>8 )*time256 +
					             ( (maskF[w]*pMCB[w] + (255-maskF[w])*pMCF[w] + 255)>>8 )*(256-time256) ) >> 8;
			}
			pDst += dst_pitch;
			pMCB += MCB_pitch;
			pMCF += MCF_pitch;
			pRef += ref_pitch;
			pSrc += src_pitch;
			maskB += mask_pitch;
			maskF += mask_pitch;
		}
	}
	else if (mode==4)
	{
		for (int h=0; h<nBlkSizeY; h++)
		{
			for (int w=0; w<nBlkSizeX; w++)
			{
					int f = (maskF[w]*pMCB[w] + (255-maskF[w])*pMCF[w] + 255 )>>8;
					int b =    (maskB[w]*pMCF[w] + (255-maskB[w])*pMCB[w] + 255)>>8;
					int avg =   (pRef[w]*time256 + pSrc[w]*(256-time256) + 255) >> 8 ; // simple temporal non-MC average
					int m = ( b*time256 + f*(256-time256) )>>8;
					pDst[w]= ( avg * pOcc[w] + m * (255 - pOcc[w]) + 255 )>>8;
			}
			pDst += dst_pitch;
			pMCB += MCB_pitch;
			pMCF += MCF_pitch;
			pRef += ref_pitch;
			pSrc += src_pitch;
			maskB += mask_pitch;
			maskF += mask_pitch;
			pOcc += mask_pitch;
		}
	}
	else if (mode==5)
	{
		for (int h=0; h<nBlkSizeY; h++)
		{
			for (int w=0; w<nBlkSizeX; w++)
			{
					pDst[w]= pOcc[w];
			}
			pDst += dst_pitch;
			pOcc += mask_pitch;
		}
	}
}