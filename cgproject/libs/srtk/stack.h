/* stack.h: simple stack implementation */

#ifndef _RT_STACK_H_
#define _RT_STACK_H_

#include "array.h"

namespace rt {

template<class T>
class stack: public array<T> {
public:
  inline stack<T>() { array<T>::init(0, 0); };
  inline stack<T>(int len) { array<T>::init(len, 0); };
  inline void push(const T elem) { array<T>::append(elem); }; // need a copy!
  inline T& pop(void) { return array<T>::operator[](--array<T>::size); };
  inline T& top(void) { return array<T>::operator[](array<T>::size-1); };
  inline void clear(void) { array<T>::size=0; }

  bool remove(const T& elem)
  {
    bool elem_removed = false;
    for (int i=0; i<array<T>::size; i++) {
      if (array<T>::s[i] == elem) {
	for (int j=i; j<array<T>::size-1; j++)
	  array<T>::s[j] = array<T>::s[j+1];
	array<T>::size--;
	elem_removed = true;
      }
    }
    return elem_removed;
  }
};

} // namespace rt

#endif /* _RT_STACK_H_ */
