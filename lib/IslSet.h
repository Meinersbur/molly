#ifndef MOLLY_ISLSET_H
#define MOLLY_ISLSET_H 1

#include <isl/set.h>

namespace molly {

class IslSet
{
private:
	struct isl_set *set;

public:
	IslSet(void);
	~IslSet(void);
};

}

#endif /* MOLLY_ISLSET_H */
