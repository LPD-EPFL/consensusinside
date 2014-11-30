#ifndef _ABORT_HISTORY_ELEMENT_H
#define _ABORT_HISTORY_ELEMENT_H

class AbortHistoryElement {
    public:
	int cid;
	Request_id rid;
	Digest d;
	bool operator==(AbortHistoryElement const other) const;
	AbortHistoryElement& operator=(const AbortHistoryElement& other);
};

typedef Array<AbortHistoryElement*> AbortHistory;

inline bool AbortHistoryElement::operator==(AbortHistoryElement const other) const {
    return
	(other.cid == cid)
	&& (other.rid == rid)
	&& (other.d == d);
};

inline AbortHistoryElement& AbortHistoryElement::operator=(const AbortHistoryElement& other) {
    if (this == &other)
    	return *this;
    cid = other.cid;
    rid = other.rid;
    d = other.d;

    return *this;
};

#endif
