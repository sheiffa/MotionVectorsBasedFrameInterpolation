#include "DivideFps.h"


DivideFps::DivideFps(PClip source, int fps, int divisor, int offset, IScriptEnvironment* env) :
	GenericVideoFilter(source),
	divisor(divisor),
	offset(offset)
{
	vi.SetFPS(fps,divisor);
}


DivideFps::~DivideFps(void)
{
}

PVideoFrame __stdcall DivideFps::GetFrame(int n, IScriptEnvironment* env){
	return child->GetFrame(n/divisor+offset,env);
}