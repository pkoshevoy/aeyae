/*
Copyright 2004-2007 University of Utah

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


// File         : the_document.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Tue Apr 04 15:38:30 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Document framework class.

#ifndef THE_DOCUMENT_HXX_
#define THE_DOCUMENT_HXX_

// local includes:
#include "doc/the_registry.hxx"
#include "doc/the_procedure.hxx"
#include "utils/the_text.hxx"

// forward declarations:
class the_view_t;
class the_bbox_t;


//----------------------------------------------------------------
// the_document_t
// 
class the_document_t
{
public:
  the_document_t(const the_text_t & name);
  
  inline the_document_t * clone() const
  { return new the_document_t(*this); }
  
  // regenerate the unrolled procedures geometry:
  bool regenerate();
  
  // draw the unrolled procedures geometry:
  void draw(const the_view_t & view) const;
  
  // calculate the bounding box of unrolled procedures geometry:
  void calc_bbox(const the_view_t & view, the_bbox_t & bbox) const;
  
  // this function splits procedures into active and rolled back:
  void unroll(the_procedure_t * proc);
  
  // append a new procedure to the unrolled procedure list:
  inline void add_proc(the_procedure_t * proc)
  {
    registry().add(proc);
    procs_.push_back(proc->id());
  }
  
  // remove a procedure from the unrolled procedure list:
  inline void del_proc(the_procedure_t * proc)
  {
    procs_.remove(proc->id());
    registry().del(proc);
  }
  
  // accessor to the currently active procedure:
  inline the_procedure_t * active_procedure() const
  {
    if (procs_.empty()) return NULL;
    return registry().elem<the_procedure_t>(procs_.back());
  }
  
  // check whether a procedure of a given type is currently active:
  template <typename proc_t>
  inline proc_t * active_procedure() const
  {
    if (procs_.empty()) return NULL;
    return dynamic_cast<proc_t *>(registry().elem(procs_.back()));
  }
  
  // accessors:
  inline const the_text_t & name() const { return name_; }
  inline       the_text_t & name()       { return name_; }
  
  inline const the_registry_t & registry() const { return registry_; }
  inline       the_registry_t & registry()       { return registry_; }
  
  // accessor to the currently unrolled procedures:
  inline const std::list<unsigned int> & procs() const
  { return procs_; }
  
  inline const std::list<unsigned int> & rolled_back_procs() const
  { return rolled_back_procs_; }
  
  // file io:
  bool save(std::ostream & stream) const;
  bool load(std::istream & stream);
  
private:
  // the name of this document:
  the_text_t name_;
  
  // the document registry:
  the_registry_t registry_;
  
  // unrolled and rolled-back procedures of the document:
  std::list<unsigned int> procs_;
  std::list<unsigned int> rolled_back_procs_;
};


#endif // THE_DOCUMENT_HXX_
