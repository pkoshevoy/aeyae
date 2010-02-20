// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_modifier.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 21 15:46:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Identifiers for a common set of modifier key and mouse
//                button combinations.

#ifndef THE_MODIFIER_HXX_
#define THE_MODIFIER_HXX_

//----------------------------------------------------------------
// the_modifier_t
//
// modifiers used by different tools:
// 
typedef enum
{
  THE_EH_MOD_NONE_E,
  THE_EH_MOD_VIEW_SPIN_E,
  THE_EH_MOD_VIEW_ZOOM_E,
  THE_EH_MOD_VIEW_PAN_E,
  THE_EH_MOD_VIEWING_E,
  THE_EH_MOD_PICK_E,
  THE_EH_MOD_PICK_TOGGLE_E,
  THE_EH_MOD_DRAG_E,
  THE_EH_MOD_DRAG_LOCK_HV_E,
  THE_EH_MOD_DRAG_LOCK_NORMAL_E,
  THE_EH_MOD_CURVE_EXTEND_E,
  THE_EH_MOD_CURVE_SKETCH_E,
  THE_EH_MOD_CURVE_FINISH_E,
  THE_EH_MOD_POINT_SNAP_E,
  THE_EH_MOD_GRID_START_E,
  THE_EH_MOD_GRID_EXTEND_E
} the_eh_modifier_t;


#endif // THE_MODIFIER_HXX_
