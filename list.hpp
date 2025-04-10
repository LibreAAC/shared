#pragma once
#include <cstdlib>
#ifndef H_LIST
#define H_LIST
#include <cassert>
#include <cstddef>
#include <cstring>

#include <experimental/type_traits>
#include <initializer_list>
#include <utility>
#include <raylib.h>
#include <cstdint>
#include "utils.hpp"

template <class T> struct View
{
  T* start;
  isize len;
  isize full;
  inline T& operator[](index_t index) const
  {
    if (index < 0)
    {
      index = len + index;
    }
    return start[index];
  }
  View shorten(isize diff) const
  {
    if (len < diff)
      return View<T>{.start=start, .len=0, .full=full};
    return View<T>{.start=start, .len=len-diff, .full=full};
  }
  constexpr inline bool is_full() const { return len == full; }
};

template <class T> class list
{
public:
  using Self = list<T>;
  template <class U>
  using _temp_has_destroyer = decltype(std::declval<U&>().destroy());
  static constexpr bool has_destroyer
      = std::experimental::is_detected_exact_v<void, _temp_has_destroyer, T>;
  T* _data;
  isize _cap;

  list();
  list(const T*);
  list(const list<T>& other) { memcpy(this, &other, sizeof(list<T>)); }
  list(const list<T>&& other) { memcpy(this, &other, sizeof(list<T>)); }
  inline Self& init();
  inline Self& push(T element);
  Self& prextend(View<T> ref_pre);
  Self& extend(View<T> ref_ext);
  inline Self& insert(T element, index_t index);
  // Return value is not destroyed !!!
  inline T pop();
  inline Self& prealloc(isize size);
  inline Self& set_len(isize new_len);
  inline Self& realloc();
  inline Self& force_realloc();
  inline void destroy();
  inline Self copy() const;
  inline Self& rmv(index_t index);
  inline View<T> to_view() const;
  Self& rmv_range(std::initializer_list<index_t> start_end);
  inline Self& clear();
  inline Self& assign(const View<T>& other);
  inline Self& assign(const list<T>& other);
  inline Self& operator=(const list<T>& other);
  inline Self& operator=(std::initializer_list<T> init_ls);
  T& operator[](const index_t index) const;
  inline View<T> operator[](std::initializer_list<index_t> start_end);

  // Modifies the state of the argument as disowned
  //  => other.cap := 0 (other.is_owned() := true)
  inline Self& own(list<T>& other);

  inline T* data() const;
  inline isize len() const;
  inline isize cap() const;

  // len == 0
  inline bool is_empty() const;
  // cap != 0
  inline bool is_owned() const;
   
  inline Self& operator += (const Self&& other)
  {
    extend(other);
  }
  inline Self& operator += (const T&& obj)
  {
    push(obj);
  }
  // inline Self copy() const { // fuck C++
  //   struct fake_list_char
  //   {
  //     char* _p;
  //     isize cap;
  //   };
  //   const isize _len = len();
  //   fake_list_char ret;
  //   ret._p = (char*)malloc(_len);
  //   memcpy(ret._p, _data, sizeof(char)*(_len+1));
  //   // Copying memory with length bytes.
  //   return *(list<char>*)&ret;
  // }
};

#define GEN template <class T>
#define Self list<T>
#define self (*this)
#define FETCH_LEN() *(((isize*)_data) - 1)

GEN list<T>::list()
{
  _data = NULL;
  _cap = 0;
}

