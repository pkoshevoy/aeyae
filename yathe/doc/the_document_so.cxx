// File         : the_document_so.hxx
// Author       : Paul A. Koshevoy
// Created      : Fri Apr 07 13:37:00 MDT 2004
// Copyright    : (C) 2004
// License      : GPL.
// Description  :

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
