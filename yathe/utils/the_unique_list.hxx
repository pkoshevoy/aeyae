// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_unique_list.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2006/04/15 21:39
// Copyright    : (C) 2006
// License      : MIT
// Description  : A linked list that tries to keep duplicates out -- expensive.

#ifndef UNIQUE_LIST_HXX_
#define UNIQUE_LIST_HXX_

// system includes:
#include <list>
#include <algorithm>


namespace the
{

//----------------------------------------------------------------
// unique_list
// 
template <typename data_t, typename alloc_t = std::allocator<data_t> >
class unique_list : public std::list<data_t, alloc_t>
{
  typedef std::list<data_t, alloc_t>	base_t;
  
public:
  typedef data_t		value_type;
  typedef value_type *		pointer;
  typedef const value_type *	const_pointer;
  
  typedef value_type &		reference;
  typedef const value_type &	const_reference;
  typedef size_t		size_type;
  typedef ptrdiff_t		difference_type;
  
  typedef typename base_t::allocator_type	allocator_type;
  typedef typename base_t::iterator		iterator;
  typedef typename base_t::const_iterator	const_iterator;
  typedef std::reverse_iterator<iterator>	reverse_iterator;
  typedef std::reverse_iterator<const_iterator>	const_reverse_iterator;
  
public:
  explicit unique_list(const allocator_type & a = allocator_type()):
    base_t(a)
  {}
  
  // copy constructor:
  unique_list(const unique_list & x):
    base_t(x.get_allocator())
  {
    this->insert(begin(), x.begin(), x.end());
  }
  
  // construct a list from a range of elements:
  template <typename input_iterator_t>
  unique_list(input_iterator_t first,
	      input_iterator_t last,
	      const allocator_type & a = allocator_type()):
    base_t(a)
  {
    unique_list::insert(begin(), first, last);
  }
  
  // destructor:
  virtual ~unique_list()
  { base_t::clear(); }
  
  // assignment operator:
  inline unique_list & operator = (const unique_list & x)
  {
    if (this != &x)
    {
      assign(x.begin(), x.end());
    }
    
    return *this;
  }
  
  // reset the list to a given range of elements:
  template <typename input_iterator_t>
  void
  assign(input_iterator_t first2, input_iterator_t last2)
  {
    iterator first1 = begin();
    iterator last1 = end();
    for (; first1 != last1 && first2 != last2; ++first1, ++first2)
    {
      *first1 = *first2;
    }
    
    if (first2 == last2)
    {
      erase(first1, last1);
    }
    else
    {
      insert(last1, first2, last2);
    }
  }
  
  // Get a copy of the memory allocation object.
  inline allocator_type get_allocator() const
  { return base_t::get_allocator(); }
  
  // forward iterator accessors:
  inline iterator begin()
  { return base_t::begin(); }
  
  inline const_iterator begin() const
  { return base_t::begin(); }
  
  inline iterator end()
  { return base_t::end(); }
  
  inline const_iterator end() const
  { return base_t::end(); }
  
  // reverse iterators:
  inline reverse_iterator rbegin()
  { return base_t::rbegin(); }
  
  inline const_reverse_iterator rbegin() const
  { return base_t::rbegin(); }
  
  inline reverse_iterator rend()
  { return base_t::rend(); }
  
  inline const_reverse_iterator rend() const
  { return base_t::rend(); }
  
  // check whether the list is empty:
  inline bool empty() const
  { return base_t::empty(); }
  
  // return the number of elements in the list:
  inline size_type size() const
  { return base_t::size(); }
  
  inline size_type max_size() const
  { return base_t::max_size(); }
  
  // accessors:
  inline reference front()
  { return base_t::front(); }
  
  inline const_reference front() const
  { return base_t::front(); }
  
  inline reference back()
  { return base_t::back(); }
  
  inline const_reference back() const
  { return base_t::back(); }
  
  // inserters:
  virtual void push_front(const value_type & x)
  { this->insert(begin(), x); }
  
  virtual void push_back(const value_type & x)
  { this->insert(end(), x); }
  
  virtual iterator insert(iterator position, const value_type & x)
  {
    iterator it = std::find(begin(), end(), x);
    if (it != end()) return end();
    
    return base_t::insert(position, x);
  }
  
  // this will maintain the uniqueness properties of the list elements:
  template <typename input_iterator_t>
  void
  insert(iterator pos, input_iterator_t first, input_iterator_t last)
  {
    for (; first != last; ++first)
    {
      insert(pos, *first);
    }
  }
  
  // removers:
  inline iterator erase(iterator position)
  { return base_t::erase(position); }
  
  inline iterator erase(iterator first, iterator last)
  { return base_t::erase(first, last); }
  
  // swap the elements of two lists:
  inline void swap(unique_list & x)
  { base_t::swap(x); }
  
  // clear this list:
  virtual void clear()
  { base_t::clear(); }
  
  inline void remove(const data_t & value)
  { base_t::remove(value); }
  
  template<typename predicate_t>
  void remove_if(predicate_t p)
  { base_t::template remove_if<predicate_t>(p); }
  
  // NOTE: this is probably unnecessary, and it may miss
  // some duplicates if the list is not sorted:
  void unique()
  { base_t::unique(); }
  
  template<typename binary_predicate_t>
  void unique(binary_predicate_t p)
  { base_t::template unique<binary_predicate_t>(p); }
  
  // reverse the list:
  inline void reverse()
  { base_t::reverse(); }
  
  // sort the list:
  inline void sort()
  { base_t::sort(); }
  
  template<typename strict_weak_ordering_t>
  void sort(strict_weak_ordering_t o)
  { base_t::template sort<strict_weak_ordering_t>(o); }
  
  // check whether this list contains a given data element:
  inline bool has(const data_t & data) const
  { return std::find(begin(), end(), data) != end(); }
  
  // replace an element of the list with another element:
  virtual bool replace(const data_t & old_elem, const data_t & new_elem)
  {
    // avoid generating duplicates:
    if (!(old_elem == new_elem) && has(new_elem)) return false;
    
    iterator it = std::find(begin(), end(), old_elem);
    if (it == end()) return false;
    
    *it = new_elem;
    return true;
  }

  template <class list_t>
  void splice(iterator position, list_t & src, iterator first, iterator last)
  {
    for (; first != last;)
    {
      position = insert(position, *first);
      first = src.erase(first);
    }
  }
};


} // namespace the


#endif // UNIQUE_LIST_HXX_
