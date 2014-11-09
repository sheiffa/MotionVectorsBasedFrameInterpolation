#include "MVInterface.h"
#include "MVAnalyse.h"
#include "MVBlockFps.h"
#include "MVSuper.h"
#include "DivideFps.h"
#include "ErrorDetectionClip.h"

class AnalyseErrors : public GenericVideoFilter
{
private:
	int fpsDivisor;
    int fps;
	DivideFps** loweredFpsClips;
	MVSuper** loweredFpsSuperClips;
    MVAnalyse** loweredFpsClipsForwardVectors;
	MVAnalyse** loweredFpsClipsBackwardVectors;
    MVBlockFps** interpolatedClips;
	ErrorDetectionClip* errorDetectionClip;
	MVSuper* errorDetectionSuperClip;
    MVAnalyse* errorDetectionClipVectors;

public:
	AnalyseErrors(PClip source, int fpsDivisor,int fps,IScriptEnvironment* env);
	~AnalyseErrors();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

