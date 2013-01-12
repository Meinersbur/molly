#pragma once

#include <isl/set.h>

class IslSet
{
private:
	struct isl_set *set;

public:
	IslSet(void);
	~IslSet(void);
};

