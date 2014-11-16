#include "AnalyseErrors.h"
#include "commonfunctions.h"
#include "MVInterface.h"
#include "CopyCode.h"
#include "SimpleResize.h"
#include "yuy2planes.h"

class BlockFpsWithCorrectionVectors : public GenericVideoFilter, MVFilter
{
private:
	PClip super;
	PClip errorVectors;
	MVClip forwardVectors;
	MVClip backwardVectors;
	int fpsMultiplier;

	//fps data
	unsigned int numerator;
	unsigned int denominator;
  	unsigned int numeratorOld;
	unsigned int denominatorOld;
	__int64 fa, fb;
	
	int thres;

	//super clip params
	int nSuperModeYUV;
	int nSuperHPad, nSuperVPad;
	MVGroupOfFrames *pRefBGOF;
    MVGroupOfFrames *pRefFGOF;

	//copy functions
	COPYFunction *BLITLUMA;
	COPYFunction *BLITCHROMA;

	BYTE *MaskFullYB; // shifted (projected) images planes
    BYTE *MaskFullUVB;
    BYTE *MaskFullYF;
    BYTE *MaskFullUVF;

    BYTE *MaskOccY; // full frame occlusion mask
    BYTE *MaskOccUV;

    BYTE *smallMaskF;// small forward occlusion mask
    BYTE *smallMaskB; // backward
    BYTE *smallMaskO; // both

	BYTE *OnesBlock; // block of ones

    int nWidthP, nHeightP, nPitchY, nPitchUV, nHeightPUV, nWidthPUV, nHeightUV, nWidthUV;
    int nBlkXP, nBlkYP;

	SimpleResize *upsizer;
    SimpleResize *upsizerUV;

	YUY2Planes * DstPlanes;

	//functionality from BlockFps constructor
	void setOutputFps(int fpsMultiplier);
	void getParamsFromSuperclip(IScriptEnvironment* env);
	void setCopyFunctions();

	void ResultBlock(BYTE *pDst, int dst_pitch, const BYTE * pMCB, int MCB_pitch, const BYTE * pMCF, int MCF_pitch,
		const BYTE * pRef, int ref_pitch, const BYTE * pSrc, int src_pitch, BYTE *maskB, int mask_pitch, BYTE *maskF,
		BYTE *pOcc, int nBlkSizeX, int nBlkSizeY, int time256, int mode);


public:
	BlockFpsWithCorrectionVectors(PClip source, PClip super, PClip backwardVectors, PClip forwardVectors, PClip errorVectors, int fpsMultiplier, IScriptEnvironment* env);
	~BlockFpsWithCorrectionVectors();

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

