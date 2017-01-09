/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 *  < subject >
 *  exception type
 *  1, special/panic/unknownreboot/wdtreboot
 *  2, anr/modem reset
 *
 */

#ifndef OBSERVABLE_H
#define OBSERVABLE_H
#include <vector>
#include "Observer.h"
class Observer;
using namespace std;

class Observable
{
public:
	Observable();
	virtual ~Observable();
protected:
	vector<Observer*> observer;
	volatile uint32_t changed;

public:
	void addObserver(Observer *o);
	void deleteObserver(Observer *o);
	void notifyObservers();
	void notifyObservers(void * arg);
	void setChanged();
	virtual void getSubjectInfo() = 0;
};

#endif