GEN inline isize list<T>::len() const
{
  if (_data == nullptr)
    return 0;
  return FETCH_LEN();
}
GEN inline Self& list<T>::set_len(isize new_len) 
{
  if (new_len > 0)
  {
    assert(_data != nullptr);
    assert(_cap >= new_len);
    FETCH_LEN() = new_len;
  }
  return *this;
}
GEN inline Self& list<T>::clear()
{
  if (_data == nullptr || !is_owned())
  {
    init();
    return *this;
  }
  const isize size = FETCH_LEN();
  if (size == 0)
    return *this;
  if constexpr (has_destroyer)
  {
    for (isize i = 0; i < size; i++)
    {
      _data[i].destroy();
    }
  }
  FETCH_LEN() = 0;
  return self;
}
GEN inline isize list<T>::cap() const { return _cap; }
GEN inline T* list<T>::data() const { return _data; }
GEN inline bool list<T>::is_empty() const
{
  if (_data == nullptr)
  {
    return true;
  }
  return FETCH_LEN() == 0;
}
GEN inline bool list<T>::is_owned() const
{
  if (_data == nullptr)
    assert(_cap == 0);
  return _data != nullptr || _cap != 0;
}

GEN inline Self& list<T>::init()
{
  _data = nullptr;
  _cap = 0;
  return *this;
}

GEN inline Self& list<T>::prealloc(isize size)
{
  const isize _len = len();
  assertm(_cap == 0 && _len == 0, "List is already set: cannot prealloc.");
  _cap = size;
  _data = (T*)poff(malloc(sizeof(T) * _cap + sizeof(isize)), sizeof(isize));
  FETCH_LEN() = 0;
  return self;
}

// content of element is owned
GEN inline Self& list<T>::push(T element)
{
  isize _len = len();
  _len++;
  if (_len > _cap)
  {
    if (_cap == 0 || _data == NULL)
    {
      const void* const old = (void*)_data; // Pointing to content, not length
      _data = (T*)(((isize*)malloc(sizeof(T) * _len + sizeof(isize))) + 1);
      memcpy((void*)_data, old, _len - 1); // Copying old content, not length
      _cap = _len;
    }
    else
    {
      _cap *= 2;
      _data = (T*)poff(
          ::realloc(
              poff(_data, -sizeof(isize)),
              sizeof(T) * _cap + sizeof(isize)
          ),
          sizeof(isize)
      );
    }
  }
  _data[_len - 1] = element;
  FETCH_LEN() = _len;
  return self;
}

GEN Self& list<T>::extend(const View<T> ref_ext)
{
  isize init_len = len();
  isize _len = init_len;
  _len += ref_ext.len;
  if (_len > _cap)
  {
    if (_cap == 0 || _data == NULL)
    {
      const void* const old = (void*)_data; // Pointing to content, not length
      _data = (T*)(((isize*)malloc(sizeof(T) * _len + sizeof(isize))) + 1);
      memcpy(
          (void*)_data,
          old,
          _len - ref_ext.len
      ); // Copying old content, not length
      _cap = _len;
    }
    else
    {
      _cap = _len;
      _data = (T*)poff(
          ::realloc(
              poff(_data, -sizeof(isize)),
              sizeof(T) * _cap + sizeof(isize)
          ),
          sizeof(isize)
      );
    }
  }
  // 2. Push others
  for (isize i = init_len; i < _len; i++)
  {
    _data[i] = ref_ext[i - init_len];
  }
  FETCH_LEN() = _len;
  return self;
}

GEN Self& list<T>::prextend(View<T> ref_pre)
{
  isize init_len = len();
  isize _len = init_len;
  _len += ref_pre.len;
  if (_len == 0)
  {
    return *this;
  }
  if (init_len == 0)
  {
    return extend(ref_pre);
  }
  if (_len > _cap)
  {
    if (_cap == 0 || _data == NULL)
    {
      const void* const old = (void*)_data; // Pointing to content, not length
      _data = (T*)(((isize*)malloc(sizeof(T) * _len + sizeof(isize))) + 1);
      memcpy(
          (void*)_data,
          old,
          _len - ref_pre.len
      ); // Copying old content, not length
      _cap = _len;
    }
    else
    {
      _cap = _len;
      _data = (T*)poff(
          ::realloc(
              poff(_data, -sizeof(isize)),
              sizeof(T) * _cap + sizeof(isize)
          ),
          sizeof(isize)
      );
    }
  }
  // 1. Move everything
  for (int i = 0; i < init_len; i++)
  {
    _data[_len - i - 1] = _data[init_len - i - 1];
  }
  // 2. Push others
  for (isize i = 0; i < ref_pre.len; i++)
  {
    _data[i] = ref_pre.start[i];
  }
  FETCH_LEN() = _len;
  return self;
}

