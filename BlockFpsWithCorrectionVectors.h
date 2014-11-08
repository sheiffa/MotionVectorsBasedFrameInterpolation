#include "AnalyseErrors.h"

class BlockFpsWithCorrectionVectors : public GenericVideoFilter
{
private:
	int fpsMultiplier;
	PClip errorsForward;
	PClip errorsBackward;

public:
	BlockFpsWithCorrectionVectors(PClip source, PClip super, PClip errorVectorsForward, PClip errorVectorsBackward, int fpsMultiplier, IScriptEnvironment* env);
	~BlockFpsWithCorrectionVectors();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

