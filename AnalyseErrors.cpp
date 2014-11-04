#include "AnalyseErrors.h"


AnalyseErrors::AnalyseErrors(PClip source, bool isAnalysingBackward, int fpsDivisor,int fps,IScriptEnvironment* env) :
	GenericVideoFilter(source),
	fpsDivisor(2), //prototype
	fps(fps)
{
	//separate clip into number of clips specified by fpsDivisor and calculate vectors
	loweredFpsClips=new DivideFps* [fpsDivisor];
	loweredFpsClipsForwardVectors=new MVAnalyse* [fpsDivisor];
	loweredFpsClipsBackwardVectors=new MVAnalyse* [fpsDivisor];

	for(int i=0; i<fpsDivisor; i++){
		loweredFpsClips[i]=new DivideFps(source,fpsDivisor,i,env);
		
		//calculate errors using default MVAnalyse args
		loweredFpsClipsForwardVectors[i]=new MVAnalyse(loweredFpsClips[i],8,8,0,4,2,0,false,0,true,1,1200,1,true,50,50,0,0,0,"",0,0,0,10000,24,true,true,false,false,env);
		loweredFpsClipsBackwardVectors[i]=new MVAnalyse(loweredFpsClips[i],8,8,0,4,2,0,true,0,true,1,1200,1,true,50,50,0,0,0,"",0,0,0,10000,24,true,true,false,false,env);
	}

	//interpolate separated clips to get the base fps
	interpolatedClips=new MVBlockFps* [fpsDivisor];
	for(int i=0; i<fpsDivisor; i++)
		//interpolate using default MVBlockFps args
		interpolatedClips[i]=new MVBlockFps(PClip(loweredFpsClips[i]),
										 PClip(new MVSuper(loweredFpsClips[i],8,8,2,0,true,2,2,0,true,false,env)),
										 PClip(loweredFpsClipsBackwardVectors[i]),
										 PClip(loweredFpsClipsForwardVectors[i]),
										 fps,
										 1,0,0,true,MV_DEFAULT_SCD1,MV_DEFAULT_SCD2,true,false,env);

	errorDetectionClipVectors=new MVAnalyse(
		PClip(new ErrorDetectionClip(source,interpolatedClips[0],interpolatedClips[1],env)),
		8,8,0,4,2,0,
		isAnalysingBackward,
		0,true,1,1200,1,true,50,50,0,0,0,"",0,0,0,10000,24,true,true,false,false,env);
}


AnalyseErrors::~AnalyseErrors()
{
	delete[] loweredFpsClips;
	delete[] loweredFpsClipsForwardVectors;
	delete[] loweredFpsClipsBackwardVectors;
	delete[] interpolatedClips;
	delete errorDetectionClipVectors;
}


PVideoFrame __stdcall AnalyseErrors::GetFrame(int n, IScriptEnvironment* env){
	return errorDetectionClipVectors->GetFrame(2*n,env);
}
