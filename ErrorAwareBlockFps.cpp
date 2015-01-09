#include "ErrorAwareBlockFps.h"
#include "MaskFun.h"

ErrorAwareBlockFps::ErrorAwareBlockFps(PClip source, PClip super, PClip backwardVectors, PClip forwardVectors, int fpsDivisor, int fpsMultiplier,int mode,IScriptEnvironment* env) :
	MVBlockFps(source,super,backwardVectors,forwardVectors,(source->GetVideoInfo().fps_numerator)*fpsMultiplier,1,mode,0,true,MV_DEFAULT_SCD1,MV_DEFAULT_SCD2,true,false,env),
	fpsDivisor(2), //prototype
	fpsMultiplier(fpsMultiplier)
{
	sourceFps=source->GetVideoInfo().fps_numerator;
	
	//separate clip into number of clips specified by fpsDivisor and calculate vectors
	DivideFps** loweredFpsClips=new DivideFps* [fpsDivisor];
	MVSuper** loweredFpsSuperClips=new MVSuper* [fpsDivisor];
	MVAnalyse** loweredFpsClipsForwardVectors=new MVAnalyse* [fpsDivisor];
	MVAnalyse** loweredFpsClipsBackwardVectors=new MVAnalyse* [fpsDivisor];

	for(int i=0; i<fpsDivisor; i++){
		loweredFpsClips[i]=new DivideFps(source,sourceFps,fpsDivisor,i,env);
		loweredFpsSuperClips[i]=new MVSuper(loweredFpsClips[i],8,8,2,0,true,2,2,0,true,false,env);
		
		//calculate errors using default MVAnalyse args
		loweredFpsClipsForwardVectors[i]=new MVAnalyse(loweredFpsSuperClips[i],8,8,0,4,2,0,false,0,true,1,1200,1,true,50,50,0,0,0,"",0,0,0,10000,24,true,true,false,false,env);
		loweredFpsClipsBackwardVectors[i]=new MVAnalyse(loweredFpsSuperClips[i],8,8,0,4,2,0,true,0,true,1,1200,1,true,50,50,0,0,0,"",0,0,0,10000,24,true,true,false,false,env);
	}
	
	//interpolate separated clips to get the base fps
	MVBlockFps** interpolatedClips=new MVBlockFps* [fpsDivisor];
	for(int i=0; i<fpsDivisor; i++)
		//interpolate using default MVBlockFps args
		interpolatedClips[i]=new MVBlockFps(loweredFpsClips[i],
										 loweredFpsSuperClips[i],
										 loweredFpsClipsBackwardVectors[i],
										 loweredFpsClipsForwardVectors[i],
										 sourceFps,
										 1,0,0,true,MV_DEFAULT_SCD1,MV_DEFAULT_SCD2,true,false,env);
		
	errorDetectionClip=new ErrorDetectionClip(source,interpolatedClips[0],interpolatedClips[1],env);
	errorDetectionSuperClip=new MVSuper(errorDetectionClip,8,8,2,0,true,2,2,0,true,false,env);
	errorDetectionClipVectors=new MVAnalyse(
		errorDetectionSuperClip,
		8,8,0,4,2,0,
		false,	//analyse forward to compare interpolated frames with original
		0,true,1,1200,1,true,50,50,0,0,0,"",0,0,0,10000,24,true,true,false,false,env);
	errorVectors=new MVClip(errorDetectionClipVectors,MV_DEFAULT_SCD1,MV_DEFAULT_SCD2, env);
}

ErrorAwareBlockFps::~ErrorAwareBlockFps(){
}

inline BYTE MEDIAN(BYTE a, BYTE b, BYTE c)
{
	BYTE mn = min(a, b);
	BYTE mx = max(a, b);
	BYTE m = min(mx, c);
	m = max(mn, m);
	return m;
}


