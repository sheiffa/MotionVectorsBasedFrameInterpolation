#include "BlockFpsWithCorrectionVectors.h"


BlockFpsWithCorrectionVectors::BlockFpsWithCorrectionVectors(PClip source, PClip super, PClip backwardVectors, PClip forwardVectors, PClip errorVectors, int fpsMultiplier,IScriptEnvironment* env) :
	GenericVideoFilter(source),
	backwardVectors(backwardVectors),
	forwardVectors(forwardVectors),
	errorVectors(errorVectors),
	fpsMultiplier(fpsMultiplier)
{
}


BlockFpsWithCorrectionVectors::~BlockFpsWithCorrectionVectors()
{
}

PVideoFrame __stdcall BlockFpsWithCorrectionVectors::GetFrame(int n, IScriptEnvironment* env){
	return child->GetFrame(n,env); //stub
}