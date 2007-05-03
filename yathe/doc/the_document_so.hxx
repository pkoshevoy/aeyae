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

// system includes:
#include <stack>

// forward declarations:
class the_document_t;


//----------------------------------------------------------------
// the_document_so_t
// 
// The document state object (manages document undo/redo
// functionality).
//
class the_document_so_t
{
public:
  // helper class:
  class the_document_stack_t : public std::stack<the_document_t *>
  {
  public:
    virtual ~the_document_stack_t()
    { the_document_stack_t::clear(); }
    
    // remove and delete each document on the stack:
    void clear();
  };
  
  the_document_so_t(const char * magic);
  ~the_document_so_t();
  
  // accessor to the document:
  inline the_document_t * document() const
  { return document_; }
  
  // accessor to the document of a given type:
  template <class doc_t>
  inline doc_t * document() const
  { return dynamic_cast<doc_t *>(document_); }
  
  // undo/redo manipulation methods:
  void save_undo_record();
  
  void undo();
  void redo();
  
  // undo/redo stack status accessors:
  inline bool undo_stack_empty() { return undo_stack_.empty(); }
  inline bool redo_stack_empty() { return redo_stack_.empty(); }
  
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
  
  inline void clear_undo_redo()
  {
    undo_stack_.clear();
    redo_stack_.clear();
  }
  
private:
  // disable copy constructor and assignment operator:
  the_document_so_t(const the_document_so_t & so);
  the_document_so_t & operator = (const the_document_so_t & so);
  
  // the document:
  the_document_t * document_;
  
  // undo/redo stacks:
  the_document_stack_t undo_stack_;
  the_document_stack_t redo_stack_;
  
  // this flag indicates whether all the modifications to the document
  // have been saved:
  bool changes_saved_;
  
  // the magic tag associated with the document:
  the_text_t magic_;
  
  // filename associated with the document:
  the_text_t filename_;
};


#endif // THE_DOCUMENT_SO_HXX_
