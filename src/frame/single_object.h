#ifndef CSINGLETONOBJECT_H
#define CSINGLETONOBJECT_H

#include <stdlib.h>

template<typename TYPE>
class SingletonObject
{
protected:
	SingletonObject<TYPE>(){};
	~SingletonObject<TYPE>(){};
private:
	static TYPE* instance_;
public:
	static TYPE* GetInstance();
};

template<typename TYPE>
TYPE* SingletonObject<TYPE>::instance_	= NULL;

template<typename TYPE>
TYPE* SingletonObject<TYPE>::GetInstance()
{
	if( instance_ == NULL )
	{
		instance_	= new TYPE();
	}
	return instance_;
}
#endif
