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
// the_document_so_t::the_document_so_t
// 
the_document_so_t::the_document_so_t(const char * magic):
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
// the_document_so_t::cancel_stack_save
// 
void
the_document_so_t::cancel_stack_save()
{
  assert(document_);
  if (!document_) return;
  
  the_document_so_t::restore_t saved;
  saved.document_ = boost::shared_ptr<the_document_t>(document_->clone());
  saved.undo_ = undo_;
  saved.undo_.push(boost::shared_ptr<the_document_t>(document_->clone()));
  saved.redo_ = redo_;
  ::clear_stack(undo_);
  ::clear_stack(redo_);
  cancel_.push(saved);
}

//----------------------------------------------------------------
// the_document_so_t::cancel_stack_restore
// 
bool
the_document_so_t::cancel_stack_restore()
{
  if (cancel_.empty()) return false;
  
  the_document_so_t::restore_t saved = cancel_.top();
  cancel_.pop();
  saved.undo_.pop();
  undo_ = saved.undo_;
  redo_ = saved.redo_;
  document_ = saved.document_;
  document_->regenerate();
  return true;
}

//----------------------------------------------------------------
// the_document_so_t::cancel_stack_pop
// 
bool
the_document_so_t::cancel_stack_dismiss()
{
  if (cancel_.empty()) return false;
  
  the_document_so_t::restore_t saved = cancel_.top();
  cancel_.pop();
  undo_ = saved.undo_;
  ::clear_stack(redo_);
  return true;
}

//----------------------------------------------------------------
// the_document_so_t::save_undo_record
// 
void
the_document_so_t::save_undo_record()
{
  assert(document_ != NULL);
  undo_.push(boost::shared_ptr<the_document_t>(document_->clone()));
  ::clear_stack(redo_);
  
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
  if (undo_.empty()) return;
  
  boost::shared_ptr<the_document_t> undo_document = undo_.top();
  undo_.pop();
  
  boost::shared_ptr<the_document_t> curr_document = document_;
  redo_.push(curr_document);
  
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
  if (redo_.empty()) return;
  
  boost::shared_ptr<the_document_t> redo_document = redo_.top();
  redo_.pop();
  
  boost::shared_ptr<the_document_t> curr_document = document_;
  undo_.push(curr_document);
  
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
  document_ = boost::shared_ptr<the_document_t>(document);
  changes_saved_ = true;
}

//----------------------------------------------------------------
// the_document_so_t::close_document
// 
void
the_document_so_t::close_document()
{
  document_ = boost::shared_ptr<the_document_t>();
  
  ::clear_stack(cancel_);
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
  changes_saved_ = ::save(magic_, filename, document_.get());
  
  if (changes_saved_)
  {
    set_filename(filename);
  }
  
  return changes_saved_;
}

//----------------------------------------------------------------
// the_document_so_t::clear_undo_redo
// 
void
the_document_so_t::clear_undo_redo()
{
  ::clear_stack(undo_);
  ::clear_stack(redo_);
}
