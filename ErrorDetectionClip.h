#include "MVInterface.h"
#include "MVBlockFps.h"

class ErrorDetectionClip : public GenericVideoFilter{
private:
	int fpsDivisor;
	PClip source;
	PClip* interpolatedClips;
public:
	ErrorDetectionClip(PClip source, int fpsDivisor, MVBlockFps** interpolatedClips, IScriptEnvironment* env);
	~ErrorDetectionClip();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
}; 