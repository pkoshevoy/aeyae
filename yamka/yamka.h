// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Apr  9 22:51:18 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_H_
#define YAMKA_H_

// yamka includes:
#include <yamkaFile.h>
#include <yamkaIStorage.h>
#include <yamkaFileStorage.h>
#include <yamkaElt.h>
#include <yamkaPayload.h>
#include <yamkaEBML.h>
#include <yamkaMatroska.h>


// element: type id, payload vsize, payload
// payload: type specific (container, vsize binary, vsize scalar, vsize string)
// storage: abstract mechanism to save/load binary data


#endif // YAMKA_H_
