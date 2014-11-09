#include "AnalyseErrors.h"

class BlockFpsWithCorrectionVectors : public GenericVideoFilter
{
private:
	int fpsMultiplier;
	PClip errorVectors;

public:
	BlockFpsWithCorrectionVectors(PClip source, PClip super, PClip errorVectors, int fpsMultiplier, IScriptEnvironment* env);
	~BlockFpsWithCorrectionVectors();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

