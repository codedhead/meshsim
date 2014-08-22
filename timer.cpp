#include "timer.h"

#include <stdio.h>
#include <windows.h>
#include <assert.h>
#include <vector>
using std::vector;

static LARGE_INTEGER freq;

static vector<LARGE_INTEGER> tic_stack;

void tic()
{
	static int _dummy_freq=QueryPerformanceFrequency(&freq);

	LARGE_INTEGER pc1={0};
	QueryPerformanceCounter(&pc1);
	tic_stack.push_back(pc1); // overhead??
}
double toc(const char* print_t)
{
	LARGE_INTEGER pc2={0};
	QueryPerformanceCounter(&pc2);

	if(tic_stack.empty()) return 0.;

	LARGE_INTEGER pc1=tic_stack.back();tic_stack.pop_back();

	double timecnt=((double)(pc2.QuadPart-pc1.QuadPart))/((double)freq.QuadPart);
	if(print_t) printf("%s %f ms\n",print_t,1000.*timecnt);
	return timecnt;
}
void toc(AccumTic* t)
{
	LARGE_INTEGER pc2={0};
	QueryPerformanceCounter(&pc2);

	if(!tic_stack.empty())
	{
		LARGE_INTEGER pc1=tic_stack.back();tic_stack.pop_back();
		t->add(pc2.QuadPart-pc1.QuadPart);
	}
}

ScopeTic::ScopeTic(const char* t)
:accum(0)
{
	static int _dummy_freq=QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&this->pc1);

	if(t)
	{
		printf("Starting [%s]\n",t);
		strcpy(text,t);
	}
	else text[0]=0;
}
ScopeTic::ScopeTic(AccumTic* ac)
:accum(ac)
{
	static int _dummy_freq=QueryPerformanceFrequency(&freq);

	QueryPerformanceCounter(&this->pc1);
	text[0]=0;
}
ScopeTic::~ScopeTic()
{
	QueryPerformanceCounter(&this->pc2);

	if(accum)
	{
		accum->add(this->pc2.QuadPart-this->pc1.QuadPart);
	}
	else // report it
	{
		double timecnt=((double)(this->pc2.QuadPart-this->pc1.QuadPart))/((double)freq.QuadPart);
		if(text[0])
			printf("[%s] %f ms\n",text,1000.*timecnt);
		else
			printf("Time elapsed %f ms\n",1000.*timecnt);
	}
}

AccumTic::AccumTic(const char* t):running(false),tm(0)
{
	if(t) strcpy(text,t);
	else text[0]=0;
}

void AccumTic::reset()
{
	tm=0;
}
void AccumTic::add(LONGLONG c)
{
	tm+=c;
}
void AccumTic::report()
{
	double timecnt=((double)tm)/((double)freq.QuadPart);
	if(text[0])
		printf("[%s] %f ms\n",text,1000.*timecnt);
	else
		printf("Time elapsed %f ms\n",1000.*timecnt);
}
