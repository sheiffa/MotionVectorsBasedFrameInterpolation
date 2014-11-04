#include "MVInterface.h"

class DivideFps : public GenericVideoFilter
{
private:
	MVClip source;
	int divisor;
	int offset;
public:
	DivideFps(PClip,int,int,IScriptEnvironment*);
	~DivideFps();
	PVideoFrame __stdcall GetFrame(int, IScriptEnvironment*);
};

