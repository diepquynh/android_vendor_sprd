/*
 * Author	:liunan
 * Date		:2014-10-20
 */

#ifndef OBSERVER_H
#define OBSERVER_H

#include "Observable.h"

class Observable;
class Observer
{
public:
	Observer(Observable *o);
	virtual ~Observer();
	virtual void handleEvent() = 0;
	virtual void handleEvent(void* arg) = 0;
protected:
	Observable *m_observable;
};

#endif
