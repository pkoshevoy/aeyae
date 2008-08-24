// File         : the_curve_selset.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun May 13 16:48:38 MDT 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A selection set class for interpolation curves.

#ifndef THE_CURVE_SELSET_HXX_
#define THE_CURVE_SELSET_HXX_

// the includes:
#include <ui/the_document_ui.hxx>
#include <doc/the_registry.hxx>
#include <doc/the_document.hxx>
#include <doc/the_procedure.hxx>
#include <sel/the_selset.hxx>
#include <geom/the_curve.hxx>

// forward declarations:
class the_curve_selset_t;


//----------------------------------------------------------------
// delete_point
// 
extern void
delete_point(the_registry_t * registry,
	     the_procedure_t * owner_proc,
	     const unsigned int & id);

//----------------------------------------------------------------
// delete_curve
// 
extern void
delete_curve(the_registry_t * registry,
	     the_procedure_t * owner_proc,
	     const unsigned int & id);


//----------------------------------------------------------------
// the_curve_selrec_t
// 
class the_curve_selrec_t : public the_selset_t
{
public:
  typedef the_selset_t super_t;
  
  the_curve_selrec_t(const unsigned int & curve_id):
    the_selset_t(new the_selset_traits_t()),
    id_(curve_id)
  {}
  
  inline bool operator == (const the_curve_selrec_t & selrec) const
  { return selrec.id_ == id_; }
  
  // accessor:
  inline const unsigned int & id() const
  { return id_; }
  
  // shortcut:
  inline the_registry_t * registry() const
  { return traits<the_selset_traits_t>()->registry(); }
  
  // accessor to the curve:
  template <typename curve_t>
  inline curve_t * get() const
  { return registry()->elem<curve_t>(id_); }
  
  // reset anchors on all points of the active curve:
  void reset_anchors() const;
  
  // activate all points:
  void activate_all_points();
  
private:
  the_curve_selrec_t();
  
  // id of the selected curve:
  unsigned int id_;
};


//----------------------------------------------------------------
// the_curve_selset_traits_t
// 
class the_curve_selset_traits_t :
  public the_base_selset_t<the_curve_selrec_t>::traits_t
{
public:
  typedef the_base_selset_t<the_curve_selrec_t>::traits_t super_t;
  
  the_curve_selset_traits_t(const the_curve_selset_t & selset):
    selset_(selset)
  {}
  
  // virtual:
  super_t * clone() const;
  bool is_valid(const the_curve_selrec_t & rec) const;
  bool activate(the_curve_selrec_t & rec) const;
  bool deactivate(the_curve_selrec_t & rec) const;
  
private:
  const the_curve_selset_t & selset_;
};


//----------------------------------------------------------------
// the_curve_selset_t
// 
class the_curve_selset_t :
  public the_base_selset_t<the_curve_selrec_t>
{
public:
  typedef the_base_selset_t<the_curve_selrec_t> super_t;
  
  // this will setup the traits:
  the_curve_selset_t();
  
  // copy constructor (for undo/redo):
  the_curve_selset_t(const the_curve_selset_t & selset);
  
  // return the first/last selected curve:
  template <typename curve_t>
  inline curve_t * head_curve() const
  { return records_.empty() ? NULL : records_.front().get<curve_t>(); }
  
  template <typename curve_t>
  inline curve_t * tail_curve() const
  { return records_.empty() ? NULL : records_.back().get<curve_t>(); }
  
  // activate a given curve and add it to the list of already
  // active curves:
  bool append_active_curve(const unsigned int & curve_id);
  
  // deactivate a given curve and remove it from the active curve list:
  bool remove_active_curve(const unsigned int & curve_id);
  
  // deactivate all active curves and clear the active curve list:
  inline void remove_active_curves()
  { deactivate_all(); }
  
  // clear all active curves and add a given curve to the list:
  bool set_active_curve(const unsigned int & curve_id);
  
  // deactivate curve if active, activate if inactive:
  void toggle_active_curve(const unsigned int & curve_id);
  
  // clear active points of all active curves:
  void remove_active_points();
  
  // search the active curves for one that owns a given point:
  unsigned int active_curve_of_point(const unsigned int & point_id) const;
  
  // reset anchors on all points of all active curves:
  void reset_anchors();
  
  // calculate the total number of active points:
  unsigned int count_active_points() const;
  
  // lookup the first selection record with a given number of active points:
  const the_curve_selrec_t *
  record_with_points(const unsigned int & num_points) const;
  
  // lookup selection record by curve id:
  const the_curve_selrec_t * record(const unsigned int & id) const;
  the_curve_selrec_t * record(const unsigned int & curve_id);
  
  // check whether the selection set contains a given curve:
  template <typename curve_t>
  inline curve_t * has_active_curve(const unsigned int & id) const
  {
    const the_curve_selrec_t * selrec = has(the_curve_selrec_t(id));
    return selrec ? selrec->get<curve_t>() : NULL;
  }
  
  // check whether the selection set contains a curve with a
  // selected giben point:
  bool has_active_point(const unsigned int & point_id,
			the_intcurve_t *& curve,
			the_point_t *& point) const;
  
  // put to gether a dependency graph using the selection for the roots:
  void graph(the_graph_t & graph) const;
  
  // helpers:
  template <typename curve_t>
  inline curve_t * get(const unsigned int & curve_id) const
  { return registry()->elem<curve_t>(curve_id); }
  
  inline the_registry_t * registry() const
  { return &(the_document_ui_t::doc_ui()->document()->registry()); }
  
  inline the_procedure_t * proc() const
  { return the_document_ui_t::doc_ui()->document()->active_procedure(); }
  
  template <typename proc_t>
  inline proc_t * proc() const
  {
    return the_document_ui_t::doc_ui()->document()->active_procedure<proc_t>();
  }
};


#endif // THE_CURVE_SELSET_HXX_
