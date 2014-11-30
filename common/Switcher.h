#ifndef _Switcher_h_
#define _Switcher_h_

#include "types.h"
#include "libbyz.h"

typedef void (*switcher_func)(bool); // switcher function. Enable or disable specific protocol

enum replica_state {
    replica_state_NORMAL = 1,
    replica_state_PANICKING = 2,
    replica_state_MISWAITING= 3,
    replica_state_STOP = 4,
    replica_state_ABORTING = 5,
};


struct _Switcher_LE
{
    enum protocols_e protocol;
    switcher_func func;
    _Switcher_LE *next;
};

class Switcher
{
    public:
	Switcher() { list = NULL; }
	~Switcher();

	void register_switcher(enum protocols_e protocol, switcher_func);
	void deregister_switcher(enum protocols_e protocol);

	void switch_state(enum protocols_e protocol, bool state);

    private:
	_Switcher_LE *find(enum protocols_e protocol);
	_Switcher_LE *list;
};

inline _Switcher_LE* Switcher::find(enum protocols_e protocol)
{
    _Switcher_LE* ptr = list;
    while (ptr != NULL) {
	if (ptr->protocol == protocol)
	    return ptr;
	ptr = ptr->next;
    }
    return ptr;
};

inline void Switcher::register_switcher(enum protocols_e protocol, switcher_func func) {
    _Switcher_LE* ptr = find(protocol);
    if (ptr == NULL)
    	ptr = new _Switcher_LE();
    ptr->protocol = protocol;
    ptr->func = func;
    ptr->next = list;
    list = ptr;
};

inline void Switcher::deregister_switcher(enum protocols_e protocol) {
    _Switcher_LE* ptr = find(protocol);
    if (ptr == NULL)
	return;
    if (ptr == list) {
	list = list->next;
	ptr->next = NULL;
	delete ptr;
	return;
    }

    _Switcher_LE* ptr2 = list;
    while (ptr2->next && ptr2->next != ptr) ptr2 = ptr2->next;
    if (ptr2) {
	ptr2->next = ptr->next;
	ptr->next = NULL;
	delete ptr;
    }
};

inline void Switcher::switch_state(enum protocols_e protocol, bool state) {
    _Switcher_LE* ptr = find(protocol);
    if (ptr != NULL) {
	ptr->func(state);
    }
};

inline Switcher::~Switcher() {
    _Switcher_LE* ptr = list;
    while (ptr) {
	delete ptr;
	ptr = list->next;
	list = ptr;
    }
    list = NULL;
};

extern Switcher* great_switcher;

#endif //_Switcher_h_
