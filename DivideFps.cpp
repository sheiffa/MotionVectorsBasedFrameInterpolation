#include "DivideFps.h"


DivideFps::DivideFps(PClip source, int divisor, int offset, IScriptEnvironment* env) :
	GenericVideoFilter(source),
	source(source,0,0,env),
	divisor(divisor),
	offset(offset)
{
}


DivideFps::~DivideFps(void)
{
}

PVideoFrame __stdcall DivideFps::GetFrame(int n, IScriptEnvironment* env){
	return source.GetFrame(n/divisor+offset,env);
}