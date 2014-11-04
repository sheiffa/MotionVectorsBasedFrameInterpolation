#include "MVInterface.h"

class DivideFps : public GenericVideoFilter
{
private:
	MVClip source;
	int divisor;
	int offset;
public:
	DivideFps(PClip source, int divisor, int offset, IScriptEnvironment* env);
	~DivideFps();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

