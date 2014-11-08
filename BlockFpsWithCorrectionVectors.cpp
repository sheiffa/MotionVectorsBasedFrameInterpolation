#include "BlockFpsWithCorrectionVectors.h"


BlockFpsWithCorrectionVectors::BlockFpsWithCorrectionVectors(PClip source, PClip super, PClip errorVectorsBackward, PClip errorVectorsForward, int fpsMultiplier,IScriptEnvironment* env) :
	GenericVideoFilter(source),
	errorsBackward(errorsBackward),
	errorsForward(errorsForward),
	fpsMultiplier(fpsMultiplier)
{
}


BlockFpsWithCorrectionVectors::~BlockFpsWithCorrectionVectors()
{
}

PVideoFrame __stdcall BlockFpsWithCorrectionVectors::GetFrame(int n, IScriptEnvironment* env){
	return child->GetFrame(n,env);
}