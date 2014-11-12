#include "BlockFpsWithCorrectionVectors.h"


BlockFpsWithCorrectionVectors::BlockFpsWithCorrectionVectors(PClip source, PClip super, PClip backwardVectors, PClip forwardVectors, PClip errorVectors, int fpsMultiplier,IScriptEnvironment* env) :
	GenericVideoFilter(source),
	MVFilter(backwardVectors, "BlockFpsWithCorrectionVectors", env),
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

	getParamsFromSuperclip(super,env);

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

void BlockFpsWithCorrectionVectors::getParamsFromSuperclip(PClip super, IScriptEnvironment* env){
// get parameters of prepared super clip - v2.0
	SuperParams64Bits params;
    memcpy(&params, &super->GetVideoInfo().num_audio_samples, 8);
    int nHeightS = params.nHeight;
    nSuperHPad = params.nHPad;
    nSuperVPad = params.nVPad;
    int nSuperPel = params.nPel;
    nSuperModeYUV = params.nModeYUV;
    int nSuperLevels = params.nLevels;

    pRefBGOF = new MVGroupOfFrames(nSuperLevels, nWidth, nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, false, yRatioUV);
    pRefFGOF = new MVGroupOfFrames(nSuperLevels, nWidth, nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, false, yRatioUV);
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


PVideoFrame __stdcall BlockFpsWithCorrectionVectors::GetFrame(int n, IScriptEnvironment* env){
	return child->GetFrame(n,env); //stub
}