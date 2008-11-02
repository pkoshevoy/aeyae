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


// File         : the_document_eh.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Aug 23 19:38:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Event handler base class for the document.

#ifndef THE_DOCUMENT_EH_HXX_
#define THE_DOCUMENT_EH_HXX_

// local includes:
#include "eh/the_input_device_eh.hxx"
#include "ui/the_document_ui.hxx"
#include "doc/the_document_so.hxx"
#include "doc/the_document.hxx"


//----------------------------------------------------------------
// the_document_eh_t
//
template <class doc_t>
class the_document_eh_t : public the_input_device_eh_t
{
public:
  the_document_eh_t(the_document_ui_t * doc_ui):
    doc_ui_(doc_ui)
  {}
  
  // accessor to the document ui:
  inline the_document_ui_t & doc_ui() const
  { return *doc_ui_; }
  
  // accessors to the document state object:
  inline the_document_so_t & doc_so()
  { return doc_ui_->doc_so(); }
  
  inline const the_document_so_t & doc_so() const
  { return doc_ui_->doc_so(); }
  
  // accessor to the document:
  inline doc_t * document() const
  { return dynamic_cast<doc_t *>(doc_so().document()); }
  
private:
  the_document_ui_t * doc_ui_;
};


#endif // THE_DOCUMENT_EH_HXX_