PVideoFrame __stdcall  ErrorAwareBlockFps::GetFrame(int n, IScriptEnvironment* env)
{
   	 int nHeightUV = nHeight/yRatioUV;
	 int nWidthUV = nWidth/2;

    _asm emms; // paranoya
	// intermediate product may be very large! Now I know how to multiply int64
 	int nleft = (int) ( __int64(n)* fa/fb );
	int time256 = int( (double(n)*double(fa)/double(fb) - nleft)*256 + 0.5);

   PVideoFrame dst;
   BYTE *pDst[3];
	const BYTE *pRef[3], *pSrc[3];
    int nDstPitches[3], nRefPitches[3], nSrcPitches[3];
	unsigned char *pDstYUY2;
	int nDstPitchYUY2;

   int off = mvClipB.GetDeltaFrame(); // integer offset of reference frame
	// usually off must be = 1
	if (off > 1)
		time256 = time256/off;

	int nright = nleft+off;


// v2.0
	if (time256 ==0){
        dst = child->GetFrame(nleft,env); // simply left
		return dst;
	}
	else if (time256==256){
		dst = child->GetFrame(nright,env); // simply right
		return dst;
	}

	PVideoFrame mvF = mvClipF.GetFrame(nright, env);
	mvClipF.Update(mvF, env);// forward from current to next
	mvF = 0;

	PVideoFrame mvB = mvClipB.GetFrame(nleft, env);
	mvClipB.Update(mvB, env);// backward from next to current
	mvB = 0;

	// updating Frame data for errorVectors MVClip
	PVideoFrame mvErrorVectors = errorVectors->GetFrame(nleft, env);
	errorVectors->Update(mvErrorVectors, env);// backward from next to current
	mvErrorVectors = 0;

	PVideoFrame	src	= super->GetFrame(nleft, env);
	PVideoFrame ref = super->GetFrame(nright, env);//  ref for backward compensation


	dst = env->NewVideoFrame(vi);

	// added usability condition
   if ( mvClipB.IsUsable() && mvClipF.IsUsable() && errorVectors->IsUsable())
   {
//		MVFrames *pFrames = mvCore->GetFrames(nIdx);
//         PMVGroupOfFrames pRefGOFF = pFrames->GetFrame(nleft); // forward ref
//         PMVGroupOfFrames pRefGOFB = pFrames->GetFrame(nright); // backward ref

      PROFILE_START(MOTION_PROFILE_YUY2CONVERT);

		if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
		{
            pSrc[0] = src->GetReadPtr();
            pSrc[1] = pSrc[0] + src->GetRowSize()/2;
            pSrc[2] = pSrc[1] + src->GetRowSize()/4;
            nSrcPitches[0] = src->GetPitch();
            nSrcPitches[1] = nSrcPitches[0];
            nSrcPitches[2] = nSrcPitches[0];

            pRef[0] = ref->GetReadPtr();
            pRef[1] = pRef[0] + ref->GetRowSize()/2;
            pRef[2] = pRef[1] + ref->GetRowSize()/4;
            nRefPitches[0] = ref->GetPitch();
            nRefPitches[1] = nRefPitches[0];
            nRefPitches[2] = nRefPitches[0];

            if (!planar)
            {
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
                pDst[0] = dst->GetWritePtr();
                pDst[1] = pDst[0] + nWidth;
                pDst[2] = pDst[1] + nWidth/2;
                nDstPitches[0] = dst->GetPitch();
                nDstPitches[1] = nDstPitches[0];
                nDstPitches[2] = nDstPitches[0];
			}
		}
		else
		{
         pDst[0] = YWPLAN(dst);
         pDst[1] = UWPLAN(dst);
         pDst[2] = VWPLAN(dst);
         nDstPitches[0] = YPITCH(dst);
         nDstPitches[1] = UPITCH(dst);
         nDstPitches[2] = VPITCH(dst);

         pRef[0] = YRPLAN(ref);
         pRef[1] = URPLAN(ref);
         pRef[2] = VRPLAN(ref);
         nRefPitches[0] = YPITCH(ref);
         nRefPitches[1] = UPITCH(ref);
         nRefPitches[2] = VPITCH(ref);

         pSrc[0] = YRPLAN(src);
         pSrc[1] = URPLAN(src);
         pSrc[2] = VRPLAN(src);
         nSrcPitches[0] = YPITCH(src);
         nSrcPitches[1] = UPITCH(src);
         nSrcPitches[2] = VPITCH(src);
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
		 int blocks = mvClipB.GetBlkCount();

		 int maxoffset = nPitchY*(nHeightP-nBlkSizeY)-nBlkSizeX;

		 if (mode == 3 || mode==4 || mode==5) {
		// make forward shifted images by projection to build occlusion mask
             for ( int i = 0; i < blocks; i++ )
             {
                const FakeBlockData &blockF = mvClipF.GetBlock(0, i);
                int offset = blockF.GetX() - ((blockF.GetMV().x*(time256))>>8)/nPel + (blockF.GetY() - ((blockF.GetMV().y*(time256))>>8)/nPel)*nPitchY;
                if (offset>= 0 && offset < maxoffset)
                    BLITLUMA(MaskFullYF + offset, nPitchY, OnesBlock, nBlkSizeX); // fill by ones
             }
            //  same mask for backward
            for ( int i = 0; i < blocks; i++ )
             {
                const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
                int offset = blockB.GetX() - ((blockB.GetMV().x*(256-time256))>>8)/nPel + (blockB.GetY() - ((blockB.GetMV().y*(256-time256))>>8)/nPel)*nPitchY;
                if (offset>= 0 && offset < maxoffset)
                    BLITLUMA(MaskFullYB + offset, nPitchY, OnesBlock, nBlkSizeX); // fill by ones
             }

			 // make small binary mask from  occlusion  regions
			 MakeSmallMask(MaskFullYF, nPitchY, smallMaskF, nBlkXP, nBlkYP, nBlkSizeX, nBlkSizeY, thres);
			 InflateMask(smallMaskF, nBlkXP, nBlkYP);
			 // upsize small mask to full frame size
			  upsizer->SimpleResizeDo(MaskFullYF, nWidthP, nHeightP, nPitchY, smallMaskF, nBlkXP, nBlkXP, dummyplane);
			  upsizerUV->SimpleResizeDo(MaskFullUVF, nWidthPUV, nHeightPUV, nPitchUV, smallMaskF, nBlkXP, nBlkXP, dummyplane);
			// now we have forward fullframe blured occlusion mask in maskF arrays

			 // make small binary mask from  occlusion  regions
			 MakeSmallMask(MaskFullYB, nPitchY, smallMaskB, nBlkXP, nBlkYP, nBlkSizeX, nBlkSizeY, thres);
			 InflateMask(smallMaskB, nBlkXP, nBlkYP);
			 // upsize small mask to full frame size
			  upsizer->SimpleResizeDo(MaskFullYB, nWidthP, nHeightP, nPitchY, smallMaskB, nBlkXP, nBlkXP, dummyplane);
			  upsizerUV->SimpleResizeDo(MaskFullUVB, nWidthPUV, nHeightPUV, nPitchUV, smallMaskB, nBlkXP, nBlkXP, dummyplane);
		 }
		 if (mode==4 || mode==5)
		 {
			// make final (both directions) occlusion mask
			MultMasks(smallMaskF, smallMaskB, smallMaskO,  nBlkXP, nBlkYP);
			 InflateMask(smallMaskO, nBlkXP, nBlkYP);
			 // upsize small mask to full frame size
			  upsizer->SimpleResizeDo(MaskOccY, nWidthP, nHeightP, nPitchY, smallMaskO, nBlkXP, nBlkXP, dummyplane);
			  upsizerUV->SimpleResizeDo(MaskOccUV, nWidthPUV, nHeightPUV, nPitchUV, smallMaskO, nBlkXP, nBlkXP, dummyplane);
		 }

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
		 {//char out[33]; itoa(blocks,out,10);
			 //if(i==30)env->ThrowError("in 30");
            const FakeBlockData &blockB = mvClipB.GetBlock(0, i);
            const FakeBlockData &blockF = mvClipF.GetBlock(0, i);
						
			// obtaining block data for forward analysis only
			const FakeBlockData &errorClipBlockLeft = errorVectors->GetBlock(0,i);

			// updating errorVectors with the next proper frame if exists
			mvErrorVectors = errorVectors->GetFrame(nleft+2, env);
			errorVectors->Update(mvErrorVectors, env);
			mvErrorVectors = 0;

			// hopefully this way we can check if we are on the last frame
			if (!errorVectors->IsUsable()) {
				// and then just obtain first frame twice, because we don't like proper handling of the border cases
				mvErrorVectors = errorVectors->GetFrame(nleft, env);
				errorVectors->Update(mvErrorVectors, env);
				mvErrorVectors = 0;
			}

			// obtaining block data for the next frame so the thing works just as in the algorithm
			// except for that we dont know if it works
			// most likely not
			const FakeBlockData &errorClipBlockRight = errorVectors->GetBlock(0,i);

			// updating errorVectors with the former frame again because paranoya
			mvErrorVectors = errorVectors->GetFrame(nleft, env);
			errorVectors->Update(mvErrorVectors, env);
			mvErrorVectors = 0;
			
			// luma
            ResultBlock(pDst[0], nDstPitches[0],
               //pPlanesB[0]->GetPointer(blockB.GetX() * nPel + ((blockB.GetMV().x*(256-time256))>>8), blockB.GetY() * nPel + ((blockB.GetMV().y*(256-time256))>>8)),
               pPlanesB[0]->GetPointer(blockB.GetX() * nPel + (((blockB.GetMV().x-( (errorClipBlockLeft.GetMV().x*time256)>>8 + (errorClipBlockRight.GetMV().x*(256-time256))>>8)/(fpsMultiplier*fpsMultiplier) )*(256-time256))>>8),
									   blockB.GetY() * nPel + (((blockB.GetMV().y-( (errorClipBlockLeft.GetMV().y*time256)>>8 + (errorClipBlockRight.GetMV().y*(256-time256))>>8)/(fpsMultiplier*fpsMultiplier) )*(256-time256))>>8)),
			   pPlanesB[0]->GetPitch(),
               //pPlanesF[0]->GetPointer(blockF.GetX() * nPel + ((blockF.GetMV().x*time256)>>8), blockF.GetY() * nPel + ((blockF.GetMV().y*time256)>>8)),
			   pPlanesF[0]->GetPointer(blockF.GetX() * nPel + (((blockF.GetMV().x-( (errorClipBlockLeft.GetMV().x*time256)>>8 + (errorClipBlockRight.GetMV().x*(256-time256))>>8)/(fpsMultiplier*fpsMultiplier) )*time256)>>8), 
									   blockF.GetY() * nPel + (((blockF.GetMV().y-( (errorClipBlockLeft.GetMV().y*time256)>>8 + (errorClipBlockRight.GetMV().y*(256-time256))>>8)/(fpsMultiplier*fpsMultiplier) )*time256)>>8)),
               pPlanesF[0]->GetPitch(),
			   pRef[0], nRefPitches[0],
			   pSrc[0], nSrcPitches[0],
			   pMaskFullYB, nPitchY,
			   pMaskFullYF, pMaskOccY,
			   nBlkSizeX, nBlkSizeY, time256, mode);
			
			// chroma u
            if (nSuperModeYUV & UPLANE) ResultBlock(pDst[1], nDstPitches[1],
               //pPlanesB[1]->GetPointer((blockB.GetX() * nPel + ((blockB.GetMV().x*(256-time256))>>8))>>1, (blockB.GetY() * nPel + ((blockB.GetMV().y*(256-time256))>>8))/yRatioUV),
               pPlanesB[1]->GetPointer((blockB.GetX() * nPel + (((blockB.GetMV().x-( (errorClipBlockLeft.GetMV().x*time256)>>8 + (errorClipBlockRight.GetMV().x*(256-time256))>>8)/(fpsMultiplier*fpsMultiplier) )*(256-time256))>>8))>>1,
									   (blockB.GetY() * nPel + (((blockB.GetMV().y-( (errorClipBlockLeft.GetMV().x*time256)>>8 + (errorClipBlockRight.GetMV().x*(256-time256))>>8)/(fpsMultiplier*fpsMultiplier) )*(256-time256))>>8))/yRatioUV),
			   pPlanesB[1]->GetPitch(),
               //pPlanesF[1]->GetPointer((blockF.GetX() * nPel + ((blockF.GetMV().x*time256)>>8))>>1, (blockF.GetY() * nPel + ((blockF.GetMV().y*time256)>>8))/yRatioUV),
               pPlanesF[1]->GetPointer((blockF.GetX() * nPel + (((blockF.GetMV().x-( (errorClipBlockLeft.GetMV().x*time256)>>8 + (errorClipBlockRight.GetMV().x*(256-time256))>>8)/(fpsMultiplier*fpsMultiplier) )*time256)>>8))>>1,
			                           (blockF.GetY() * nPel + (((blockF.GetMV().y-( (errorClipBlockLeft.GetMV().y*time256)>>8 + (errorClipBlockRight.GetMV().y*(256-time256))>>8)/(fpsMultiplier*fpsMultiplier) )*time256)>>8))/yRatioUV),
               pPlanesF[1]->GetPitch(),
			   pRef[1], nRefPitches[1],
			   pSrc[1], nSrcPitches[1],
			   pMaskFullUVB, nPitchUV,
			   pMaskFullUVF, pMaskOccUV,
			   nBlkSizeX>>1, nBlkSizeY/yRatioUV, time256, mode);
			// chroma v
            if (nSuperModeYUV & VPLANE) ResultBlock(pDst[2], nDstPitches[2],
               //pPlanesB[2]->GetPointer((blockB.GetX() * nPel + ((blockB.GetMV().x*(256-time256))>>8))>>1, (blockB.GetY() * nPel + ((blockB.GetMV().y*(256-time256))>>8))/yRatioUV),
               pPlanesB[2]->GetPointer((blockB.GetX() * nPel + (((blockB.GetMV().x-( (errorClipBlockLeft.GetMV().x*time256)>>8 + (errorClipBlockRight.GetMV().x*(256-time256))>>8)/(fpsMultiplier*fpsMultiplier) )*(256-time256))>>8))>>1,
									   (blockB.GetY() * nPel + (((blockB.GetMV().y-( (errorClipBlockLeft.GetMV().y*time256)>>8 + (errorClipBlockRight.GetMV().y*(256-time256))>>8)/(fpsMultiplier*fpsMultiplier) )*(256-time256))>>8))/yRatioUV),
			   pPlanesB[2]->GetPitch(),
               //pPlanesF[2]->GetPointer((blockF.GetX() * nPel + ((blockF.GetMV().x*time256)>>8))>>1, (blockF.GetY() * nPel + ((blockF.GetMV().y*time256)>>8))/yRatioUV),
               pPlanesF[2]->GetPointer((blockF.GetX() * nPel + (((blockF.GetMV().x-( (errorClipBlockLeft.GetMV().x*time256)>>8 + (errorClipBlockRight.GetMV().x*(256-time256))>>8)/(fpsMultiplier*fpsMultiplier) )*time256)>>8))>>1,
			                           (blockF.GetY() * nPel + (((blockF.GetMV().y-( (errorClipBlockLeft.GetMV().y*time256)>>8 + (errorClipBlockRight.GetMV().y*(256-time256))>>8)/(fpsMultiplier*fpsMultiplier) )*time256)>>8))/yRatioUV),
               pPlanesF[2]->GetPitch(),
			   pRef[2], nRefPitches[2],
			   pSrc[2], nSrcPitches[2],
			   pMaskFullUVB, nPitchUV,
			   pMaskFullUVF, pMaskOccUV,
			   nBlkSizeX>>1, nBlkSizeY/yRatioUV, time256, mode);

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
             //blend rest right with time weight
                Blend(pDst[0], pSrc[0], pRef[0], nBlkSizeY, nWidth-nBlkSizeX*nBlkX, nDstPitches[0], nSrcPitches[0], nRefPitches[0], time256, isse);
                if (nSuperModeYUV & UPLANE) Blend(pDst[1], pSrc[1], pRef[1], nBlkSizeY /yRatioUV, nWidthUV-(nBlkSizeX>>1)*nBlkX, nDstPitches[1], nSrcPitches[1], nRefPitches[1], time256, isse);
                if (nSuperModeYUV & VPLANE) Blend(pDst[2], pSrc[2], pRef[2], nBlkSizeY /yRatioUV, nWidthUV-(nBlkSizeX>>1)*nBlkX, nDstPitches[2], nSrcPitches[2], nRefPitches[2], time256, isse);

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

        //blend rest bottom with time weight
        Blend(pDst[0], pSrc[0], pRef[0], nHeight-nBlkSizeY*nBlkY, nWidth, nDstPitches[0], nSrcPitches[0], nRefPitches[0], time256, isse);
        if (nSuperModeYUV & UPLANE) Blend(pDst[1], pSrc[1], pRef[1], nHeightUV-(nBlkSizeY /yRatioUV)*nBlkY, nWidthUV, nDstPitches[1], nSrcPitches[1], nRefPitches[1], time256, isse);
        if (nSuperModeYUV & VPLANE) Blend(pDst[2], pSrc[2], pRef[2], nHeightUV-(nBlkSizeY /yRatioUV)*nBlkY, nWidthUV, nDstPitches[2], nSrcPitches[2], nRefPitches[2], time256, isse);
         PROFILE_STOP(MOTION_PROFILE_COMPENSATION);
		 
      PROFILE_START(MOTION_PROFILE_YUY2CONVERT);
		if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 && !planar)
		{
			YUY2FromPlanes(pDstYUY2, nDstPitchYUY2, nWidth, nHeight,
								  pDstSave[0], nDstPitches[0], pDstSave[1], pDstSave[2], nDstPitches[1], isse);
		}
      PROFILE_STOP(MOTION_PROFILE_YUY2CONVERT);

		return dst;
   }
   else // bad
   {
       PVideoFrame src = child->GetFrame(nleft,env); // it is easy to use child here - v2.0

	if (blend) //let's blend src with ref frames like ConvertFPS
	{
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
            Blend(pDstYUY2, pSrc[0], pRef[0], nHeight, nWidth*2, nDstPitchYUY2, nSrcPitches[0], nRefPitches[0], time256, isse);
		}
		else
		{
         pDst[0] = YWPLAN(dst);
         pDst[1] = UWPLAN(dst);
         pDst[2] = VWPLAN(dst);
         nDstPitches[0] = YPITCH(dst);
         nDstPitches[1] = UPITCH(dst);
         nDstPitches[2] = VPITCH(dst);

         pRef[0] = YRPLAN(ref);
         pRef[1] = URPLAN(ref);
         pRef[2] = VRPLAN(ref);
         nRefPitches[0] = YPITCH(ref);
         nRefPitches[1] = UPITCH(ref);
         nRefPitches[2] = VPITCH(ref);

         pSrc[0] = YRPLAN(src);
         pSrc[1] = URPLAN(src);
         pSrc[2] = VRPLAN(src);
         nSrcPitches[0] = YPITCH(src);
         nSrcPitches[1] = UPITCH(src);
         nSrcPitches[2] = VPITCH(src);
       // blend with time weight
        Blend(pDst[0], pSrc[0], pRef[0], nHeight, nWidth, nDstPitches[0], nSrcPitches[0], nRefPitches[0], time256, isse);
        if (nSuperModeYUV & UPLANE) Blend(pDst[1], pSrc[1], pRef[1], nHeightUV, nWidthUV, nDstPitches[1], nSrcPitches[1], nRefPitches[1], time256, isse);
        if (nSuperModeYUV & VPLANE) Blend(pDst[2], pSrc[2], pRef[2], nHeightUV, nWidthUV, nDstPitches[2], nSrcPitches[2], nRefPitches[2], time256, isse);
		}
     PROFILE_STOP(MOTION_PROFILE_FLOWINTER);

 	   return dst;
	}
	else
	{
		return src; // like ChangeFPS
	}
   }

}
