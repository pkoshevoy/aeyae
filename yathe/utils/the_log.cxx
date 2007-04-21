// File         : the_log.cxx
// Author       : Paul A. Koshevoy
// Created      : Fri Mar 23 11:04:53 MDT 2007
// Copyright    : (C) 2007
// License      : 
// Description  : 

// local includes:
#include "the_log.hxx"


//----------------------------------------------------------------
// null_log
// 
the_null_log_t *
null_log()
{
  static the_null_log_t * log = NULL;
  if (log == NULL)
  {
    log = new the_null_log_t;
  }
  
  return log;
}


//----------------------------------------------------------------
// cerr_log
// 
the_stream_log_t *
cerr_log()
{
  static the_stream_log_t * log = NULL;
  if (log == NULL)
  {
    log = new the_stream_log_t(std::cerr);
  }
  
  return log;
}
