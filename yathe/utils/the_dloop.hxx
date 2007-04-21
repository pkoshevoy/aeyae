// File         : the_dloop.hxx
// Author       : Paul A. Koshevoy
// Created      : Tue Jul  5 11:40:00 MDT 2005
// Copyright    : (C) 2005
// License      : GPL.
// Description  : cyclic double-linked list

#ifndef THE_DLOOP_HXX_
#define THE_DLOOP_HXX_

// local includes:
#include "utils/the_dlink.hxx"


//----------------------------------------------------------------
// the_dloop_t
// 
// A double-linked loop (cyclic list) of elements
template<class T>
class the_dloop_t
{
public:
  // default constructor:
  the_dloop_t():
    head(NULL),
    tail(NULL),
    size(0)
  {}
  
  // copy constructor:
  the_dloop_t(const the_dloop_t<T> & l):
    head(NULL),
    tail(NULL),
    size(0)
  { assign(l.size, l.head); }
  
  // destructor:
  virtual ~the_dloop_t()
  {
    for (unsigned int i = 0; i < size; i++)
    {
      the_dlink_t<T> * next = head->next;
      delete head;
      head = next;
    }
  }
  
  // assignment operator:
  inline the_dloop_t<T> & operator = (const the_dloop_t<T> & l)
  { assign(l.size, l.head); }
  
  // remove all elements from this list:
  virtual void clear()
  { while (head != NULL) remove_head(); }
  
  // check whether the list is empty:
  inline bool is_empty() const
  { return head == NULL; }
  
  // make a loop of a given size, copy the data from a given pointer:
  virtual void assign(const unsigned int new_size,
		      const the_dlink_t<T> * index)
  {
    the_dlink_t<T> * tmp_head = NULL;
    the_dlink_t<T> * tmp_tail = NULL;
    unsigned int tmp_size = 0;
    
    for (unsigned int i = 0;
	 i < new_size && index != NULL;
	 i++, index = index->next)
    {
      if (tmp_head == NULL)
      {
	tmp_head = new the_dlink_t<T>(index->elem, NULL, NULL);
	tmp_tail = tmp_head;
      }
      else
      {
	tmp_tail->next = new the_dlink_t<T>(index->elem, tmp_tail, NULL);
	tmp_tail = tmp_tail->next;
      }
      
      tmp_size++;
    }
    
    // replace the contents of the loop:
    clear();
    head = tmp_head;
    tail = tmp_tail;
    size = tmp_size;
    
    // close the loop:
    if (tail != NULL)
    {
      tail->next = head;
      head->prev = tail;
    }
  }
  
  // remove an element from the top of the stack and return it:
  virtual T remove_head()
  {
    assert(head != NULL);
    T elem(head->elem);
    
    the_dlink_t<T> * new_head = head->next;
    delete head;
    head = new_head;
    
    if (head == NULL)
    {
      tail = NULL;
    }
    else
    {
      // close the loop:
      head->prev = tail;
      tail->next = head;
    }
    
    size--;
    return elem;
  }
  
  // add a new element at the head of the list (WARNING: this function
  // does not check for duplicates):
  virtual bool prepend(const T & elem)
  {
    if (head == NULL)
    {
      head = new the_dlink_t<T>(elem, NULL, NULL);
      tail = head;
    }
    else
    {
      head->prev = new the_dlink_t<T>(elem, NULL, head);
      head = head->prev;
    }
    
    // close the loop:
    head->prev = tail;
    tail->next = head;
    size++;
    return true;
  }
  
  // add a new element at the tail of the list (WARNING: this function
  // does not check for duplicates):
  virtual bool append(const T & elem)
  {
    if (tail == NULL)
    {
      head = new the_dlink_t<T>(elem, NULL, NULL);
      tail = head;
    }
    else
    {
      tail->next = new the_dlink_t<T>(elem, tail, NULL);
      tail = tail->next;
    }
    
    // close the loop:
    head->prev = tail;
    tail->next = head;
    size++;
    return true;
  }
  
  // remove a first occurrence of given element from the loop:
  virtual bool remove(the_dlink_t<T> *& link)
  {
    if (link == NULL) return false;
    
    // open the loop:
    head->prev = NULL;
    tail->next = NULL;
    
    if (link == head)
    {
      // found a match at the head of the list:
      head = link->next;
    }
    else
    {
      // found a match in the body of the list:
      link->prev->next = link->next;
    }
    
    if (link == tail)
    {
      // found a match at the tail of the list:
      tail = link->prev;
    }
    else
    {
      // found a match in the body of the list:
      link->next->prev = link->prev;
    }
    
    size--;
    delete link;
    link = NULL;
    
    // close the loop:
    if (tail != NULL)
    {
      head->prev = tail;
      tail->next = head;
    }
    
    return true;
  }
  
  // remove a first occurrence of given element from the loop:
  virtual bool remove(const T & elem)
  {
    the_dlink_t<T> * index = head;
    for (unsigned int i = 0; i < size; i++, index = index->next)
    {
      if (a_is_equal_to_b(index->elem, elem))
      {
	remove(index);
	return true;
      }
    }
    
    return false;
  }
  
  // return the link that contains the first occurrence of a given element:
  the_dlink_t<T> * link(const T & elem) const
  {
    the_dlink_t<T> * index = head;
    for (unsigned int i = 0; i < size; i++, index = index->next)
    {
      if (a_is_equal_to_b(index->elem, elem)) return index;
    }
    
    return NULL;
  }
  
  // check whether a given element is in the list:
  inline bool has(const T & elem) const
  { return link(elem) != NULL; }
  
  // for debugging, dumps this loop:
  void dump(ostream & strm) const
  {
    strm << "the_dloop_t(" << (void *)this << ") { ";
    const the_dlink_t<T> * index = head;
    for (unsigned int i = 0; i < size; i++, index = index->next)
    {
      strm << index->elem << ' ';
    }
    strm << '}' << endl << endl;
  }
  
  virtual bool a_is_equal_to_b(const T & a, const T & b) const
  { return (a == b); }
  
  // data members:
  the_dlink_t<T> * head;
  the_dlink_t<T> * tail;
  
  unsigned int size;
};


//----------------------------------------------------------------
// operator <<
//
template <class T>
ostream &
operator << (ostream & s, const the_dloop_t<T> & l)
{
  l.dump(s);
  return s;
}


#endif // THE_DLOOP_HXX_
