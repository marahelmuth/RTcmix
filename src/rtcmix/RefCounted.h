// RefCounted.h
//
// Base class for objects which are held by reference in multiple locations.
//

#ifndef _RT_REFCOUNTED_H_
#define _RT_REFCOUNTED_H_

class RefCounted {
public:
	int ref() { return ++_refcount; }
	int unref() { int r; if ((r=--_refcount) <= 0) { delete this; } return r; }
	static int unref(RefCounted *r);	// defined in rtstuff/RefCounted.C
protected:
	RefCounted() : _refcount(0) {}
	virtual ~RefCounted();				// defined in rtstuff/RefCounted.C
private:
	int _refcount;
};

#endif	//	 _RT_REFCOUNTED_H_