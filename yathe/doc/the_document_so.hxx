// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: t -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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


// File         : the_document_so.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Apr 07 13:34:30 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Document state object class.

#ifndef THE_DOCUMENT_SO_HXX_
#define THE_DOCUMENT_SO_HXX_

// local includes:
#include "utils/the_text.hxx"
#include "doc/the_document.hxx"

// system includes:
#include <stack>

// Boost includes:
#include <boost/shared_ptr.hpp>


//----------------------------------------------------------------
// the_document_so_t
// 
// The document state object (manages document undo/redo
// functionality).
//
class the_document_so_t
{
public:
  the_document_so_t(const char * magic);
  ~the_document_so_t();
  
  // accessor to the document:
  inline the_document_t * document() const
  { return document_.get(); }
  
  // accessor to the document of a given type:
  template <class doc_t>
  inline doc_t * document() const
  { return dynamic_cast<doc_t *>(document_.get()); }
  
  // document change cancellation record manipulation methods:
  struct restore_t
  {
    boost::shared_ptr<the_document_t> document_;
    std::stack<boost::shared_ptr<the_document_t> > undo_;
    std::stack<boost::shared_ptr<the_document_t> > redo_;
  };
  
  void cancel_stack_save();
  bool cancel_stack_restore();
  bool cancel_stack_dismiss();
  
  // undo/redo manipulation methods:
  void save_undo_record();
  
  void undo();
  void redo();
  
  // undo/redo stack status accessors:
  inline bool undo_stack_empty() { return undo_.empty(); }
  inline bool redo_stack_empty() { return redo_.empty(); }
  
  // start a new document:
  void new_document(the_document_t * document);
  
  // close currently open document:
  void close_document();
  
  // file i/o:
  bool load_document(const the_text_t & filename);
  bool save_document(const the_text_t & filename);
  
  inline bool save_document()
  { return save_document(filename_); }
  
  // check the document filename:
  bool has_filename() const
  { return filename_.size() != 0; }
  
  // filename accessors:
  inline void set_filename(const char * filename)
  { filename_.assign(filename); }
  
  inline const the_text_t & filename() const
  { return filename_; }
  
  // return true if all modifications to the document have been saved:
  inline bool changes_saved() const
  { return changes_saved_; }
  
  inline void changes_saved(bool saved)
  { changes_saved_ = saved; }
  
  // reset the undo/redo stacks due to big non un-doable changes:
  inline void big_changes()
  {
    clear_undo_redo();
    changes_saved_ = false;
  }
  
  void clear_undo_redo();
  
private:
  // disable copy constructor and assignment operator:
  the_document_so_t(const the_document_so_t & so);
  the_document_so_t & operator = (const the_document_so_t & so);
  
  // the document:
  boost::shared_ptr<the_document_t> document_;
  
  // document change cancellation stack:
  std::stack<restore_t> cancel_;
  
  // undo/redo stacks:
  std::stack<boost::shared_ptr<the_document_t> > undo_;
  std::stack<boost::shared_ptr<the_document_t> > redo_;
  
  // this flag indicates whether all the modifications to the document
  // have been saved:
  bool changes_saved_;
  
  // the magic tag associated with the document:
  the_text_t magic_;
  
  // filename associated with the document:
  the_text_t filename_;
};


#endif // THE_DOCUMENT_SO_HXX_
