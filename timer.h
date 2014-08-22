#ifndef _TIMER_H_
#define _TIMER_H_

#include <windows.h>

class AccumTic
{
public:
	AccumTic(const char* t=0);

	void reset();
	void add(LONGLONG c);
	void report();

	bool running;
	LONGLONG tm;
	char text[128];
};

class ScopeTic
{
public:
	ScopeTic(const char* t=0);
	ScopeTic(AccumTic*);
	~ScopeTic();

	LARGE_INTEGER pc1,pc2;
	char text[128];
	AccumTic* accum;
};

void tic();
double toc(const char* print_t); // in s
inline double toc(){return toc("Time elapsed");}
void toc(AccumTic*);


#endif