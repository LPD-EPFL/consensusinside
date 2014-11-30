#include "zl_Smasher.h"

void zl_Smasher::process(int id)
{
    // phase 1, getting AH_1
    unsigned int j = 0;
    AbortHistory AH_1(size);
    ssize_t offset = sizeof(zl_Abort_rep);
    bool stop = false;
    while (!stop && j<size) {
	seeder.clear();
	for (int i=0; i<aborts.size(); i++) {
	    if (aborts[i] == NULL || aborts[i]->abort == NULL || aborts[i]->abort->contents() == NULL)
		continue;
	    if (aborts[i]->abort->hist_size() < j)
		continue;

	    AbortHistoryElement *ahe = new AbortHistoryElement();
	    char *contents = aborts[i]->abort->contents();

	    ssize_t cur_off = offset;
	    memcpy((char *)&ahe->cid, contents+cur_off, sizeof(int));
	    cur_off += sizeof(int);
	    memcpy((char *)&ahe->rid, contents+cur_off, sizeof(Request_id));
	    cur_off += sizeof(Request_id);
	    memcpy(ahe->d.digest(), contents+cur_off, sizeof(Digest));
	    cur_off += sizeof(Digest);

	    if (!seeder.store(ahe)) {
		delete ahe;
	    }
	};
	offset += sizeof(int) + sizeof(Request_id) + sizeof(Digest);
	bool found = false;
	for (int i=0; i < seeder.size(); i++) {
	    if (seeder.fetch_count(i) >= t+1) {
		AbortHistoryElement *ahc = new AbortHistoryElement();
		*ahc = *seeder.fetch(i);
		AH_1.append(ahc);
		found = true;
		break;
	    }
	}
	if (!found) {
	    stop = true;
	    break;
	}
	j++;
    }
    seeder.clear();

    // j contains the size, AH_1 is ready
    // create AH_2
    SCSet<AbortHistoryElement> seen(j);
    unsigned int pos = 0;
    AbortHistory *AH_2 =
    	new AbortHistory(j);
    while (pos < j) {
	if (!seen.has(AH_1[pos])) {
	    AH_2->append(AH_1[pos]);
	    seen.store(AH_1[pos]);
	    AH_1[pos] = NULL;
	} else {
	    delete AH_1[pos];
	    AH_1[pos] = NULL;
	}
	pos++;
    }
    for (int i=0; i<AH_1.size(); i++)
        if (AH_1[i] != NULL)
            delete AH_1[i];

    AH_1.clear();
    // now, calculate the missing requests
    // missing requests are the ones which are in AH_2, but we don't have them
    th_assert(aborts[id] != NULL && aborts[id]->abort != NULL, "no aborts from itself\n");

    missing = new AbortHistory(aborts[id]->abort->hist_size());
    missing->clear();
    char *contents = aborts[id]->abort->contents();
    offset = sizeof(zl_Abort_rep);
    for (pos=0; pos < aborts[id]->abort->hist_size(); pos++) {
	AbortHistoryElement *ahe = new AbortHistoryElement();

	memcpy((char *)&ahe->cid, contents+offset, sizeof(int));
	offset += sizeof(int);
	memcpy((char *)&ahe->rid, contents+offset, sizeof(Request_id));
	offset += sizeof(Request_id);
	memcpy(ahe->d.digest(), contents+offset, sizeof(Digest));
	offset += sizeof(Digest);

	// if not in the set, just discard it
	// if it is the set, store once again
	if (seen.has(ahe)) {
	    seen.store(ahe);
	}
	delete ahe;
    }

    // get all in the set, which have count 1
    for (int ndx = 0; ndx < seen.size(); ndx++) {
	if (seen.fetch_count(ndx) == 1) {
	    AbortHistoryElement *ahe;
	    ahe = seen.fetch(ndx);
	    missing->append(ahe);
	}
    }

    seen.non_destructive_clear();
    seeder.non_destructive_clear();

    // now, append? the last request
    ah = AH_2;
};