GEN inline Self& list<T>::insert(T element, index_t index)
{
  if (index < 0)
  {
    index = len() + index;
  }
  if (len() < 1)
  {
    self.push(element);
    return self;
  }
  self.push(self[-1]);
  for (index_t i = len() - 2; i > index; i--)
  {
    self._data[i] = self._data[i - 1];
  }
  self._data[index] = element;
  return self;
}

GEN inline Self& list<T>::rmv(index_t index)
{
  isize _len = len();
  if constexpr (has_destroyer)
  {
    _data[index].destroy();
  }
  _len--;
  for (isize i = index; i < _len; i++)
  {
    _data[i] = _data[i + 1];
  }
  FETCH_LEN() = _len;
  return self;
}

GEN Self& list<T>::rmv_range(std::initializer_list<index_t> start_end)
{
  isize _len = len();
  index_t start = *start_end.begin();
  index_t end = *(start_end.begin() + 1);
  assert(start < end);
  if constexpr (has_destroyer)
  {
    for (index_t i = start; i < end; i++)
    {
      _data[i].destroy();
    }
  }
  const isize dist = end - start;
  _len -= dist;
  for (isize i = start; i < _len; i++)
  {
    _data[i] = _data[i + dist];
  }
  FETCH_LEN() = _len;
  return self;
}


GEN inline View<T> list<T>::to_view() const
{
  return View<T>{.start = _data, .len = len(), .full = len()};
}

// Object is not destroyed !!!
GEN inline T list<T>::pop()
{
  const isize _len = len();
  assertm(_len > 0, "You tried to pop an empty list !");
  FETCH_LEN() = _len - 1;
  return _data[_len - 1];
}

GEN inline Self& list<T>::realloc()
{
  assertm(
      _cap == 0,
      "[MEMCHECK] You tried to realloc a list, but it was already allocated "
      "and you forgot to destroy it !"
  );
  return self.force_realloc();
}

GEN inline Self& list<T>::force_realloc()
{
  if (_data == nullptr)
    return self;
  isize _len = FETCH_LEN();
  if (_len == 0)
  {
    _data = nullptr;
    _cap = 0;
    return self;
  }
  T* old = _data;
  _data = calloc(_len, sizeof(T));
  _cap = _len;
  memcpy(_data, old, _len);
  // NOTE: this function doesn't free memory
  return self;
}

GEN inline Self& list<T>::assign(const View<T>& other)
{
  assert(other.is_full()
  ); // Or else accessing length will cause undefined behavior
  assertm(
      _cap == 0,
      "[MEMCHECK] You tried to assign to a list, but you forgot to destroy it "
      "! (1)"
  );
  _data = other.start;
  _cap = 0;
  return self;
}

GEN inline Self& list<T>::assign(const list<T>& other)
{
  assertm(
      _cap == 0,
      "[MEMCHECK] You tried to assign to a list, but you forgot to destroy it "
      "! (2)"
  );
  _data = other._data;
  _cap = 0;
  return self;
}

// Default C behavior, nothing weird
GEN inline Self& list<T>::operator=(const list<T>& other)
{
  _data = other._data;
  _cap = other._cap;
  return self;
}

GEN inline Self& list<T>::operator=(std::initializer_list<T> init_ls)
{
  assertm(
      _cap == 0,
      "[MEMCHECK] You tried to assign to a list, but you forgot to destroy it "
      "! (with an initializer list)"
  ) _data
      = (T*)poff(
          malloc(sizeof(T) * init_ls.size() + sizeof(isize)),
          sizeof(isize)
      );
  FETCH_LEN() = (isize)(init_ls.size());
  _cap = (isize)(init_ls.size());
  return self;
}

