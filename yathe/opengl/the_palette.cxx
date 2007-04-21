// File         : the_palette.cxx
// Author       : Paul A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : GPL.
// Description  :

// system includes:
#include <assert.h>

// local includes:
#include "opengl/the_palette.hxx"
#include "doc/the_primitive.hxx"
#include "utils/the_utils.hxx"


//----------------------------------------------------------------
// the_palette_t::the_palette_t
// 
the_palette_t::the_palette_t(the_palette_id_t id):
  bg_(4),
  ep_(2),
  cs_(2),
  wcs_drag_(2),
  point_(THE_NUMBER_OF_STATES_E),
  curve_(THE_NUMBER_OF_STATES_E)
{
  change(id);
}

//----------------------------------------------------------------
// the_palette_t::change
// 
void
the_palette_t::change(the_palette_id_t id)
{
  // background colors of the UL, UR, LR, LL corners of the screen:
  switch (id)
  {
    case THE_ORIGINAL_PALETTE_E:
    {
      // text color:
      text_ = the_color_t(0xffffff);
      mask_ = the_color_t(0x000000);
      
      // background colors of the UL, UR, LR, LL corners of the screen:
      bg_ << the_color_t(0x1f2f7f)
	  << the_color_t(0x1f2f7f)
	  << the_color_t(0x3f7fff)
	  << the_color_t(0x3f7fff);
      
      // edit plane grid color:
      ep_ << the_color_t(0xffffff)
	  << the_color_t(0xffffff);
      
      // coordinate system colors:
      cs_ << the_color_t(0xff0000)
	  << the_color_t(0xffffff);
      
      // drag vector colors:
      scs_drag_ = the_color_t(0xff7f00);
      wcs_drag_ << the_color_t(0xffffff)
		<< the_color_t(0x000000);
      
      // colors of the model primitives:
      point_ << the_color_t(0xffffff)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x000000); // failed
      
      curve_ << the_color_t(0xffffff)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x000000); // failed
    }
    break;
    
    case THE_AMPAD_PALETTE_E:
    {
      // text color:
      text_ = the_color_t(0x000000);
      mask_ = the_color_t(0xffffff);
      
      // background colors of the UL, UR, LR, LL corners of the screen:
      bg_ << the_color_t(0xe2efba)
	  << the_color_t(0xe2efba)
	  << the_color_t(0xe2efba)
	  << the_color_t(0xe2efba);
      
      // edit plane grid color:
      ep_ << 0.3 * the_color_t(0x55af7f) + 0.7 * the_color_t(0xe2efba)
	  << the_color_t(0x55af7f);
      
      // coordinate system colors:
      cs_ << the_color_t(0xff0000)
	  << the_color_t(0xffffff);
      
      // drag vector colors:
      scs_drag_ = the_color_t(0xff7f00);
      wcs_drag_ << the_color_t(0xffffff)
		<< the_color_t(0x000000);
      
      // colors of the model primitives:
      point_ << the_color_t(0xffffff)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x000000); // failed
      
      curve_ << the_color_t(0x525439)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x0000ff); // failed
    }
    break;
    
    case THE_NORCOM_PALETTE_E:
    {
      // text color:
      text_ = the_color_t(0x000000);
      mask_ = the_color_t(0xffffff);
      
      // background colors of the UL, UR, LR, LL corners of the screen:
      bg_ << the_color_t(0xe6e2e0)
	  << the_color_t(0xe6e2e0)
	  << the_color_t(0xe6e2e0)
	  << the_color_t(0xe6e2e0);
      
      // edit plane grid color:
      ep_ << the_color_t(0x1eb0fe)
	  << the_color_t(0x1eb0fe);
      
      // coordinate system colors:
      cs_ << the_color_t(0xff0000)
	  << the_color_t(0xffffff);
      
      // drag vector colors:
      scs_drag_ = the_color_t(0xff7f00);
      wcs_drag_ << the_color_t(0xffffff)
		<< the_color_t(0x000000);
      
      // colors of the model primitives:
      point_ << the_color_t(0xffffff)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x000000); // failed
      
      curve_ << the_color_t(0x494c5a)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x000000); // failed
    }
    break;
    
    case THE_YELLOW_BLUE_PALETTE_E:
    {
      // text color:
      text_ = the_color_t(0x000000);
      mask_ = the_color_t(0xffffff);
      
      // background colors of the UL, UR, LR, LL corners of the screen:
      bg_ << the_color_t(0xeeee98)
	  << the_color_t(0xeeee98)
	  << the_color_t(0xeeee98)
	  << the_color_t(0xeeee98);
      
      // edit plane grid color:
      ep_ << the_color_t(0x7ba4cf)
	  << the_color_t(0x7ba4cf);
      
      // coordinate system colors:
      cs_ << the_color_t(0xff0000)
	  << the_color_t(0xffffff);
      
      // drag vector colors:
      scs_drag_ = the_color_t(0xff7f00);
      wcs_drag_ << the_color_t(0xffffff)
		<< the_color_t(0x000000);
      
      // colors of the model primitives:
      point_ << the_color_t(0xffffff)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x000000); // failed
      
      curve_ << the_color_t(0x504d30)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x000000); // failed
    }
    break;
    
    case THE_RODINA_PALETTE_E:
    {
      // text color:
      text_ = the_color_t(0x000000);
      mask_ = the_color_t(0xffffff);
      
      // background colors of the UL, UR, LR, LL corners of the screen:
      bg_ << the_color_t(0xfeeab8)
	  << the_color_t(0xfeeab8)
	  << the_color_t(0xfeeab8)
	  << the_color_t(0xfeeab8);
      
      // edit plane grid color:
      ep_ << the_color_t(0xaf6bde)
	  << the_color_t(0xaf6bde);
      
      // coordinate system colors:
      cs_ << the_color_t(0xff0000)
	  << the_color_t(0xffffff);
      
      // drag vector colors:
      scs_drag_ = the_color_t(0xff7f00);
      wcs_drag_ << the_color_t(0xffffff)
		<< the_color_t(0x000000);
      
      // colors of the model primitives:
      point_ << the_color_t(0xffffff)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x000000); // failed
      
      curve_ << the_color_t(0x8a8662)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x000000); // failed
    }
    break;
    
    case THE_PALE_BLUE_PALETTE_E:
    {
      // text color:
      text_ = the_color_t(0x000000);
      mask_ = the_color_t(0xffffff);
      
      // background colors of the UL, UR, LR, LL corners of the screen:
      bg_ << the_color_t(0xc0dedd)
	  << the_color_t(0xc0dedd)
	  << the_color_t(0xc0dedd)
	  << the_color_t(0xc0dedd);
      
      // edit plane grid color:
      ep_ << 0.3 * the_color_t(0x6ec8ee) + 0.7 * the_color_t(0xc0dedd)
	  << the_color_t(0x6ec8ee);
      
      // coordinate system colors:
      cs_ << the_color_t(0xff0000)
	  << the_color_t(0xffffff);
      
      // drag vector colors:
      scs_drag_ = the_color_t(0xff7f00);
      wcs_drag_ << the_color_t(0xffffff)
		<< the_color_t(0x000000);
      
      // colors of the model primitives:
      point_ << the_color_t(0xffffff)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x000000); // failed
      
      curve_ << the_color_t(0x4f585e)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x000000); // failed
    }
    break;
    
    case THE_PALE_GREY_PALETTE_E:
    {
      // text color:
      text_ = the_color_t(0xffffff);
      mask_ = the_color_t(0x000000);
      
      // background colors of the UL, UR, LR, LL corners of the screen:
      bg_ << the_color_t(0x828282)
	  << the_color_t(0x828282)
	  << the_color_t(0x828282)
	  << the_color_t(0x828282);
      
      // edit plane grid color:
      ep_ << 0.3 * the_color_t(0x494949) + 0.7 * the_color_t(0x828282)
	  << the_color_t(0x494949);
      
      // coordinate system colors:
      cs_ << the_color_t(0xff0000)
	  << the_color_t(0xffffff);
      
      // drag vector colors:
      scs_drag_ = the_color_t(0xff7f00);
      wcs_drag_ << the_color_t(0xffffff)
		<< the_color_t(0x000000);
      
      // colors of the model primitives:
      point_ << the_color_t(0xffffff)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x000000); // failed
      
      curve_ << the_color_t(0xffffff)  // regular
	     << the_color_t(0x00ff00)  // hilited
	     << the_color_t(0xff0000)  // selected
	     << the_color_t(0x000000); // failed
    }
    break;
    
    default: assert(false);
  }
}
