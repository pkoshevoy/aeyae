// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_file_io.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Tue Jul 27 10:45:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : File IO helper functions for common datatypes.

// local includes:
#include "io/the_file_io.hxx"
#include "io/io_base.hxx"
#include "doc/the_registry.hxx"
#include "doc/the_graph.hxx"
#include "doc/the_graph_node.hxx"
#include "doc/the_graph_node_ref.hxx"
#include "utils/the_text.hxx"


//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, const char * data)
{
  save(stream, the_text_t(data));
  return true;
}


//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, const the_text_t & data)
{
  stream << data.size() << ' ' << data.text() << ' ';
  return true;
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, the_text_t & data)
{
  data.assign("");
  
  size_t size = 0;
  stream >> size;
  
  // eat the whitespace:
  stream.get();
  
  if (size == 0) return true;
  
  char * text = new char [size + 1];
  for (size_t i = 0; i < size; i++)
  {
    stream.get(text[i]);
  }
  text[size] = '\0';
  
  data.assign(text, size);
  delete [] text;
  text = NULL;
  
  return true;
}


//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, const the_registry_t & registry)
{
  // save the magic word:
  save(stream, "the_registry_t");
  
  // save the registry:
  save(stream, registry.dispatcher_);
  save(stream, registry.table_);
  
  return true;
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, the_registry_t & registry)
{
  // verify the magic word:
  the_text_t magic_word;
  if (!load(stream, magic_word))
  {
    return false;
  }
  
  if (magic_word != "the_registry_t")
  {
    return false;
  }
  
  // load the registry:
  if (!load(stream, registry.dispatcher_))
  {
    return false;
  }
  
  if (!load(stream, registry.table_))
  {
    return false;
  }
  
  unsigned int size = (unsigned int)(registry.table_.size());
  for (unsigned int i = 0; i < size; i++)
  {
    the_graph_node_t * p = registry[i];
    if (p == NULL) continue;
    
    p->registry_ = &registry;
  }
  
  return true;
}


//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, const the_id_dispatcher_t & dispatcher)
{
  // save the magic word:
  save(stream, "the_id_dispatcher_t");
  
  save(stream, dispatcher.reuse_);
  save(stream, dispatcher.id_);
  
  return true;
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, the_id_dispatcher_t & dispatcher)
{
  // verify the magic word:
  the_text_t magic_word;
  load(stream, magic_word);
  if (magic_word != "the_id_dispatcher_t") return false;
  
  load(stream, dispatcher.reuse_);
  load(stream, dispatcher.id_);
  
  return true;
}


//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, const the_graph_node_ref_t * ref)
{
  if (ref == NULL)
  {
    save(stream, "NULL");
    return true;
  }
  
  save(stream, ref->name());
  return ref->save(stream);
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, the_graph_node_ref_t *& ref)
{
  return the_graph_node_ref_file_io().load(stream, ref);
}


//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, const the_graph_node_t * graph_node)
{
  // save the magic word:
  if (graph_node == NULL)
  {
    save(stream, "NULL");
    return true;
  }
  
  save(stream, graph_node->name());
  return graph_node->save(stream);
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, the_graph_node_t *& graph_node)
{
  return the_graph_node_file_io().load(stream, graph_node);
}

//----------------------------------------------------------------
// the_graph_node_file_io
// 
the_file_io_t<the_graph_node_t> &
the_graph_node_file_io()
{
  static the_file_io_t<the_graph_node_t> io;
  return io;
}

//----------------------------------------------------------------
// the_graph_node_ref_file_io
// 
the_file_io_t<the_graph_node_ref_t> &
the_graph_node_ref_file_io()
{
  static the_file_io_t<the_graph_node_ref_t> io;
  return io;
}
