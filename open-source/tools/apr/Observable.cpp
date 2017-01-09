

#include <vector>
#include "Observer.h"
#include "Observable.h"

Observable::Observable()
{
	// initialize
	changed = 0;
}

Observable::~Observable()
{
}

void Observable::addObserver(Observer *o)
{
	observer.push_back(o);
	return;
}


void Observable::deleteObserver(Observer *o)
{
	for (vector<Observer*>::iterator iter=observer.begin(); iter!=observer.end(); iter++)
	{
		if (o == *iter)
		{
			observer.erase(iter);
			return;
		}
	}
}

void Observable::notifyObservers()
{
	if (changed--)
	{
		for (vector<Observer*>::iterator iter=observer.begin(); iter!=observer.end(); iter++)
		{
			(*iter)->handleEvent();
		}
	}
}

void Observable::notifyObservers(void *arg)
{
	if (changed--)
	{
		for (vector<Observer*>::iterator iter=observer.begin(); iter!=observer.end(); iter++)
		{
			(*iter)->handleEvent(arg);
		}
	}
}

void Observable::setChanged()
{
	changed++;
}


