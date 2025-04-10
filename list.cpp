#include "../include/list.hpp"

constexpr list<char>::list(const char* init_ls)
	: _data(nullptr), _cap(0)
{
	const isize _len = str_len(init_ls);
	if (_len > 0) {
		_data = (char*)malloc(sizeof(char)*_len + 1);
		memcpy(_data, init_ls, sizeof(char) * (_len + 1));
		_cap = _len;
	} else {
		_data = nullptr;
		_cap = 0;
	}
}

list<char> list<char>::copy() const {
	const isize _len = len();
	list<char> ret;
	ret.init();
	ret.prealloc(_len);
	memcpy(ret._data, _data, sizeof(char)*(_len+1));
	// Copying memory with length bytes.
	return ret;
}

list<char>& list<char>::extend(View<char> ref_ext)
{
	isize _len = len();
	_len+=ref_ext.len;
	if (_len > _cap) {
		if (_cap == 0 || _data == NULL) {
			const void* const old = (void*)_data; // Pointing to content, not length
			_data = (char*)malloc(sizeof(char)*(_len+1));
			if (old != NULL)
				memcpy((void*)_data, old, _len); // Copying old content (+Ld0), not length
			_cap = _len;
		} else {
			while (_len > _cap)
				_cap *= 2;
			// C++ really can be ugly sometimes ngl
			// btw i critize but my language has the same kind of shitty features
			// but mine is better coz it's my baby yk
			_data = (char*)(::realloc(
				_data,
				sizeof(char)*(_cap+1)
			));
		}
	}
	memcpy(_data + (_len - ref_ext.len), ref_ext.start, sizeof(char)*ref_ext.len);
	_data[_len] = 0;
	return *this;
}

list<char>& list<char>::rmv_range([[maybe_unused]] std::initializer_list<index_t> start_end) {
	todo();
	return *this;
}


