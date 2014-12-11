#include "AnalyseErrors.h"
#include "MVInterface.h"
#include "MVBlockFps.h"


class BlockFpsWithCorrectionVectors : public MVBlockFps
{
private:
	MVClip errorVectors;
	int fpsMultiplier;

public:
	BlockFpsWithCorrectionVectors(PClip source, PClip super, PClip backwardVectors, PClip forwardVectors, PClip errorVectors, int sourceFps, int fpsMultiplier, int mode, IScriptEnvironment* env);
	~BlockFpsWithCorrectionVectors();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

