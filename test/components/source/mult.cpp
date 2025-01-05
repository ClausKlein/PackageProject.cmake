#include "components/number.h"
#include "components/mult.h"

namespace components {

	Number mult(const Number &lhs, const Number &rhs) {
		return Number(lhs.value() * rhs.value());
	}

}
