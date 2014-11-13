#include "BlockFpsWithCorrectionVectors.h"
#include "MaskFun.h"

BlockFpsWithCorrectionVectors::BlockFpsWithCorrectionVectors(PClip source, PClip super, PClip backwardVectors, PClip forwardVectors, PClip errorVectors, int fpsMultiplier,IScriptEnvironment* env) :
	GenericVideoFilter(source),
	MVFilter(backwardVectors, "BlockFpsWithCorrectionVectors", env),
	super(super),
	backwardVectors(backwardVectors, MV_DEFAULT_SCD1, MV_DEFAULT_SCD2, env),
	forwardVectors(forwardVectors, MV_DEFAULT_SCD1, MV_DEFAULT_SCD2, env),
	errorVectors(errorVectors),
	fpsMultiplier(fpsMultiplier)
{
	if (!vi.IsYV12() && !vi.IsYUY2())
		env->ThrowError("MBlockFps: Clip must be YV12 or YUY2");

    if (nOverlapX!=0 || nOverlapX!=0)
		env->ThrowError("MBlockFps: Overlap must be 0");

	setOutputFps(fpsMultiplier);

	thres = nBlkSizeX*nBlkSizeY/4; // threshold for count of occlusions per block

	getParamsFromSuperclip(env);

	setCopyFunctions();

	// may be padded for full frame cover
	nBlkXP = (nBlkX*(nBlkSizeX - nOverlapX) + nOverlapX < nWidth) ? nBlkX+1 : nBlkX;
	nBlkYP = (nBlkY*(nBlkSizeY - nOverlapY) + nOverlapY < nHeight) ? nBlkY+1 : nBlkY;
	nWidthP = nBlkXP*(nBlkSizeX - nOverlapX) + nOverlapX;
	nHeightP = nBlkYP*(nBlkSizeY - nOverlapY) + nOverlapY;
	// for YV12
	nWidthPUV = nWidthP/2;
	nHeightPUV = nHeightP/yRatioUV;
	nHeightUV = nHeight/yRatioUV;
	nWidthUV = nWidth/2;

	nPitchY = (nWidthP + 15) & (~15);
	nPitchUV = (nWidthPUV + 15) & (~15);

	//set masks
	MaskFullYB = new BYTE [nHeightP*nPitchY];
	MaskFullUVB = new BYTE [nHeightPUV*nPitchUV];
	MaskFullYF = new BYTE [nHeightP*nPitchY];
	MaskFullUVF = new BYTE [nHeightPUV*nPitchUV];

	MaskOccY = new BYTE [nHeightP*nPitchY];
	MaskOccUV = new BYTE [nHeightPUV*nPitchUV];

	smallMaskF = new BYTE [nBlkXP*nBlkYP];
	smallMaskB = new BYTE [nBlkXP*nBlkYP];
	smallMaskO = new BYTE [nBlkXP*nBlkYP];

    OnesBlock = new BYTE [nBlkSizeX*nBlkSizeY + 32]; // with some padding

    for (int j=0; j<nBlkSizeY; j++)
        for (int i=0; i<nBlkSizeX; i++)
            OnesBlock[j*nBlkSizeX + i] = 255; // put ones

	 int CPUF_Resize = env->GetCPUFlags();

	 upsizer = new SimpleResize(nWidthP, nHeightP, nBlkXP, nBlkYP, CPUF_Resize);
	 upsizerUV = new SimpleResize(nWidthPUV, nHeightPUV, nBlkXP, nBlkYP, CPUF_Resize);

	 if ((pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2)
		DstPlanes =  new YUY2Planes(nWidth, nHeight);
}


BlockFpsWithCorrectionVectors::~BlockFpsWithCorrectionVectors()
{
}

void BlockFpsWithCorrectionVectors::setOutputFps(int fpsMultiplier){
	numeratorOld = vi.fps_numerator;
	denominatorOld = vi.fps_denominator;

    if (fpsMultiplier !=0)
    {
        numerator = vi.fps_numerator*fpsMultiplier;
        denominator = 1;
    }
    else if (numeratorOld < (1<<30))
    {
        numerator = (numeratorOld<<1); // double fps by default
        denominator = denominatorOld;
    }
    else // very big numerator
    {
        numerator = numeratorOld;
        denominator = (denominatorOld>>1);// double fps by default
    }

    //  safe for big numbers since v2.1
    fa = __int64(denominator)*__int64(numeratorOld);
    fb = __int64(numerator)*__int64(denominatorOld);
    __int64 fgcd = gcd(fa, fb); // general common divisor
    fa /= fgcd;
    fb /= fgcd;

	vi.SetFPS(numerator, denominator);

	vi.num_frames = (int)(1 + __int64(vi.num_frames-1) * fb/fa );
}

void BlockFpsWithCorrectionVectors::getParamsFromSuperclip(IScriptEnvironment* env){
// get parameters of prepared super clip - v2.0
	SuperParams64Bits params;
    memcpy(&params, &super->GetVideoInfo().num_audio_samples, 8);
    int nHeightS = params.nHeight;
    nSuperHPad = params.nHPad;
    nSuperVPad = params.nVPad;
    int nSuperPel = params.nPel;
    nSuperModeYUV = params.nModeYUV;
    int nSuperLevels = params.nLevels;

    pRefBGOF = new MVGroupOfFrames(nSuperLevels, nWidth, nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, true, yRatioUV);
    pRefFGOF = new MVGroupOfFrames(nSuperLevels, nWidth, nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, true, yRatioUV);
    int nSuperWidth = super->GetVideoInfo().width;
    int nSuperHeight = super->GetVideoInfo().height;

    if (nHeight != nHeightS || nHeight != vi.height || nWidth != nSuperWidth-nSuperHPad*2 || nWidth != vi.width)
    		env->ThrowError("MBlockFps : wrong source or super frame size");
}

void BlockFpsWithCorrectionVectors::setCopyFunctions(){
      switch (nBlkSizeX)
      {
      case 32:
      if (nBlkSizeY==16) {
         BLITLUMA = Copy32x16_mmx;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy16x8_mmx;
		 } else {
	         BLITCHROMA = Copy16x16_mmx;
		 }
      } else if (nBlkSizeY==32) {
         BLITLUMA = Copy32x32_mmx;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy16x16_mmx;
		 } else {
	         BLITCHROMA = Copy16x32_mmx;
		 }
      } break;
      case 16:
      if (nBlkSizeY==16) {
         BLITLUMA = Copy16x16_mmx;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy8x8_mmx;
		 } else {
	         BLITCHROMA = Copy8x16_mmx;
		 }
      } else if (nBlkSizeY==8) {
         BLITLUMA = Copy16x8_mmx;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy8x4_mmx;
		 } else {
	         BLITCHROMA = Copy8x8_mmx;
		 }
      } else if (nBlkSizeY==2) {
         BLITLUMA = Copy16x2_mmx;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy8x1_mmx;
		 } else {
	         BLITCHROMA = Copy8x2_mmx;
		 }
      }
         break;
      case 4:
		 BLITLUMA = Copy4x4_mmx;
		 if (yRatioUV==2) {
			 BLITCHROMA = Copy2x2_mmx;
		 } else {
			 BLITCHROMA = Copy2x4_mmx;
		 }
         break;
      case 8:
      default:
      if (nBlkSizeY==8) {
         BLITLUMA = Copy8x8_mmx;
		 if (yRatioUV==2) {
	         BLITCHROMA = Copy4x4_mmx;
		 } else {
	         BLITCHROMA = Copy4x8_mmx;
		 }
      } else if (nBlkSizeY==4) { // 8x4
         BLITLUMA = Copy8x4_mmx;
		 if (yRatioUV==2) {
			 BLITCHROMA = Copy4x2_mmx; // idem
		 } else {
			 BLITCHROMA = Copy4x4_mmx; // idem
		 }
      }
      }
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
					int dyn =  MEDIAN(avg, pMCB[w], pMCF[w]); // dynamic median
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



