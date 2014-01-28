// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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
  virtual ~the_document_t() {}

  virtual the_document_t * clone() const
  { return new the_document_t(*this); }

  // regenerate the unrolled procedures geometry:
  virtual bool regenerate();

  // draw the unrolled procedures geometry:
  virtual void draw(const the_view_t & view) const;

  // calculate the bounding box of unrolled procedures geometry:
  virtual void calc_bbox(const the_view_t & view, the_bbox_t & bbox) const;

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

extern bool save(const the_text_t & magic,
		 const the_text_t & filename,
		 const the_document_t * doc);
extern bool load(const the_text_t & magic,
		 const the_text_t & filename,
		 the_document_t *& doc);


#endif // THE_DOCUMENT_HXX_
