#include "AnalyseErrors.h"
#include "MVInterface.h"
#include "MVBlockFps.h"


class BlockFpsWithCorrectionVectors : public MVBlockFps
{
private:
	PClip errorVectors;
	int fpsMultiplier;

	void ResultBlock(BYTE *pDst, int dst_pitch, const BYTE * pMCB, int MCB_pitch, const BYTE * pMCF, int MCF_pitch,
		const BYTE * pRef, int ref_pitch, const BYTE * pSrc, int src_pitch, BYTE *maskB, int mask_pitch, BYTE *maskF,
		BYTE *pOcc, int nBlkSizeX, int nBlkSizeY, int time256, int mode);


public:
	BlockFpsWithCorrectionVectors(PClip source, PClip super, PClip backwardVectors, PClip forwardVectors, PClip errorVectors, int sourceFps, int fpsMultiplier, int mode, IScriptEnvironment* env);
	~BlockFpsWithCorrectionVectors();
};

