#ifndef _O_AcceptedProposals_h
#define _O_AcceptedProposals_h 1

#include "O_Request.h"
#include <map>
using namespace std;

class Proposal
{
	public:
		Proposal() {};
		Proposal(uint64_t _index, int _cid, Request_id _rid): index(_index), cid(_cid), rid(_rid) {};
		uint64_t index;
		int cid;
		Request_id rid;
};

class AcceptedProposals
{
   #define MAX_Proposal_SIZE 5
	public:
		AcceptedProposals(const map<uint64_t, O_Accept*> &acceptedSoFar, 
				const map<uint64_t, O_Learn*> &chosenSoFar)
		{
		   size = 0;
			for (map<uint64_t, O_Accept*>::const_iterator j = acceptedSoFar.begin(); j != acceptedSoFar.end(); j++) 
			{
			   map<uint64_t, O_Learn*>::const_iterator li = chosenSoFar.find(j->first);
				if (li != chosenSoFar.end()) 
					continue;
				fprintf(stderr, "index %lld accepted among (%d) but not chosen among %d\n", j->first, acceptedSoFar.size(), chosenSoFar.size());
				size++;
				th_assert(size < MAX_Proposal_SIZE, "too many accepted items");//TODO
				list[(int)size] = Proposal(j->second->index(), j->second->client_id(), j->second->request_id());
			}
			size++;//real size value
		}
		AcceptedProposals() {size = 0;}
		char size;
		Proposal list[MAX_Proposal_SIZE];
};

#endif 