void BlockFpsWithCorrectionVectors::extractYUYFrame(PVideoFrame frame, const BYTE* channels[], int pitches[]){
	channels[0] = YWPLAN(frame);
	channels[1] = UWPLAN(frame);
	channels[2] = VWPLAN(frame);
	pitches[0] = YPITCH(frame);
	pitches[1] = UPITCH(frame);
	pitches[2] = VPITCH(frame);
}

void BlockFpsWithCorrectionVectors::extractYUY2Frame(PVideoFrame frame, const BYTE* channels[], int pitches[]){
	channels[0] = frame->GetReadPtr();
    channels[1] = channels[0] + frame->GetRowSize()/2;
    channels[2] = channels[1] + frame->GetRowSize()/4;
    pitches[0] = frame->GetPitch();
	pitches[1] = pitches[0];
    pitches[2] = pitches[0];
}



PVideoFrame __stdcall BlockFpsWithCorrectionVectors::GetFrame(int n, IScriptEnvironment* env){
	int nHeightUV = nHeight/yRatioUV;
	int nWidthUV = nWidth/2;

	// intermediate product may be very large!
 	int nleft = (int) ( __int64(n)* fa/fb );
	int time256 = int( (double(n)*double(fa)/double(fb) - nleft)*256 + 0.5);

	int off = backwardVectors.GetDeltaFrame(); // integer offset of reference frame
	//usually off must be = 1
	if (off > 1)
		time256 = time256/off;

	int nright = nleft+off;

	//check if first or last frame
	if (time256 ==0)
        return child->GetFrame(nleft,env); // simply left
	else if (time256==256)
		return child->GetFrame(nright,env); // simply right


	//set motion vectors clips to proper frames
	PVideoFrame mvF = forwardVectors.GetFrame(nright, env);
	forwardVectors.Update(mvF, env);// forward from current to next
	mvF = 0;

	PVideoFrame mvB = backwardVectors.GetFrame(nleft, env);
	backwardVectors.Update(mvB, env);// backward from next to current
	mvB = 0;

	//save source and reference frames
	PVideoFrame	src	= super->GetFrame(nleft, env);
	PVideoFrame ref = super->GetFrame(nright, env);//  ref for backward compensation

	//create output frame
	PVideoFrame dst = env->NewVideoFrame(vi);


	BYTE *pDst[3];
	const BYTE *pRef[3], *pSrc[3];
    int nDstPitches[3], nRefPitches[3], nSrcPitches[3];
	unsigned char *pDstYUY2;
	int nDstPitchYUY2;
	if ( backwardVectors.IsUsable() && forwardVectors.IsUsable() )
	{
		PROFILE_START(MOTION_PROFILE_YUY2CONVERT);

		//extract YUY/YUY2 data from frame
		if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
		{
			extractYUY2Frame(src,pSrc,nSrcPitches);
			extractYUY2Frame(ref,pRef,nRefPitches);

			//dst frame needs special extracting
			pDstYUY2 = dst->GetWritePtr();
			nDstPitchYUY2 = dst->GetPitch();
			pDst[0] = DstPlanes->GetPtr();
			pDst[1] = DstPlanes->GetPtrU();
			pDst[2] = DstPlanes->GetPtrV();
			nDstPitches[0]  = DstPlanes->GetPitch();
			nDstPitches[1]  = DstPlanes->GetPitchUV();
			nDstPitches[2]  = DstPlanes->GetPitchUV();
			
		}
		else
		{
			extractYUYFrame(dst,pDst,nDstPitches); //possible error due to const modifier?
			extractYUYFrame(ref,pRef,nRefPitches);
			extractYUYFrame(src,pSrc,nSrcPitches);
		}
		PROFILE_STOP(MOTION_PROFILE_YUY2CONVERT);

		pRefBGOF->Update(YUVPLANES, (BYTE*)pRef[0], nRefPitches[0], (BYTE*)pRef[1], nRefPitches[1], (BYTE*)pRef[2], nRefPitches[2]);// v2.0
		pRefFGOF->Update(YUVPLANES, (BYTE*)pSrc[0], nSrcPitches[0], (BYTE*)pSrc[1], nSrcPitches[1], (BYTE*)pSrc[2], nSrcPitches[2]);

		MVPlane *pPlanesB[3];
		MVPlane *pPlanesF[3];

		pPlanesB[0] = pRefBGOF->GetFrame(0)->GetPlane(YPLANE);
		pPlanesB[1] = pRefBGOF->GetFrame(0)->GetPlane(UPLANE);
		pPlanesB[2] = pRefBGOF->GetFrame(0)->GetPlane(VPLANE);

		pPlanesF[0] = pRefFGOF->GetFrame(0)->GetPlane(YPLANE);
        pPlanesF[1] = pRefFGOF->GetFrame(0)->GetPlane(UPLANE);
        pPlanesF[2] = pRefFGOF->GetFrame(0)->GetPlane(VPLANE);

		MemZoneSet(MaskFullYB, 0, nWidthP, nHeightP, 0, 0, nPitchY); // put zeros
		MemZoneSet(MaskFullYF, 0, nWidthP, nHeightP, 0, 0, nPitchY);

		int dummyplane = PLANAR_Y; // always use it for resizer

        PROFILE_START(MOTION_PROFILE_COMPENSATION);
		int blocks = backwardVectors.GetBlkCount();

		int maxoffset = nPitchY*(nHeightP-nBlkSizeY)-nBlkSizeX;

		// pointers
		BYTE * pMaskFullYB = MaskFullYB;
		BYTE * pMaskFullYF = MaskFullYF;
		BYTE * pMaskFullUVB = MaskFullUVB;
		BYTE * pMaskFullUVF = MaskFullUVF;
		BYTE * pMaskOccY = MaskOccY;
		BYTE * pMaskOccUV = MaskOccUV;

		BYTE *pDstSave[3];
		pDstSave[0] = pDst[0];
		pDstSave[1] = pDst[1];
		pDstSave[2] = pDst[2];

		pSrc[0] += nSuperHPad + nSrcPitches[0]*nSuperVPad; // add offset source in super
		pSrc[1] += (nSuperHPad>>1) + nSrcPitches[1]*(nSuperVPad>>1);
		pSrc[2] += (nSuperHPad>>1) + nSrcPitches[2]*(nSuperVPad>>1);
		pRef[0] += nSuperHPad + nRefPitches[0]*nSuperVPad;
		pRef[1] += (nSuperHPad>>1) + nRefPitches[1]*(nSuperVPad>>1);
		pRef[2] += (nSuperHPad>>1) + nRefPitches[2]*(nSuperVPad>>1);

		// fetch image blocks
        for ( int i = 0; i < blocks; i++ )
        {
            const FakeBlockData &blockB = backwardVectors.GetBlock(0, i);
            const FakeBlockData &blockF = forwardVectors.GetBlock(0, i);

			// luma
            ResultBlock(pDst[0], nDstPitches[0],
               pPlanesB[0]->GetPointer(blockB.GetX() * nPel + ((blockB.GetMV().x*(256-time256))>>8), blockB.GetY() * nPel + ((blockB.GetMV().y*(256-time256))>>8)),
               pPlanesB[0]->GetPitch(),
               pPlanesF[0]->GetPointer(blockF.GetX() * nPel + ((blockF.GetMV().x*time256)>>8), blockF.GetY() * nPel + ((blockF.GetMV().y*time256)>>8)),
               pPlanesF[0]->GetPitch(),
			   pRef[0], nRefPitches[0],
			   pSrc[0], nSrcPitches[0],
			   pMaskFullYB, nPitchY,
			   pMaskFullYF, pMaskOccY,
			   nBlkSizeX, nBlkSizeY, time256, 0);
			// chroma u
            if (nSuperModeYUV & UPLANE)
				ResultBlock(pDst[1], nDstPitches[1],
				pPlanesB[1]->GetPointer((blockB.GetX() * nPel + ((blockB.GetMV().x*(256-time256))>>8))>>1, (blockB.GetY() * nPel + ((blockB.GetMV().y*(256-time256))>>8))/yRatioUV),
				pPlanesB[1]->GetPitch(),
				pPlanesF[1]->GetPointer((blockF.GetX() * nPel + ((blockF.GetMV().x*time256)>>8))>>1, (blockF.GetY() * nPel + ((blockF.GetMV().y*time256)>>8))/yRatioUV),
				pPlanesF[1]->GetPitch(),
				pRef[1], nRefPitches[1],
				pSrc[1], nSrcPitches[1],
				pMaskFullUVB, nPitchUV,
				pMaskFullUVF, pMaskOccUV,
				nBlkSizeX>>1, nBlkSizeY/yRatioUV, time256, 0);
			// chroma v
            if (nSuperModeYUV & VPLANE)
				ResultBlock(pDst[2], nDstPitches[2],
				pPlanesB[2]->GetPointer((blockB.GetX() * nPel + ((blockB.GetMV().x*(256-time256))>>8))>>1, (blockB.GetY() * nPel + ((blockB.GetMV().y*(256-time256))>>8))/yRatioUV),
				pPlanesB[2]->GetPitch(),
				pPlanesF[2]->GetPointer((blockF.GetX() * nPel + ((blockF.GetMV().x*time256)>>8))>>1, (blockF.GetY() * nPel + ((blockF.GetMV().y*time256)>>8))/yRatioUV),
				pPlanesF[2]->GetPitch(),
				pRef[2], nRefPitches[2],
				pSrc[2], nSrcPitches[2],
				pMaskFullUVB, nPitchUV,
				pMaskFullUVF, pMaskOccUV,
				nBlkSizeX>>1, nBlkSizeY/yRatioUV, time256, 0);


            // update pDsts
            pDst[0] += nBlkSizeX;
            pDst[1] += nBlkSizeX >> 1;
            pDst[2] += nBlkSizeX >> 1;
            pRef[0] += nBlkSizeX;
            pRef[1] += nBlkSizeX >> 1;
            pRef[2] += nBlkSizeX >> 1;
            pSrc[0] += nBlkSizeX;
            pSrc[1] += nBlkSizeX >> 1;
            pSrc[2] += nBlkSizeX >> 1;
			pMaskFullYB += nBlkSizeX;
			pMaskFullUVB += nBlkSizeX>>1;
			pMaskFullYF += nBlkSizeX;
			pMaskFullUVF += nBlkSizeX>>1;
			pMaskOccY += nBlkSizeX;
			pMaskOccUV += nBlkSizeX>>1;


            if ( !((i + 1) % nBlkX)  )
            {
            // blend rest right with time weight
                Blend(pDst[0], pSrc[0], pRef[0], nBlkSizeY, nWidth-nBlkSizeX*nBlkX, nDstPitches[0], nSrcPitches[0], nRefPitches[0], time256, true);
                if (nSuperModeYUV & UPLANE) Blend(pDst[1], pSrc[1], pRef[1], nBlkSizeY /yRatioUV, nWidthUV-(nBlkSizeX>>1)*nBlkX, nDstPitches[1], nSrcPitches[1], nRefPitches[1], time256, true);
                if (nSuperModeYUV & VPLANE) Blend(pDst[2], pSrc[2], pRef[2], nBlkSizeY /yRatioUV, nWidthUV-(nBlkSizeX>>1)*nBlkX, nDstPitches[2], nSrcPitches[2], nRefPitches[2], time256, true);

              pDst[0] += nBlkSizeY * nDstPitches[0] - nBlkSizeX*nBlkX;
               pDst[1] += ( nBlkSizeY /yRatioUV ) * nDstPitches[1] - (nBlkSizeX>>1)*nBlkX;
               pDst[2] += ( nBlkSizeY /yRatioUV ) * nDstPitches[2] - (nBlkSizeX>>1)*nBlkX;
               pRef[0] += nBlkSizeY * nRefPitches[0] - nBlkSizeX*nBlkX;
               pRef[1] += ( nBlkSizeY /yRatioUV ) * nRefPitches[1] - (nBlkSizeX>>1)*nBlkX;
               pRef[2] += ( nBlkSizeY /yRatioUV ) * nRefPitches[2] - (nBlkSizeX>>1)*nBlkX;
               pSrc[0] += nBlkSizeY * nSrcPitches[0] - nBlkSizeX*nBlkX;
               pSrc[1] += ( nBlkSizeY /yRatioUV ) * nSrcPitches[1] - (nBlkSizeX>>1)*nBlkX;
               pSrc[2] += ( nBlkSizeY /yRatioUV ) * nSrcPitches[2] - (nBlkSizeX>>1)*nBlkX;
               pMaskFullYB += nBlkSizeY * nPitchY - nBlkSizeX*nBlkX;
               pMaskFullUVB += (nBlkSizeY /yRatioUV) * nPitchUV - (nBlkSizeX>>1)*nBlkX;
               pMaskFullYF += nBlkSizeY * nPitchY - nBlkSizeX*nBlkX;
               pMaskFullUVF += (nBlkSizeY /yRatioUV) * nPitchUV - (nBlkSizeX>>1)*nBlkX;
               pMaskOccY += nBlkSizeY * nPitchY - nBlkSizeX*nBlkX;
               pMaskOccUV += (nBlkSizeY /yRatioUV) * nPitchUV - (nBlkSizeX>>1)*nBlkX;
            }
        }
       // blend rest bottom with time weight
        Blend(pDst[0], pSrc[0], pRef[0], nHeight-nBlkSizeY*nBlkY, nWidth, nDstPitches[0], nSrcPitches[0], nRefPitches[0], time256, true);
        
		if (nSuperModeYUV & UPLANE)
			Blend(pDst[1], pSrc[1], pRef[1], nHeightUV-(nBlkSizeY /yRatioUV)*nBlkY, nWidthUV, nDstPitches[1], nSrcPitches[1], nRefPitches[1], time256, true);
        
		if (nSuperModeYUV & VPLANE)
			Blend(pDst[2], pSrc[2], pRef[2], nHeightUV-(nBlkSizeY /yRatioUV)*nBlkY, nWidthUV, nDstPitches[2], nSrcPitches[2], nRefPitches[2], time256, true);
        
		PROFILE_STOP(MOTION_PROFILE_COMPENSATION);

		PROFILE_START(MOTION_PROFILE_YUY2CONVERT);

		if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2)
			YUY2FromPlanes(pDstYUY2, nDstPitchYUY2, nWidth, nHeight,
								  pDstSave[0], nDstPitches[0], pDstSave[1], pDstSave[2], nDstPitches[1], true);
		
		PROFILE_STOP(MOTION_PROFILE_YUY2CONVERT);

		return dst;
   }

   else // bad
   {
		PVideoFrame src = child->GetFrame(nleft,env); // it is easy to use child here - v2.0
		
	   //let's blend src with ref frames like ConvertFPS
        PVideoFrame ref = child->GetFrame(nright,env);
		PROFILE_START(MOTION_PROFILE_FLOWINTER);
        if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
		{
			pSrc[0] = src->GetReadPtr(); // we can blend YUY2
			nSrcPitches[0] = src->GetPitch();
			pRef[0] = ref->GetReadPtr();
			nRefPitches[0]  = ref->GetPitch();
			pDstYUY2 = dst->GetWritePtr();
			nDstPitchYUY2 = dst->GetPitch();
			Blend(pDstYUY2, pSrc[0], pRef[0], nHeight, nWidth*2, nDstPitchYUY2, nSrcPitches[0], nRefPitches[0], time256, true);
		}

		else
		{
			extractYUYFrame(dst,pDst,nDstPitches);
			extractYUYFrame(ref,pRef,nRefPitches);
			extractYUYFrame(src,pSrc,nSrcPitches);
		
			// blend with time weight
			Blend(pDst[0], pSrc[0], pRef[0], nHeight, nWidth, nDstPitches[0], nSrcPitches[0], nRefPitches[0], time256, true);
		
			if (nSuperModeYUV & UPLANE)
				Blend(pDst[1], pSrc[1], pRef[1], nHeightUV, nWidthUV, nDstPitches[1], nSrcPitches[1], nRefPitches[1], time256, true);
		
			if (nSuperModeYUV & VPLANE)
				Blend(pDst[2], pSrc[2], pRef[2], nHeightUV, nWidthUV, nDstPitches[2], nSrcPitches[2], nRefPitches[2], time256, true);
		}

		PROFILE_STOP(MOTION_PROFILE_FLOWINTER);

 		return dst;
   }
}