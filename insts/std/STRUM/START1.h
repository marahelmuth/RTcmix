#include "strums.h"
#include "delayq.h"

class START1 : public Instrument {
	float spread, amp, aamp;
	strumq *strumq1;
	delayq *dq;
	float dgain, fbgain;
	float cleanlevel, distlevel;
	float d;
	float *amptable, amptabs[2];
	int deleteflag, branch, skip;

public:
	START1();
	virtual ~START1();
	int init(float*, int);
	int run();
	};
