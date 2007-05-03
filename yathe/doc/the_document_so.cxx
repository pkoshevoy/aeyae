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
// Created      : Fri Apr 07 13:37:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Document state object class.

// local includes:
#include "doc/the_document_so.hxx"
#include "doc/the_document.hxx"
#include "utils/the_indentation.hxx"
#include "io/the_file_io.hxx"


//----------------------------------------------------------------
// the_document_so_t::the_document_stack_t::clear
// 
void
the_document_so_t::the_document_stack_t::clear()
{
  while (!empty())
  {
    the_document_t * doc = top();
    delete doc;
    pop();
  }
}


//----------------------------------------------------------------
// the_document_so_t::the_document_so_t
// 
the_document_so_t::the_document_so_t(const char * magic):
  document_(NULL),
  changes_saved_(true),
  magic_(magic)
{}

//----------------------------------------------------------------
// the_document_so_t::~the_document_so_t
// 
the_document_so_t::~the_document_so_t()
{
  close_document();
}

//----------------------------------------------------------------
// the_document_so_t::save_undo_record
// 
void
the_document_so_t::save_undo_record()
{
  assert(document_ != NULL);
  undo_stack_.push(document_->clone());
  redo_stack_.clear();
  
  // the state of the document is about to change,
  // the file will probably be out of sync:
  changes_saved_ = false;
}

//----------------------------------------------------------------
// the_document_so_t::undo
// 
void
the_document_so_t::undo()
{
  if (undo_stack_.empty()) return;
  
  the_document_t * undo_document = undo_stack_.top();
  undo_stack_.pop();
  
  the_document_t * curr_document = document_;
  redo_stack_.push(curr_document);
  
  document_ = undo_document;
  document_->regenerate();
  
  // changes were undone, the file may be out of sync:
  changes_saved_ = false;
}

//----------------------------------------------------------------
// the_document_so_t::redo
// 
void
the_document_so_t::redo()
{
  if (redo_stack_.empty()) return;
  
  the_document_t * redo_document = redo_stack_.top();
  redo_stack_.pop();
  
  the_document_t * curr_document = document_;
  undo_stack_.push(curr_document);
  
  document_ = redo_document;
  document_->regenerate();
  
  // changes were redone, the file may be out of sync:
  changes_saved_ = false;
}

//----------------------------------------------------------------
// the_document_so_t::new_document
// 
void
the_document_so_t::new_document(the_document_t * document)
{
  close_document();
  document_ = document;
  changes_saved_ = true;
}

//----------------------------------------------------------------
// the_document_so_t::close_document
// 
void
the_document_so_t::close_document()
{
  delete document_;
  document_ = NULL;
  
  clear_undo_redo();
  filename_.clear();
}

//----------------------------------------------------------------
// the_document_so_t::load_document
// 
bool
the_document_so_t::load_document(const the_text_t & filename)
{
  the_document_t * new_doc = NULL;
  bool load_ok = ::load(magic_, filename, new_doc);
  if (!load_ok)
  {
    delete new_doc;
    return false;
  }
  
  new_document(new_doc);
  document()->regenerate();
  // FIXME: document()->registry().assert_sanity();
  
  set_filename(filename);
  return true;
}

//----------------------------------------------------------------
// the_document_so_t::save_document
// 
bool
the_document_so_t::save_document(const the_text_t & filename)
{
  assert(document_ != NULL);
  changes_saved_ = ::save(magic_, filename, document_);
  
  if (changes_saved_)
  {
    set_filename(filename);
  }
  
  return changes_saved_;
}
