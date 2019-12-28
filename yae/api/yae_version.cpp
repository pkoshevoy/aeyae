// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Oct 11 16:42:50 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <sstream>

// aeyae:
#include "yae_version.h"


//----------------------------------------------------------------
// parse_next
//
static unsigned int
parse_next(std::istringstream & iss)
{
    std::string token;
    std::getline(iss, token, '.');

    std::istringstream tmp(token);
    unsigned int scalar = 0;
    tmp >> scalar;

    return scalar;
}

//----------------------------------------------------------------
// yae_version
//
void
yae_version(unsigned int * major, unsigned int * minor, unsigned int * patch)
{
  std::istringstream iss;
  iss.str(std::string(YAE_REVISION_TIMESTAMP));

  unsigned int year = parse_next(iss);
  unsigned int month = parse_next(iss);
  unsigned int day = parse_next(iss);
  unsigned int hhmm = parse_next(iss);
  unsigned int hh = hhmm / 100;
  unsigned int mm = hhmm % 100;

  *major = (year < 2000 || year > 2255) ? 0 : (year - 2000);
  *minor = (month < 1 || month > 12) ? 0 : month;
  *patch = (day < 1 || day > 31) ? 0 : (mm + 60 * (hh + 24 * (day - 1)));
}
