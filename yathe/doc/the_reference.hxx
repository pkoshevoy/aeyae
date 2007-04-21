// File         : the_reference.hxx
// Author       : Paul A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : GPL.
// Description  :

#ifndef THE_REFERENCE_HXX_
#define THE_REFERENCE_HXX_

// local includes:
#include "doc/the_registry.hxx"
#include "math/v3x1p3x1.hxx"

#ifndef NOUI
#include "opengl/the_point_symbols.hxx"
#endif // NOUI

// forward declarations:
class the_view_mgr_t;


//----------------------------------------------------------------
// the_reference_t
// 
// Base class for the reference types:
// 
class the_reference_t
{
public:
  the_reference_t(const unsigned int & id);
  virtual ~the_reference_t() {}
  
  // a method for cloning references (potential memory leak):
  virtual the_reference_t * clone() const = 0;
  
  // a human-readable name, should be provided by each instantiable primitive:
  virtual const char * name() const = 0;
  
  // accessor to the reference primitive:
  template <class T>
  T * references(the_registry_t * registry) const
  { return registry->template elem<T>(id_); }
  
  // calculate the 3D value of this reference:
  virtual bool eval(the_registry_t * registry, p3x1_t & wcs_pt) const = 0;
  
  // if possible, adjust this reference:
  virtual bool move(the_registry_t * registry,
		    const the_view_mgr_t & view_mgr,
		    const p3x1_t & wcs_pt) = 0;
  
  // equality test:
  virtual bool equal(const the_reference_t * ref) const
  { return id_ == ref->id_; }
  
  // equality test operator:
  inline bool operator == (const the_reference_t & ref) const
  { return equal(&ref); }
  
  // reference model primitive id accessor:
  inline const unsigned int & id() const
  { return id_; }
  
#ifndef NOUI
  // symbol id used to display this reference:
  virtual the_point_symbol_id_t symbol() const = 0;
#endif // NOUI
  
  // file io:
  virtual bool save(std::ostream & stream) const;
  virtual bool load(std::istream & stream);
  
  // For debugging, dumps the id:
  virtual void dump(ostream & strm, unsigned int indent = 0) const;
  
protected:
  the_reference_t();
  
  // the primitive that is being referenced:
  unsigned int id_;
};


#endif // THE_REFERENCE_HXX_
