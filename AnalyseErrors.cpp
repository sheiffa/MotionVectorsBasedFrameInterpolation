#include "AnalyseErrors.h"


AnalyseErrors::AnalyseErrors(PClip source, int fpsDivisor,int fps,IScriptEnvironment* env) :
	GenericVideoFilter(source),
	fpsDivisor(2), //prototype
	fps(fps)
{
	//separate clip into number of clips specified by fpsDivisor and calculate vectors
	loweredFpsClips=new DivideFps* [fpsDivisor];
	loweredFpsSuperClips=new MVSuper* [fpsDivisor];
	loweredFpsClipsForwardVectors=new MVAnalyse* [fpsDivisor];
	loweredFpsClipsBackwardVectors=new MVAnalyse* [fpsDivisor];

	for(int i=0; i<fpsDivisor; i++){
		loweredFpsClips[i]=new DivideFps(source,fps,fpsDivisor,i,env);
		loweredFpsSuperClips[i]=new MVSuper(loweredFpsClips[i],8,8,2,0,true,2,2,0,true,false,env);
		
		//calculate errors using default MVAnalyse args
		loweredFpsClipsForwardVectors[i]=new MVAnalyse(loweredFpsSuperClips[i],8,8,0,4,2,0,false,0,true,1,1200,1,true,50,50,0,0,0,"",0,0,0,10000,24,true,true,false,false,env);
		loweredFpsClipsBackwardVectors[i]=new MVAnalyse(loweredFpsSuperClips[i],8,8,0,4,2,0,true,0,true,1,1200,1,true,50,50,0,0,0,"",0,0,0,10000,24,true,true,false,false,env);
	}
	
	//interpolate separated clips to get the base fps
	interpolatedClips=new MVBlockFps* [fpsDivisor];
	for(int i=0; i<fpsDivisor; i++)
		//interpolate using default MVBlockFps args
		interpolatedClips[i]=new MVBlockFps(loweredFpsClips[i],
										 loweredFpsSuperClips[i],
										 loweredFpsClipsBackwardVectors[i],
										 loweredFpsClipsForwardVectors[i],
										 fps,
										 1,0,0,true,MV_DEFAULT_SCD1,MV_DEFAULT_SCD2,true,false,env);
		
	errorDetectionClip=new ErrorDetectionClip(source,interpolatedClips[0],interpolatedClips[1],env);
	errorDetectionSuperClip=new MVSuper(errorDetectionClip,8,8,2,0,true,2,2,0,true,false,env);
	errorDetectionClipVectors=new MVAnalyse(
		errorDetectionSuperClip,
		8,8,0,4,2,0,
		false,	//analyse forward to compare interpolated frames with original
		0,true,1,1200,1,true,50,50,0,0,0,"",0,0,0,10000,24,true,true,false,false,env);
}


AnalyseErrors::~AnalyseErrors()
{
	delete[] loweredFpsClips;
	delete[] loweredFpsSuperClips;
	delete[] loweredFpsClipsForwardVectors;
	delete[] loweredFpsClipsBackwardVectors;
	delete[] interpolatedClips;
	delete errorDetectionClip;
	delete errorDetectionSuperClip;
	delete errorDetectionClipVectors;
}


PVideoFrame __stdcall AnalyseErrors::GetFrame(int n, IScriptEnvironment* env){
	return errorDetectionClipVectors->GetFrame(n,env);
}
