// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_document.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Aug 23 17:17:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Implementation of the document framework class.

// local includes:
#include "doc/the_document.hxx"
#include "io/the_file_io.hxx"
#include "math/the_bbox.hxx"
#include "utils/the_unique_list.hxx"
#include "utils/the_utils.hxx"


//----------------------------------------------------------------
// the_document_t::the_document_t
// 
the_document_t::the_document_t(const the_text_t & name):
  name_(name)
{}

//----------------------------------------------------------------
// the_document_t::regenerate
// 
bool
the_document_t::regenerate()
{
  // regenerate the unrolled procedures:
  for (std::list<unsigned int>::const_iterator i = procs_.begin();
       i != procs_.end(); ++i)
  {
    the_procedure_t * procedure = registry().elem<the_procedure_t>(*i);
    if (!procedure->regenerate())
    {
      return false;
    }
  }
  
  return true;
}

//----------------------------------------------------------------
// the_document_t::draw
// 
void
the_document_t::draw(const the_view_t & view) const
{
  for (std::list<unsigned int>::const_iterator i = procs_.begin();
       i != procs_.end(); ++i)
  {
    const the_procedure_t * procedure = registry().elem<the_procedure_t>(*i);
    procedure->draw(view);
  }
}

//----------------------------------------------------------------
// the_document_t::calc_bbox
// 
void
the_document_t::calc_bbox(const the_view_t & view, the_bbox_t & bbox) const
{
  // regenerate the unrolled procedures:
  for (std::list<unsigned int>::const_iterator i = procs_.begin();
       i != procs_.end(); ++i)
  {
    const the_procedure_t * procedure = registry().elem<the_procedure_t>(*i);
    procedure->calc_bbox(view, bbox);
  }
}

//----------------------------------------------------------------
// the_document_t::unroll
// 
void
the_document_t::unroll(the_procedure_t * proc)
{
  procs_.splice(procs_.end(),
		rolled_back_procs_,
		rolled_back_procs_.begin(),
		rolled_back_procs_.end());
  
  std::list<unsigned int>::iterator it =
    std::find(procs_.begin(), procs_.end(), proc->id());
  assert(it != procs_.end());
  
  rolled_back_procs_.splice(rolled_back_procs_.end(),
			    procs_,
			    ++it,
			    procs_.end());
}

//----------------------------------------------------------------
// the_document_t::save
// 
bool
the_document_t::save(std::ostream & stream) const
{
  // save the magic word:
  ::save(stream, "the_document_t");
  
  // save the registry:
  ::save(stream, registry_);
  
  // save the unrolled procedures:
  ::save<unsigned int>(stream, procs_);
  
  // save the rolled back procedures:
  ::save<unsigned int>(stream, rolled_back_procs_);
  
  return true;
}

//----------------------------------------------------------------
// the_document_t::load
// 
bool
the_document_t::load(std::istream & stream)
{
  // verify the magic word:
  the_text_t magic_word;
  ::load(stream, magic_word);
  if (magic_word != "the_document_t" &&
      magic_word != "the_bernstein_doc_t") // it's a relic
  {
    return false;
  }
  
  // load the registry:
  bool ok = ::load(stream, registry_);
  
  if (ok)
  {
    // load the unrolled procedures:
    ok = ::load(stream, procs_);
    
    if (ok)
    {
      // load the rolled back procedures:
      ok = ::load(stream, rolled_back_procs_);
    }
  }
  
  return ok;
}


//----------------------------------------------------------------
// save
// 
bool
save(const the_text_t & magic,
     const the_text_t & filename,
     const the_document_t * doc)
{
  assert(doc != NULL);
  
  std::ofstream file;
  file.open(filename, ios::out);
  if (!file.is_open()) return false;
  
  // save the right magic word:
  file << magic << endl;
  
  // update the document name:
  the_document_t * document = const_cast<the_document_t *>(doc);
  the_text_t old_name(doc->name());
  {
    std::vector<the_text_t> tokens;
    std::size_t num_tokens = filename.split(tokens, '/');
    document->name().assign(tokens[num_tokens - 1]);
  }
  
  // save the document:
  bool ok = document->save(file);
  if (!ok)
  {
    document->name().assign(old_name);
  }
  
  // done:
  file.close();
  return ok;
}

//----------------------------------------------------------------
// load
// 
bool
load(const the_text_t & magic,
     const the_text_t & filename,
     the_document_t *& doc)
{
  assert(doc == NULL);
  
  std::ifstream file;
  file.open(filename, ios::in);
  if (!file.is_open()) return false;
  
  // make sure this is not a bogus file:
  the_text_t magic_word;
  file >> magic_word;
  
  if (magic_word == magic)
  {
    std::vector<the_text_t> tokens;
    std::size_t num_tokens = filename.split(tokens, '/');
    
    // update the document name:
    the_document_t * document = new the_document_t(filename);
    
    // load the document:
    if (document->load(file))
    {
      document->name().assign(tokens[num_tokens - 1]);
      doc = document;
    }
  }
  
  // done:
  file.close();
  
  return doc != NULL;
}
