/* array.h: simple array template class */

#ifndef _RT_ARRAY_H_
#define _RT_ARRAY_H_

#ifdef OLD_OSTREAM
#include <ostream.h>
#else
#include <iostream>
#endif
#include <string.h>

namespace rt {

template<class T>
class array {
public:
  // number of array elements in use (the actual number of array cells may be higher)
  int size, allocsz;

  // array of elements (don't touch, unless you know what you're doing)
  T* s;

protected:
  int chunksz;
  bool dont_delete;

  // copies the used source string elements 
  void copy(const array& src)
  {
    s = new T[src.size];
    for (int i=0; i<src.size; i++)
      s[i] = src.s[i];
    allocsz = src.size;
    chunksz = src.chunksz;
    size = src.size;
    dont_delete = false;
  }

  // reallocates s so it can contain extra extra elements.
  void expand(int extra)  
  {
    T *t = new T[allocsz+=extra]; 
    for (int i=0; i<size; i++) 
      t[i] = s[i]; 
    if (!dont_delete) delete[] s; 
    s = t; 
    dont_delete = false;
  }

  // tries to resize the array in a clever way so there will
  // be place for at least 'nrelements' more.
  void smart_expand(int nrelems)
  {
    int needed = size+nrelems-allocsz;
    if (needed <= 0)
      return;  // no additional space needed
    if (chunksz != 0) // expand with sufficient number of chunks
      expand((needed/chunksz+1)*chunksz);
    else if (size>15) // try doubling the size
      expand(needed > size ? needed : size);
    else
      expand(needed); // make array exactly large enough
  }

  // initializes an array with 'len' elements pointed to by t.
public:		// public because Assoc<> wouldn't find it on Irix6.2
  void init (int len, T *t)
  {
    s = 0;
    chunksz = len;
    allocsz = len;
    size = 0;
    dont_delete = false;

    if (len > 0) {
      s = new T[len];
      chunksz = len;
      
      if (t) {	// copy the elements if there are given any
	for (int i=0; i<len; i++) 
	  s[i] = t[i];
	size = len;
      }
    }
  }

  array() { init(0, 0); }

  // Construct a new array with room for 'len' elements
  // initially. 
  // If the array needs to be expanded later, it will be expanded
  // with 'len' elements at once.
  // If 't' is not nil, it shall be an array containing
  // at least 'len' elements that will be copied to the new
  // array. (The array 't' can be deleted afterwards if so desired).
  array(int len, T *t =0) { init(len, t); }

  // copy constructor
  array(const array& src) 	{ copy(src); }

  // assignement: the source array elements are copied
  array& operator=(const array& src)
  {
    if (this != &src) {
      if (!dont_delete) delete [] s;
      copy(src);
    }
    return *this;
  }

  // destructor: also calls the destructor for the array elements.
  virtual ~array()
  {
    if (!dont_delete) delete [] s;
  }

  // encapsulates 'elems' into an array class object ('elems' is not copied
  // and shall not be destroyed during the life time of the array object)
  inline void set(int nrelems, T* elems)
  {
    if (!dont_delete) delete [] s;
    s = elems;
    chunksz = allocsz = size = nrelems;
    dont_delete = true;
  }

  // returns the i-th element, no range checking. Reference returned is only
  // valid until the next expansion.
  inline T& operator[](const int i) const { return s[i]; }
  inline T& elem(const int i) const { return s[i]; }

  // Appends an element to the end of the array, dynamically
  // expanding the array if needed.
  // Returns the index of the new element in the array.
  inline int append(const T& elem)
  {
    if (size >= allocsz)
      smart_expand(1);
    s[size++] = elem;
    return size-1;
  }

  // Appends a 'nrelems' new elements in one call.
  // Returns index of first added elements.
  inline int append(int nrelems, const T* elems)
  {
    if (size+nrelems > allocsz)
      smart_expand(nrelems);
    int firstindex = size;
    for (int i=0; i<nrelems; i++)
      s[size++] = elems[i];
    return firstindex;
  }

  // inserts element at the given location, moving the
  // element at the location and all subsequent ones one
  // place ahead. If i is larger than the size of the
  // array, the array is properly expanded. The value
  // of the non-filled in elements created this way
  // is undefined.
  inline void insert(const T& elem, const int i)
  {
    if (i>=size)
      size=i+1;
    else
      size++;
    if (size > allocsz)
      smart_expand(size-allocsz);

    for (int j=size-1; j>i; j--)
      s[j] = s[j-1];
    s[i] = elem;
  }

  // Appends elements from array 'elems' to this array.
  inline int concat(const array& elems)
  {
    return append(elems.size, elems.s);
  }

  // Reserves 'extra' array cells at the end of the array.
  // Array size is increased by 'extra'.
  inline void grow(int extra)  
  {
    if (size+extra > allocsz) expand(size+extra-allocsz); 
    size += extra; 
  }

  inline T* current(void)
  {
    return size>0 ? &s[size] : 0;
  }

  // clears the array: simply set its number of elements to zero.
  inline void clear(void)
  {
    size = 0;
  }

  inline std::ostream& print(std::ostream& out) const
  {
    out << '[';
    for (int i=0; i<size; i++)
      out << (i==0 ? " " : ", ") << s[i];
    return out << " ]";
  }

  friend std::ostream& operator<<(std::ostream& s, const array<T>& ar) { return ar.print(s); }
  friend std::ostream& operator<<(std::ostream& s, array<T> *ar) { return ar ? ar->print(s) : (s << "NULL"); }
};

} // namespace rt

#endif /*_RT_ARRAY_H_*/
