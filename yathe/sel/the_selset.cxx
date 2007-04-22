// File         : the_selset.cxx
// Author       : Paul A. Koshevoy
// Created      : Sat Jan 22 13:03:00 MDT 2005
// Copyright    : (C) 2005
// License      : GPL.
// Description  : 

// the includes:
#include <sel/the_selset.hxx>
#include <ui/the_document_ui.hxx>
#include <doc/the_document.hxx>


//----------------------------------------------------------------
// the_selset_traits_t::registry
// 
the_registry_t *
the_selset_traits_t::registry() const
{
  the_document_ui_t * doc_ui = the_document_ui_t::doc_ui();
  if (doc_ui == NULL) return NULL;
  
  the_document_t * doc = doc_ui->document();
  if (doc == NULL) return NULL;
  
  return &(doc->registry());
}

//----------------------------------------------------------------
// the_selset_traits_t::is_valid
// 
bool
the_selset_traits_t::is_valid(const unsigned int & id) const
{
  return registry()->elem(id) != NULL;
}

//----------------------------------------------------------------
// the_selset_traits_t::activate
// 
bool
the_selset_traits_t::activate(const unsigned int & id) const
{
  if (!is_valid(id))
  {
    return false;
  }
  
  registry()->elem(id)->set_current_state(THE_SELECTED_STATE_E);
  return true;
}

//----------------------------------------------------------------
// the_selset_traits_t::deactivate
// 
bool
the_selset_traits_t::deactivate(const unsigned int & id) const
{
  if (!is_valid(id))
  {
    return false;
  }
  
  registry()->elem(id)->clear_state(THE_SELECTED_STATE_E);
  return true;
}
