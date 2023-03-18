#pragma once
#include <stddef.h>

void panic(int num7seg, int numvoice);
void trace(int num7seg, int numvoice);

#define DEBUG // TODO

#ifndef assert
#ifdef DEBUG
#define assert(x) _assert(x)
#else
#define assert(x)
#endif
#endif

inline void _assert(bool condition) {
	if (!condition)
		panic(42, 0);
}

struct nullopt_t {};
inline nullopt_t nullopt;

/** Tries to mimic std::optional a tiny tiny bit. */
template <typename T> class optional {
  public:
	optional() { has_value_ = false; }

	optional(T const& value) {
		value_ = value;
		has_value_ = true;
	}

	optional(nullopt_t) { has_value_ = false; }

	optional<T>& operator=(optional<T> const& other) {
		this->has_value_ = other.has_value_;
		this->value_ = other.value_;
		return *this;
	}

	bool has_value() { return has_value_; }
	T& operator*() { return value_; }
	T const& operator*() const { return value_; }
	T* operator->() { return &value_; }
	T const* operator->() const { return &value_; }

  private:
	T value_;
	bool has_value_ = false;
};

template <typename T, size_t N> class StaticVector {
  public:
	StaticVector() { size_ = 0; }

	size_t size() const { return size_; }
	T* data() { return data_; }
	T const* data() const { return data_; }
	T* begin() { return data_; }
	T* end() { return data_ + size_; }
	T const* cbegin() const { return data_; }
	T const* cend() const { return data_ + size_; }
	T& operator[](size_t index) { return data_[index]; }
	T const& operator[](size_t index) const { return data_[index]; }
	T& back() { return data_[size_ - 1]; }
	T const& back() const { return data_[size_ - 1]; }
	bool empty() const { return size_ == 0; }

	bool push_back(T item) {
		if (size_ >= N) {
			return false;
		}
		data_[size_++] = item;
		return true;
	}

	T pop_back() {
		if (empty()) {
			panic(1, 1);
		}

		return data_[--size_];
	}

	void erase(size_t index) {
		if (index >= size_) {
			panic(1, 2);
		}

		size_--;

		for (size_t i = index; i < size_; i++) {
			data_[i] = data_[i + 1];
		}
	}

	void clear() { size_ = 0; }

  private:
	T data_[N];
	size_t size_ = 0;
};
