#include "components/number.h"
#include "components/add.h"

namespace components {

	Number add(const Number &lhs, const Number &rhs) {
		return Number(lhs.value() + rhs.value());
	}

}
