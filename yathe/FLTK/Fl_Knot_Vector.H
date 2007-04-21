#ifndef Fl_Knot_Vector_H
#define Fl_Knot_Vector_H

// system includes:
#include <vector>
#include <list>

// FLTK includes:
#include <FL/Fl_Widget.H>

// local includes:
#include "math/v3x1p3x1.hxx"


//----------------------------------------------------------------
// duck_t
// 
class duck_t
{
public:
  duck_t(int X = -1,
	 int Y = -1,
	 unsigned int I = UINT_MAX):
    x(X),
    y(Y),
    i(I)
  {}
  
  inline bool operator == (const duck_t & d) const
  { return i == d.i && x == d.x && y == d.y; }
  
  int x;
  int y;
  unsigned int i;
};

//----------------------------------------------------------------
// Fl_Knot_Vector
// 
class FL_EXPORT Fl_Knot_Vector : public Fl_Widget
{
public:
  Fl_Knot_Vector(int x, int y, int w, int h, const char * label = 0);
  
  // virtual:
  int handle(int event);
  
  // accessors:
  inline const std::vector<float> & value() const
  { return knots_; }
  
  inline void value(const std::vector<float> & knots)
  {
    knots_ = knots;
    redraw();
  }
  
protected:
  // virtual:
  void draw();
  
private:
  inline void set_anchor()
  { anchor_ = ducks_; }
  
  std::vector<float> knots_;
  
  std::list<unsigned int> multiplicity_;
  std::list<float> parameter_;
  std::list<duck_t> ducks_;
  std::list<duck_t> anchor_;
  
  const duck_t * active_duck_;
};


#endif // Fl_Knot_Vector_H
