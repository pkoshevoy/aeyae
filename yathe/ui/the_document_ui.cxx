// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_document_ui.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon May 24 12:02:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : The base class for a document user interface.

// local includes:
#include "ui/the_document_ui.hxx"
#include "ui/the_procedure_ui.hxx"
#include "eh/the_view_mgr_eh.hxx"
#include "opengl/the_view.hxx"
#include "opengl/the_view_mgr.hxx"

// system includes:
#include <assert.h>


//----------------------------------------------------------------
// the_document_ui_t::doc_ui_
// 
the_document_ui_t *
the_document_ui_t::doc_ui_ = NULL;

//----------------------------------------------------------------
// the_document_ui_t::the_document_ui_t
// 
the_document_ui_t::the_document_ui_t(const char * magic):
  doc_so_(new the_document_so_t(magic)),
  active_proc_ui_(NULL),
  shared_(NULL)
{
  assert(doc_ui_ == NULL);
  if (doc_ui_ != NULL) ::exit(1);
  doc_ui_ = this;
}

//----------------------------------------------------------------
// the_document_ui_t::~the_document_ui_t
// 
the_document_ui_t::~the_document_ui_t()
{
  shutdown();
  
  assert(doc_ui_ != NULL);
  if (doc_ui_ == NULL) ::exit(2);
  doc_ui_ = NULL;
}

//----------------------------------------------------------------
// the_document_ui_t::setup
// 
void
the_document_ui_t::setup(the_view_t * shared,
			 the_view_t ** views,
			 const unsigned int & num_views)
{
  if (view_.size() != 0)
  {
    // setup should be called once:
    assert(false);
    return;
  }
  
  assert(shared_ == NULL);
  shared_ = shared;
  
  view_.resize(num_views);
  view_mgr_eh_.resize(num_views);
  
  for (unsigned int i = 0; i < view_.size(); i++)
  {
    view_[i] = views[i];
    view_[i]->assign_document(doc_so_);
    
    view_mgr_eh_[i] = new the_view_mgr_eh_t();
    view_mgr_eh_[i]->attach_view(view_[i]);
    view_mgr_eh_[i]->install();
  }
}

//----------------------------------------------------------------
// the_document_ui_t::compile_display_lists
// 
void
the_document_ui_t::compile_display_lists()
{
  if (shared_ != NULL)
  {
    shared_->gl_make_current();
    THE_POINT_SYMBOLS.compile();
    THE_ASCII_FONT.compile();
  }
  else if (!view_.empty())
  {
    view_[0]->gl_make_current();
    THE_POINT_SYMBOLS.compile();
    THE_ASCII_FONT.compile();
  }
}

//----------------------------------------------------------------
// the_document_ui_t::proc_ui_installed
// 
void
the_document_ui_t::proc_ui_installed(the_procedure_ui_t * proc_ui)
{
  assert(active_proc_ui_ == NULL);
  active_proc_ui_ = proc_ui;
  proc_ui->installed_ = true;
}

//----------------------------------------------------------------
// the_document_ui_t::proc_ui_uninstalled
// 
void
the_document_ui_t::proc_ui_uninstalled(the_procedure_ui_t * proc_ui)
{
  assert(active_proc_ui_ == proc_ui);
  active_proc_ui_ = NULL;
  proc_ui->installed_ = false;
}

//----------------------------------------------------------------
// the_document_ui_t::shutdown
// 
void
the_document_ui_t::shutdown()
{
  // delete the document:
  delete doc_so_;
  doc_so_ = NULL;
  
  // delete the view manager event handles:
  for (unsigned int i = 0; i < view_.size(); i++)
  {
    if (view_mgr_eh_[i] != NULL)
    {
      view_mgr_eh_[i]->uninstall();
      delete view_mgr_eh_[i];
      view_mgr_eh_[i] = NULL;
    }
    
    view_[i]->assign_document(NULL);
  }
  view_mgr_eh_.resize(0);
  view_.resize(0);
}
