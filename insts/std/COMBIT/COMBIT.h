class COMBIT : public Instrument {
	int insamps;
	float *combarr;
	float amp, *amptable, tabs[2], *in;
	int skip,inchan;
	float spread;

public:
	COMBIT();
	virtual ~COMBIT();
	int init(double*, int);
	int run();
};
