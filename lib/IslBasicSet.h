#pragma once

#include <isl/set.h>

namespace molly {

class IslBasicSet {
private:
	isl_basic_set *set;

public:
	IslBasicSet() {
		// __isl_give isl_basic_set *isl_basic_set_empty( __isl_take isl_space *space);
	}

	~IslBasicSet() {
		isl_basic_set_free(set);
	}
};

}
