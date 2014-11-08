#include "MVInterface.h"

class DivideFps : public GenericVideoFilter
{
private:
	int divisor;
	int offset;
public:
	DivideFps(PClip source, int fps, int divisor, int offset, IScriptEnvironment* env);
	~DivideFps();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

