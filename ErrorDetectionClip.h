#include "MVInterface.h"

class ErrorDetectionClip : public GenericVideoFilter, public MVFilter{
private:
	PClip source;
	PClip interpolated1;
	PClip interpolated2;
public:
	ErrorDetectionClip(PClip, PClip, PClip, IScriptEnvironment*);
	~ErrorDetectionClip();
	PVideoFrame __stdcall GetFrame(int, IScriptEnvironment*);
}; 