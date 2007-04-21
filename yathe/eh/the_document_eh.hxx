// File         : the_document_eh.hxx
// Author       : Paul A. Koshevoy
// Created      : Mon Aug 23 19:38:00 MDT 2004
// Copyright    : (C) 2004
// License      : GPL.
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
