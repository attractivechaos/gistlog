#pragma once

template<typename T, int bits=6>
class ShardMap {
	T h[1<<bits];
public:
	struct const_iterator { // this iterator class is implemented by Gemini CLI
		const ShardMap *ptr;
		int i;
		typename T::const_iterator it;
		const_iterator(const ShardMap *ptr, int i) : ptr(ptr), i(i) {
			if (i < (1<<bits)) {
				it = ptr->h[i].begin();
				while (i < (1<<bits) && it == ptr->h[i].end()) {
					++i;
					if (i < (1<<bits)) it = ptr->h[i].begin();
				}
				this->i = i;
			}
		}
		const typename T::value_type &operator*() const { return *it; }
		const typename T::value_type *operator->() const { return &*it; }
		bool operator!=(const const_iterator &rhs) const {
			return i != rhs.i || (i < (1<<bits) && it != rhs.it);
		}
		const_iterator &operator++() {
			++it;
			while (i < (1<<bits) && it == ptr->h[i].end()) {
				++i;
				if (i < (1<<bits)) it = ptr->h[i].begin();
			}
			return *this;
		}
	};
	ShardMap() {}
	inline typename T::mapped_type &operator[](const typename T::key_type &key) {
		typename T::hasher hasher;
		return h[hasher(key) & ((1<<bits)-1)][key];
	}
	void reserve(size_t sz) {
		for (int i = 0; i < 1<<bits; ++i)
			h[i].reserve(sz>>bits);
	}
	size_t size(void) const {
		size_t sz = 0;
		for (int i = 0; i < 1<<bits; ++i)
			sz += h[i].size();
		return sz;
	}
	const_iterator begin() const { return const_iterator(this, 0); }
	const_iterator end() const { return const_iterator(this, 1<<bits); }
};
