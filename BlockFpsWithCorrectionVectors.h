#include "AnalyseErrors.h"

class BlockFpsWithCorrectionVectors : public GenericVideoFilter
{
private:
	int fpsMultiplier;
	PClip errorVectors;
	PClip forwardVectors;
	PClip backwardVectors;

public:
	BlockFpsWithCorrectionVectors(PClip source, PClip super, PClip backwardVectors, PClip forwardVectors, PClip errorVectors, int fpsMultiplier, IScriptEnvironment* env);
	~BlockFpsWithCorrectionVectors();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

