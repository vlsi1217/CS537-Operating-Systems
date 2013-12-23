/*
	int i = 0;
    	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    	{
    		if(p->state != RUNNABLE)
    		    	continue;
		
		for(i = 0; i < NPROC; i++)
		{
			if(high.valid[i] && high.ps[i]->state == ZOMBIE)
			{
				high.size--;
				high.valid[i] = 0;
			}
			if(low.valid[i] && low.ps[i]->state == ZOMBIE)
			{
				low.size--;
				low.valid[i] = 0;
			}
		}
    	  	int stop = 0;
      		for(i = 0; i < NPROC && !stop; i++)
      		{
			if(high.valid[i] && p == high.ps[i] && p->queue == 1)
			{
				stop = 1;
        		}
			else if(high.valid[i] && p == high.ps[i] && p->queue == 0)
			{
				int k, s = 0;
				for(k = 0; k < NPROC && !s; k++)
				{
					if(!low.valid[k])
					{
						low.ps[k] = p;
						low.valid[k] = 1;
						low.size++;
						s = 1;
					}
				}
				high.valid[i] = 0;
			}
	        }
		if(stop)
      		{
			continue;
      		}
		p->queue = 1;
      		int k, s = 0;
		for(k = 0; k < NPROC && !s; k++)
		{
			if(!high.valid[k])
			{
				high.ps[k] = p;
				high.valid[k] = 1;
				high.size++;
				s = 1;
			}
		}
    	}
    	cprintf("high priority queue: front: %d end: %d size: %d\n", high.front, high.end, high.size);
    	for(i = 0; i < NPROC; i++)
    	{
		cprintf("process on high priority queue: pid %d name: %s\n", high.ps[i]->pid, high.ps[i]->name);
    	}
	struct proc* winP = NULL;
	if(high.size >= 1)
	{
		//use the randomized lottery
		//first sum over all tickets
		int totalTickets = 0;
		int i;		
		for(i = 0; i < NPROC; i++)
		{
			if(high.valid[i])
			{
				totalTickets += high.ps[i]->numTickets;
			}
		}
		unsigned int rand = temper(3);
		unsigned  int winner = rand  % totalTickets, counter = 0;
		for(i = 0; i < NPROC; i++)
		{
			if(high.valid[i])
			{
				counter += high.ps[i]->numTickets;
				if(counter > winner)
				{
					winP = high.ps[i];
					break;
				}
			}
		}
		
	}
	else if(low.size >= 1)
	{
		int totalTickets = 0;
		int i;		
		for(i = 0; i < NPROC; i++)
		{
			if(low.valid[i])
			{
				totalTickets += low.ps[i]->numTickets;
			}
		}
		unsigned int rand = temper(3);
		unsigned int winner = rand % totalTickets, counter = 0;
		for(i = 0; i < NPROC; i++)
		{
			if(low.valid[i])
			{
				counter += low.ps[i]->numTickets;
				if(counter > winner)
				{
					winP = low.ps[i];
					break;
				}
			}
		}
	}
	if(winP != NULL)
	{
		proc = winP;
		switchuvm(winP);
		winP->state = RUNNING;
		swtch(&cpu->scheduler, proc->context);
		switchkvm();
		proc = 0;
	}
*/
