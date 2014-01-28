// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_document_ui.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Apr 07 17:00:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : The base class for a document user interface.

#ifndef THE_DOCUMENT_UI_HXX_
#define THE_DOCUMENT_UI_HXX_

// system includes:
#include <vector>

// local includes:
#include "opengl/the_view_mgr_orientation.hxx"
#include "doc/the_document_so.hxx"

// forward declarations:
class the_procedure_ui_t;
class the_view_t;
class the_view_mgr_eh_t;


//----------------------------------------------------------------
// the_document_ui_t
//
// The document user interface (manages the display of the document).
//
class the_document_ui_t
{
  friend class the_procedure_ui_t;

public:
  the_document_ui_t(const char * magic);
  virtual ~the_document_ui_t();

  // NOTE: the document ui will not take over memory management
  // of the views (including the shared view), this means the subclass
  // is responsible for deleting the views when they are no longer needed:
  void setup(the_view_t * shared,
	     the_view_t ** views,
	     const unsigned int & num_views);

  // compile font and point symbol display lists:
  void compile_display_lists();

  // ui update mechanism:
  virtual void sync_ui() = 0;

  // ui close mechanism, return true if the ui was shut down:
  virtual void kill_ui() = 0;

  // These functions manage the active_proc_ui_ pointer, they will be called
  // by the procedure UI object whenever it is installed/uninstalled:
  virtual void proc_ui_installed(the_procedure_ui_t * proc_ui);
  virtual void proc_ui_uninstalled(the_procedure_ui_t * proc_ui);

  // accessor to the document state object:
  inline the_document_so_t & doc_so()
  { return *doc_so_; }

  inline const the_document_so_t & doc_so() const
  { return *doc_so_; }

  // shortcut:
  inline the_document_t * document() const
  { return (doc_so_ == NULL) ? NULL : doc_so_->document(); }

  // accessor to the currently active procedure UI:
  inline the_procedure_ui_t * active_proc_ui() const
  { return active_proc_ui_; }

  template <typename proc_ui_t>
  inline proc_ui_t * active_proc_ui() const
  { return dynamic_cast<proc_ui_t *>(active_proc_ui_); }

  // view accessors:
  inline const std::vector<the_view_t *> & views() const
  { return view_; }

  inline std::vector<the_view_t *> & views()
  { return view_; }

  inline the_view_t * view(const unsigned int i) const
  { return view_[i]; }

  template <typename view_t>
  inline view_t * view(const unsigned int i) const
  { return dynamic_cast<view_t *>(view_[i]); }

  // accessor to the widget that owns the shared OpenGL context:
  inline the_view_t * shared() const
  { return shared_; }

  // document ui singleton accessor:
  static the_document_ui_t * doc_ui()
  { return doc_ui_; }

private:
  // disable copy constructor and assignment operator:
  the_document_ui_t(const the_document_ui_t & ui);
  the_document_ui_t & operator = (const the_document_ui_t & ui);

  // global pointer to the current document ui:
  static the_document_ui_t * doc_ui_;

protected:
  // uninstall the view mnagers and delete the views:
  void shutdown();

  // the document state object:
  the_document_so_t * doc_so_;

  // pointer to the currently active procedure UI object:
  the_procedure_ui_t * active_proc_ui_;

  // the widget that owns the shared OpenGL context that will be used by
  // all the geometry views:
  the_view_t * shared_;

  // the geometry views:
  std::vector<the_view_t *> view_;

  // each view gets its own view manager event handler:
  std::vector<the_view_mgr_eh_t *> view_mgr_eh_;
};


#endif // THE_DOCUMENT_UI_HXX_
