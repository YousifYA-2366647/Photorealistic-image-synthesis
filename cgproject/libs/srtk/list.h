/* list<T>.h */

#ifndef _RT_list_H_
#define _RT_list_H_

namespace rt {

template<class T>
class list {
public:
  T data;
  list<T> *next;	// pointer to next element

  inline operator T() const { return data; }
  inline operator T*() { return &data; }

  inline list<T>() { next = 0; }

  inline list<T>(const T& data)
  {
    list<T>::data = data;
    list<T>::next = 0;
  }

  // Insert new element after 'this', returns pointer to new list<T> element:
  // if inserting after the tail of the list<T>, the new tail of the list<T> if
  // returned.
  inline list<T>* append(list<T> *l)
  {
    l->next = next;
    next = l;
    return l;
  }

  // Insert new element after 'this', returns pointer to new list<T> element
  // (=new tial of the list<T> if inserting after the tail).
  inline list<T> *append(const T& data)
  {
    return append(new list<T>(data));
  }

  // Adds new elements in front of the list<T>. Returns the new head of the list<T>
  // (which is 'l').
  inline list<T> *prepend(list<T> *l)
  {
    l->next = this;
    return l;
  }

  // Adds 'data' to the head of the list<T>. Returns the new list<T> head.
  inline list<T>* prepend(const T& data)
  {
    list<T>* l = new list<T>(data);
    return prepend(l);
  }

  // Returns pointer to first list<T> element starting from 'this', with
  // data equal to 'data' (tested with '=='). If 'chasing' is not null,
  // a pointer to the list<T> element before the returned element is filled
  // in (chasing pointer). The chasing pointer is null if 'this' is the
  // matching element in the list<T>. The chasing pointer points to the last
  // element in the list<T> if no match is found.
  inline list<T>* find(const T& data, list<T>** chasing =0)
  {
    list<T> *l, *p=0;
    for (l=this; l; p=l, l=l->next)
      if (data == l->data)
	break;
    if (chasing) *chasing = p;
    return l;
  }

  // Removes 'this', 'prev' = preceeding element, returns pointer to the 
  // first element of the list<T> past the deleted element 'this'.
  // 'prev' is null is 'this' is the head of the list<T>. In that case, the
  // new head of the list<T> is returned.
  inline list<T> *remove(list<T> *prev)
  {
    if (prev) prev->next = next;
    return next;
  }

  // Removes the first occurence of 'data' in the list<T>, starting from 'this'.
  // returns the first element after the deleted element. If 'data' is not
  // found in the list<T>, nothing is removed and 'this' is returned.
  inline list<T>* remove(const T& data)
  {
    list<T> *p=0, *l = find(data, &p), *n = this;
    if (l) {
      n = remove(p);
      delete l;
    }
    return n;
  }

  // Counts the number of elements in the list<T> starting from 'this'.
  inline int count(void) const
  {
    int count=0;
    for (list<T>* l=this; l; l++)
      count++;
    return count;
  }

  // duplicates the list<T>, starting from 'this'
  inline list<T>* clone(void)
  {
    list<T> *newlist = 0, *tail = 0;
    for (list<T> *l=this; l; l=l->next) {
      list<T> *clone = new list<T>(l->data);
      tail = tail ? tail->append(clone) : (newlist = clone);
    }
    return newlist;
  }
};

} // namespace rt

#endif /* _RT_list_H_ */
