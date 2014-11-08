#include "ErrorDetectionClip.h"

ErrorDetectionClip::ErrorDetectionClip(PClip source, PClip interpolated1, PClip interpolated2, IScriptEnvironment* env) :
	GenericVideoFilter(source),
	source(source),
	interpolated1(interpolated1),
	interpolated2(interpolated2)
{
	
}

ErrorDetectionClip::~ErrorDetectionClip(){}

PVideoFrame __stdcall ErrorDetectionClip::GetFrame(int n, IScriptEnvironment* env){
	switch(n%4){
		case(0):
			return interpolated1->GetFrame(n/2,env);
		case(1):
			return source->GetFrame(n/2,env);
		case(2):	
			return interpolated2->GetFrame(n/2+1,env);
		case(3):
			return source->GetFrame(n/2+1,env);
	}	
}