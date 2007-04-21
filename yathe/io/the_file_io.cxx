// File         : the_file_io.cxx
// Author       : Paul A. Koshevoy
// Created      : Tue Jul 27 10:45:00 MDT 2004
// Copyright    : (C) 2004
// License      : GPL.
// Description  : 

// local includes:
#include "io/the_file_io.hxx"
#include "doc/the_document.hxx"
#include "doc/the_registry.hxx"
#include "doc/the_graph.hxx"
#include "doc/the_primitive.hxx"
#include "doc/the_reference.hxx"
#include "geom/the_point.hxx"
#include "utils/the_text.hxx"


//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, const unsigned int & data)
{
  stream << data << endl;
  return true;
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, unsigned int & data)
{
  stream >> data;
  return true;
}


//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, const int & data)
{
  stream << data << endl;
  return true;
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, int & data)
{
  stream >> data;
  return true;
}


//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, const char & data)
{
  stream << int(data) << endl;
  return true;
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, char & data)
{
  int ch;
  stream >> ch;
  data = char(ch);
  return true;
}


//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, const bool & data)
{
  unsigned int tmp = data;
  stream << tmp << endl;
  return true;
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, bool & data)
{
  unsigned int tmp = 0;
  stream >> tmp;
  data = (tmp != 0);
  return true;
}


//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, const double & data)
{
  stream << data << endl;
  return true;
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, double & data)
{
  stream >> data;
  return true;
}


//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, const float & data)
{
  stream << data << endl;
  return true;
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, float & data)
{
  stream >> data;
  return true;
}


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
  stream << data.size() << ' ' << data.text() << endl;
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
  load(stream, magic_word);
  if (magic_word != "the_registry_t") return false;
  
  // load the registry:
  load(stream, registry.dispatcher_);
  load(stream, registry.table_);
  
  const unsigned int & size = registry.table_.size();
  for (unsigned int i = 0; i < size; i++)
  {
    the_primitive_t * p = registry[i];
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
save(std::ostream & stream, const the_reference_t * ref)
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
load(std::istream & stream, the_reference_t *& ref)
{
  bool ok = false;
  ref = NULL;
  
  // verify the magic word:
  the_text_t magic_word;
  load(stream, magic_word);
  
  if (magic_word == "NULL")
  {
    ok = true;
  }
  else if (magic_word == "the_point_ref_t")
  {
    the_point_ref_t * pt_ref = new the_point_ref_t(0);
    ok = pt_ref->load(stream);
    ref = pt_ref;
  }
    
  return ok;
}


//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, const the_primitive_t * primitive)
{
  // save the magic word:
  if (primitive == NULL)
  {
    save(stream, "NULL");
    return true;
  }
  
  save(stream, primitive->name());
  return primitive->save(stream);
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, the_primitive_t *& primitive)
{
  return the_primitive_file_io().load(stream, primitive);
}


//----------------------------------------------------------------
// the_primitive_file_io
// 
the_file_io_t<the_primitive_t> & the_primitive_file_io()
{
  static the_file_io_t<the_primitive_t> io;
  return io;
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
    unsigned int num_tokens = filename.split(tokens, '/');
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
    unsigned int num_tokens = filename.split(tokens, '/');
    
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
