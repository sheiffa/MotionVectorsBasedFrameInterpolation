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
    MVAnalyse** loweredFpsClipsForwardVectors;
	MVAnalyse** loweredFpsClipsBackwardVectors;
    MVBlockFps** interpolatedClips;
    MVAnalyse* errorDetectionClipVectors;

public:
	AnalyseErrors(PClip source, bool isAnalysingBackward, int fpsDivisor,int fps,IScriptEnvironment* env);
	~AnalyseErrors();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

