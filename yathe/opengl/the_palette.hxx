// File         : the_palette.hxx
// Author       : Paul A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : GPL.
// Description  :

#ifndef THE_PALETTE_HXX_
#define THE_PALETTE_HXX_

// system includes:
#include <vector>

// local includes:
#include "math/the_color.hxx"


//----------------------------------------------------------------
// the_palette_id_t
// 
typedef enum
{
  THE_ORIGINAL_PALETTE_E,    // original color scheme used through 1999-2002
  THE_AMPAD_PALETTE_E,       // greenish engineering pad
  THE_NORCOM_PALETTE_E,      // DocuWorks quadrille ruled pad
  THE_YELLOW_BLUE_PALETTE_E, // yellowish engineering pad
  THE_RODINA_PALETTE_E,      // russian notebook colors
  THE_PALE_BLUE_PALETTE_E,   // blueish engineering pad
  THE_PALE_GREY_PALETTE_E    // greyish engineering pad
} the_palette_id_t;

//----------------------------------------------------------------
// the_ep_grid_id_t
// 
typedef enum
{
  THE_LIGHT_EP_GRID_E,
  THE_DARK_EP_GRID_E
} the_ep_grid_id_t;

//----------------------------------------------------------------
// the_palette_t
// 
class the_palette_t
{
public:
  the_palette_t(the_palette_id_t id);
  ~the_palette_t() {}
  
  // remap the palette to a different color scheme:
  void change(the_palette_id_t id);
  
  // text color:
  inline const the_color_t & text() const
  { return text_; }
  
  // text mask color:
  inline const the_color_t & mask() const
  { return mask_; }
  
  // background colors of the UL, UR, LR, LL corners of the screen:
  inline const std::vector<the_color_t> & bg() const
  { return bg_; }
  
  // edit plane grid colors:
  inline const std::vector<the_color_t> & ep() const
  { return ep_; }
  
  // coordinate system colors:
  inline const std::vector<the_color_t> & cs() const
  { return cs_; }
  
  // drag vector colors:
  inline const the_color_t & scs_drag() const
  { return scs_drag_; }
  
  inline const std::vector<the_color_t> & wcs_drag() const
  { return wcs_drag_; }
  
  // colors of the model primitives:
  inline const std::vector<the_color_t> & point() const { return point_; }
  inline const std::vector<the_color_t> & curve() const { return curve_; }
  
  // pencil color:
  inline const the_color_t & pencil() const
  { return curve_[0]; }
  
private:
  // disable the default constructor:
  the_palette_t();
  
  // text color:
  the_color_t text_;
  
  // text mask color:
  the_color_t mask_;
  
  // background colors of the UL, UR, LR, LL corners of the screen:
  std::vector<the_color_t> bg_;
  
  // edit plane grid color:
  std::vector<the_color_t> ep_;
  
  // coordinate system colors:
  std::vector<the_color_t> cs_;
  
  // drag vector colors:
  the_color_t scs_drag_;
  std::vector<the_color_t> wcs_drag_;
  
  // colors of the model primitives:
  std::vector<the_color_t> point_;
  std::vector<the_color_t> curve_;
};


#endif // THE_PALETTE_HXX_
