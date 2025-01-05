#pragma once

namespace components {

	class Number {
	public:
		explicit Number(int val) : m_val(val) {}
	
		int value() const { return m_val; }
	
		void setValue(int val) { m_val = val; }
	
	private:
		int m_val;
	};
	
	inline bool operator==(const Number &lhs, const Number &rhs) {
		return lhs.value() == rhs.value();
	}
	
	inline bool operator==(const Number &lhs, int rhs) {
		return lhs.value() == rhs;
	}

}
