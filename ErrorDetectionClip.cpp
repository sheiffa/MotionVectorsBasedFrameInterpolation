#include "ErrorDetectionClip.h"

ErrorDetectionClip::ErrorDetectionClip(PClip source, int fpsDivisor, MVBlockFps** interpolatedClips, IScriptEnvironment* env) :
	GenericVideoFilter(source),
	fpsDivisor(fpsDivisor),
	source(source)
{
	this->interpolatedClips=new PClip[fpsDivisor];
	for(int i=0; i<fpsDivisor; i++)
		this->interpolatedClips[i]=interpolatedClips[i];
}

ErrorDetectionClip::~ErrorDetectionClip(){}

PVideoFrame __stdcall ErrorDetectionClip::GetFrame(int n, IScriptEnvironment* env){
	switch(n%2){
		case(0):
			return interpolatedClips[n%fpsDivisor]->GetFrame(n/2,env);
		case(1):
			return source->GetFrame(n/2,env);
	}
}