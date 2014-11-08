#include "MVInterface.h"

class ErrorDetectionClip : public GenericVideoFilter{
private:
	PClip source;
	PClip interpolated1;
	PClip interpolated2;
public:
	ErrorDetectionClip(PClip source, PClip interpolated1, PClip interpolated2, IScriptEnvironment* env);
	~ErrorDetectionClip();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
}; 