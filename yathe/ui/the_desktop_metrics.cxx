// File         : the_desktop_metrics.cxx
// Author       : Paul A. Koshevoy
// Created      : Mon Feb 5 16:02:00 MST 2007
// Copyright    : (C) 2007
// License      : GPL.
// Description  : The base class for desktop metrics (DPI)


// system includes:
#include <stdlib.h>

// local includes:
#include "ui/the_desktop_metrics.hxx"
#include "utils/the_dynamic_array.hxx"

//----------------------------------------------------------------
// metrics
// 
static the_dynamic_array_t<the_desktop_metrics_t *> metrics(0, 1, NULL);

//----------------------------------------------------------------
// the_desktop_metrics
// 
const the_desktop_metrics_t *
the_desktop_metrics(unsigned int desktop)
{
  if (desktop >= metrics.size())
  {
    return NULL;
  }
  
  return metrics[desktop];
}

//----------------------------------------------------------------
// the_desktop_metrics
// 
void
the_desktop_metrics(the_desktop_metrics_t * m, unsigned int desktop)
{
  if (desktop >= metrics.size())
  {
    metrics.resize(desktop + 1);
    metrics[desktop] = m;
  }
  else
  {
    delete metrics[desktop];
    metrics[desktop] = m;
  }
}
