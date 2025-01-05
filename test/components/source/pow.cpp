#include "components/number.h"
#include "components/mult.h"
#include "components/pow.h"

namespace components {

	Number pow(const Number &base, const Number &exponent) {
		Number result(1);

		for (int i = 0; i < exponent.value(); ++i) {
			result = mult(result, base);
		}

		return result;
	}

}
