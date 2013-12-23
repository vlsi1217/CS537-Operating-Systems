#include "types.h"
#include "stat.h"
#include "pstat.h"
#include "user.h"

#define N  64

int
getprocessinfo(void)
{
	struct pstat p;
	int i;
	for(i = 0; i < N; i++)
	{
		p.inuse[i] = 0;
		p.pid[i] = -1;   // the PID of each process
    		p.hticks[i] = 0; // the number of ticks each process has accumulated at HIGH priority
    		p.lticks[i] = 0; // the number of ticks each process has accumulated at LOW priority
    		p.runTimes[i] = 0;
    		p.queue[i] = -1;
	}
	int va = getpinfo(&p);
	printf(2, "Here is the list of process stat: ");
	for(i = 0; i < N; i++)
	{
		printf(2, "Process: pid %d, inuse %d, hticks %d, lticks %d\n", p.inuse[i], p.pid[i], p.hticks[i], p.lticks[i]);
	}
	return va;
}

int
main(void)
{
  getprocessinfo();
  exit();
}