GEN T& list<T>::operator[](const index_t index) const
{
  const isize _len = len();
  if (index < 0)
  {
    return _data[_len + index];
  }
  if (0 > index || index >= _len)
  {
    abort();
  }
#ifdef DEBUG
  assertm(0 <= index && index < _len, "list out of bound access");
#endif
  return _data[index];
}

GEN inline View<T> list<T>::operator[](std::initializer_list<index_t> start_end)
{
  assert(
      start_end.size() == 2
  ); // because C++ is a piece of scheit (am very angy)
  index_t* p = (index_t*)start_end.begin();
  return View<T>{_data + p[0], p[1] - p[0], len()};
}

GEN inline Self& list<T>::own(list<T>& other)
{
  assertm(
      self._cap == 0,
      "You cannot overwrite ownership without destroying the first owner."
  );
  self._cap = other._cap;
  other._cap = 0;
  self._data = other._data;
}

GEN inline void list<T>::destroy()
{
  if (is_owned())
  {
    if constexpr (has_destroyer)
    {
      const isize _len = FETCH_LEN();
      for (isize i = 0; i < _len; i++)
      {
        _data[i].destroy();
      }
    }
    if (_data != nullptr)
    {
      free(((isize*)_data) - 1);
    }
    _data = nullptr;
    _cap = 0;
  }
}

GEN inline Self list<T>::copy() const
{
  const isize _len = len();
  list<T> ret;
  ret.init();
  ret.prealloc(_len);
  memcpy(
      ((isize*)ret._data) - 1,
      ((isize*)_data) - 1,
      sizeof(isize) + sizeof(T) * _len
  );
  // Copying memory with length bytes.
  return ret;
}

GEN inline Self operator + (const Self a, const Self b)
{
  return a.copy().extend(b.to_view());
}
GEN inline Self operator + (const Self a, const T&& obj)
{
  return a.copy().push(obj);
}

#undef GEN
#undef FETCH_LEN
#undef Self
#undef self

#include <cstdio>
// NOTE - Using abreviation: Ld0 <=> Leading zero
// NOTE - For strings,
//  the len is stored the C-string way, not in a preceding block (to the content of the array)
//  The cap doesn't count the Ld0
#define Self list<char>
#define self (*this)

template <>
class list<char>
{
public:
  using T = char;
  T* _data;
  isize _cap;

  constexpr list() : _data(nullptr), _cap(0) {}
  constexpr list(const char*);
  constexpr list(const list<T>& other)
    : _data(other._data), _cap(other._cap) {}
  constexpr list(const list<T>&& other)
    : _data(other._data), _cap(other._cap) {}
  inline Self& init() { _data = nullptr; _cap = 0; return *this; }
  inline Self& push(T element);
  Self& prextend(View<T> ref_pre);
  Self& extend(View<T> ref_ext);
  inline Self& hold(Self& other);
  inline Self& insert(T element, index_t index);
  // Return value is not destroyed !!!
  inline T pop();
  inline Self& prealloc(isize size);
  inline Self& set_len(isize new_len) = delete;
  inline Self& realloc()
  {
    assert(_cap == 0);
    const char* old = _data;
    const int _len = len();
    _cap = _len;
    _data = (char*)malloc(_cap+1);
    memcpy(_data, old, _cap+1);
    return *this;
  }
  inline Self& force_realloc();
  inline void destroy();
  Self copy() const;
  inline Self& rmv(index_t index);
  inline View<T> to_view() const
  {
    return View<T>{.start = _data, .len = len(), .full = len()};
  }
  Self& rmv_range(std::initializer_list<index_t> start_end);
  inline Self& clear();
  inline Self& assign(const View<T>& other);
  inline Self& assign(const list<T>& other)
  {
    _data = other._data;
    _cap = 0;
    return *this;
  }
  inline Self& operator=(const list<T>& other)
  {
    _data = other._data;
    _cap = other._cap;
    return *this;    
  }
  inline Self& operator=(std::initializer_list<T> init_ls);
  inline T& operator[](index_t index) const
  {
    if (index < 0) index = len() + index; // quite expensive but can be useful
#ifdef DEBUG
    assert(index >= 0);
    assert(index < len());
#endif
    return _data[index];
  }
  inline View<T> operator[](std::initializer_list<index_t> start_end);

