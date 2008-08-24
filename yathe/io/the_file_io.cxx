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


// File         : the_file_io.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Tue Jul 27 10:45:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : File IO helper functions for common datatypes.

// local includes:
#include "io/the_file_io.hxx"
#include "doc/the_document.hxx"
#include "doc/the_registry.hxx"
#include "doc/the_graph.hxx"
#include "doc/the_primitive.hxx"
#include "doc/the_reference.hxx"
#include "geom/the_point.hxx"
#include "geom/the_curve.hxx"
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
save(std::ostream & stream, const the_knot_point_t & k)
{
  return k.save(stream);
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, the_knot_point_t & k)
{
  return k.load(stream);
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
  return the_reference_file_io().load(stream, ref);
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
// the_reference_file_io
// 
the_file_io_t<the_reference_t> & the_reference_file_io()
{
  static the_file_io_t<the_reference_t> io;
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
