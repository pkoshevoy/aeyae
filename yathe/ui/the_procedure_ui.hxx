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


// File         : the_procedure_ui.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Apr 10 12:00:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : The base class for a document procedure
//                (a tool) user interface.

#ifndef THE_PROCEDURE_UI_HXX_
#define THE_PROCEDURE_UI_HXX_

// local includes:
#include "ui/the_document_ui.hxx"
#include "doc/the_document_so.hxx"
#include "doc/the_document.hxx"


//----------------------------------------------------------------
// the_procedure_ui_t
// 
// The procedure user interface abstract class.
// 
class the_procedure_ui_t
{
  friend class the_document_ui_t;
  
public:
  the_procedure_ui_t(the_document_ui_t & doc_ui):
    doc_ui_(doc_ui),
    installed_(false)
  {}
  
  virtual ~the_procedure_ui_t()
  {}
  
  // accessor to the document UI object:
  inline the_document_ui_t & doc_ui()
  { return doc_ui_; }
  
  // accessor to the document object:
  inline the_document_t * document() const
  { return doc_ui_.doc_so().document(); }
  
  // start/stop the ui:
  virtual void install()
  { doc_ui().proc_ui_installed(this); }
  
  virtual void uninstall()
  { doc_ui().proc_ui_uninstalled(this); }
  
  // check whether the UI is running:
  inline const bool & installed() const
  { return installed_; }
  
  // ui update mechanism:
  virtual void sync_ui() = 0;
  
protected:
  // disable copy constructor and assignment operator:
  the_procedure_ui_t(const the_procedure_ui_t & ui);
  the_procedure_ui_t & operator = (const the_procedure_ui_t & ui);
  
  // helper functions:
  inline the_document_so_t & doc_so()
  { return doc_ui_.doc_so(); }
  
  // the procedure state object:
  the_document_ui_t & doc_ui_;
  
  // a flag indicating whether the UI is installed or not:
  bool installed_;
};


#endif // THE_PROCEDURE_UI_HXX_
