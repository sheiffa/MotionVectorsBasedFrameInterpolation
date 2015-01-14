#include "MVInterface.h"
#include "MVBlockFps.h"
#include "DivideFps.h"
#include "ErrorDetectionClip.h"
#include "MVAnalyse.h"
#include "MVSuper.h"

class ErrorAwareBlockFps : public MVBlockFps
{
private:
	int fpsMultiplier;
	int fpsDivisor;
	int sourceFps;
	ErrorDetectionClip* errorDetectionClip[2];
	MVSuper* errorDetectionSuperClip[2];
	MVAnalyse* errorDetectionClipVectors[2];
	MVClip* errorVectors[2];

public:
	ErrorAwareBlockFps(PClip source, PClip super, PClip backwardVectors, PClip forwardVectors, int fpsDivisor, int fpsMultiplier, int mode, IScriptEnvironment* env);
	~ErrorAwareBlockFps();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

