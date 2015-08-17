// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// NOTE:
//       DO NOT EDIT THIS FILE, IT IS AUTOGENERATED.
//       ALL CHANGES WILL BE LOST UPON REGENERATION.

#ifndef THE_CURSOR_HXX_
#define THE_CURSOR_HXX_


//----------------------------------------------------------------
// the_cursor_id_t
//
typedef enum
{
  THE_ARROW_COPY_CURSOR_E,
  THE_ARROW_MOVEVP_CURSOR_E,
  THE_ARROW_MOVE_CURSOR_E,
  THE_ARROW_SNAP_CURSOR_E,
  THE_ARROW_CURSOR_E,
  THE_DEFAULT_CURSOR_E,
  THE_MOVE_CURSOR_E,
  THE_PAN_CURSOR_E,
  THE_ROTATE_CURSOR_E,
  THE_SNAP_CURSOR_E,
  THE_SPIN_CURSOR_E,
  THE_ZOOM_CURSOR_E,
  THE_BLANK_CURSOR_E
} the_cursor_id_t;


//----------------------------------------------------------------
// the_cursor_t
//
class the_cursor_t
{
public:
  the_cursor_t(const the_cursor_id_t & cursor_id)
  { setup(cursor_id); }

  void setup(const the_cursor_id_t & cursor_id);

  const unsigned char * icon_;
  const unsigned char * mask_;
  unsigned int w_;
  unsigned int h_;
  unsigned int x_;
  unsigned int y_;
};


#endif // THE_CURSOR_HXX_