  // Modifies the state of the argument as disowned
  //  => other.cap := 0 (other.is_owned() := true)
  inline Self& own(list<T>& other);

  inline constexpr T* data() const
  {
    return _data;
  }
  inline isize len() const;

  // len == 0
  inline bool is_empty() const
  {
    return len() == 0;
  }
  // cap != 0
  inline bool is_owned() const
  {
    return _cap != 0 && _data != nullptr;
  }
   
  inline Self& operator += (const Self&& other)
  {
    extend(other.to_view());
    return *this;
  }
  inline Self& operator += (const T&& obj)
  {
    push(obj);
    return *this;
  }
};


inline Self& list<char>::clear() {
	self._data[0] = 0;
	return self;
}

inline isize list<char>::len() const {
	return str_len(self._data);
}

inline Self& list<char>::hold(Self& other)
{
  self = other;
  other._cap = 0;
  return self;
}

inline Self& list<char>::push(char element) {
	isize _len = len();
	_len++;
	if (_len > _cap) {
		if (_cap == 0 || _data == NULL) {
			const void* const old = (void*)_data; // Pointing to content, not length
			_data = (char*)malloc(sizeof(char)*(_len+1));
			memcpy((void*)_data, old, _len); // Copying old content (+Ld0), not length
			_cap = _len;
		} else {
			_cap *= 2;
			// C++ really can be ugly sometimes ngl
			// btw i critize but my language has the same kind of shitty features
			_data = (char*)(::realloc(
				_data,
				sizeof(char)*(_cap+1)
			));
		}
	}
	_data[_len-1] = element;
	_data[_len] = 0; // could be useful, idk O_O
	return self;
}
// Return value is not destroyed !!!

inline char list<char>::pop() {
	const isize _len = len();
	assertm(_len > 0, "You tried to pop an empty list !");
	const char out = _data[_len-1];
	_data[_len-1] = 0;
	return out;
}

inline Self& list<char>::prealloc(isize size) {
  if (is_owned())
  {
  	const isize _len = len();
  	assertm(_cap == 0 && _len == 0, "List is already set: cannot prealloc.");
	}
	_cap = size;
	_data = (char*)malloc(sizeof(char)*(_cap+1));
	memset(_data, 0, sizeof(char)*(_cap+1));
	return self;
}

inline Self& list<char>::force_realloc() {
	if (_data == nullptr)
		return self;
	const isize _len = len();
	if (_len == 0) {
		_data = nullptr;
		_cap = 0;
		return self;
	}
	char* old = _data;
	_data = (char*)malloc((_len+1)*sizeof(char));
	memset(_data, 0, (_len+1)*sizeof(char));
	_cap = _len;
	memcpy(_data, old, _len);
	// NOTE: this function doesn't free memory
	return self;
}

inline void list<char>::destroy()
{
  if (is_owned())
  {
    if (_data != nullptr)
    {
      free(_data);
    }
    _data = nullptr;
    _cap = 0;
  }
}

inline Self& list<char>::rmv(index_t index) {
	const isize _len = len();
	memmove(_data + index, _data + index + 1, sizeof(char)*_len);
	return self;
}

using string = list<char>;
inline bool operator == (const string a, const string b)
{
  return str_eq(a.data(), b.data());
}

#undef Self
#undef self


#endif /* H_LIST */


