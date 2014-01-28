// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_trail.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003, 2004
// License      : MIT
// Description  : event trail recoring/playback abstract interface,
//                used for regression testing and debugging.

// local includes:
#include "ui/the_trail.hxx"
#include "utils/the_utils.hxx"
#include "utils/the_text.hxx"


// system includes:
#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <assert.h>
#include <stdlib.h>
#include <algorithm>
#include <limits>


//----------------------------------------------------------------
// the_trail_t::trail_
//
the_trail_t * the_trail_t::trail_ = NULL;

//----------------------------------------------------------------
// the_trail_t::the_trail_t
//
the_trail_t::the_trail_t(int & argc, char ** argv, bool record_by_default):
  record_by_default_(record_by_default),
  recorded_something_(false),
  trail_is_blocked_(false),
  line_num_(0),
  milestone_(0),
  single_step_replay_(false),
  ask_the_user_(false),
  dont_load_events_(false),
  dont_save_events_(false),
  dont_post_events_(false),
  seconds_to_wait_(7)
{
  // It only makes sence to have single instance of this class,
  // so I will enforce it here:
  assert(trail_ == NULL);
  trail_ = this;

  bool given_record_name = false;
  bool given_replay_name = false;

  the_text_t trail_replay_name;
  the_text_t trail_record_name(".dont_record.txt");

  int argj = 1;
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-ask") == 0)
    {
      single_step_replay_ = true;
      ask_the_user_ = true;
    }
    else if (strcmp(argv[i], "-replay") == 0)
    {
      if ((i + 1) >= argc)
      {
	cerr << "ERROR: option -replay: usage: " << argv[0]
	     << " -replay sample-in.txt" << endl;
	::exit(1);
      }

      i++;
      trail_replay_name = argv[i];
      given_replay_name = true;
    }
    else if (strcmp(argv[i], "-record") == 0)
    {
      if ((i + 1) >= argc)
      {
	cerr << "ERROR: option -record: usage: " << argv[0]
	     << " -record sample-out.txt" << endl;
	::exit(1);
      }

      i++;
      trail_record_name = argv[i];
      given_record_name = true;
    }
    else if (strcmp(argv[i], "-wait") == 0)
    {
      if ((i + 1) >= argc)
      {
	cerr << "ERROR: option -wait: usage: " << argv[0]
	     << " -wait seconds" << endl;
	::exit(1);
      }

      i++;
      seconds_to_wait_ = the_text_t(argv[i]).toUInt();
    }
    else
    {
      // remove the arguments that deal with event playback:
      argv[argj] = argv[i];
      argj++;
    }
  }

  // update the argument parameter counter:
  argc = argj;

  // sanity check:
  if (trail_replay_name == trail_record_name)
  {
    cerr << "ERROR: trail record and replay names can not be the same, "
	 << "aborting..." << endl;
    ::exit(0);
  }

  if (given_replay_name)
  {
    replay_stream.open(trail_replay_name, ios::in);
    if (replay_stream.rdbuf()->is_open() == false)
    {
      cerr << "ERROR: could not open "
	   << trail_replay_name << " for playback"<<endl;
      ::exit(1);
    }
    else
    {
      cerr << "NOTE: starting event replay from " << trail_replay_name << endl;
    }
  }

  if (given_record_name || record_by_default_)
  {
    record_stream.open(trail_record_name, ios::out);
    if (record_stream.rdbuf()->is_open() == false)
    {
      if (given_record_name)
      {
	cerr << "ERROR: ";
      }
      else
      {
	cerr << "WARNING: ";
      }

      cerr << "could not open " << trail_record_name
	   << " trail file for recording"<<endl;

      if (given_record_name)
      {
	::exit(1);
      }
    }
  }
}

//----------------------------------------------------------------
// the_trail_t::~the_trail_t
//
the_trail_t::~the_trail_t()
{
  if (replay_stream.rdbuf()->is_open()) replay_stream.close();
  if (record_stream.rdbuf()->is_open()) record_stream.close();
}

//----------------------------------------------------------------
// the_trail_t::record
//
void
the_trail_t::record(const io_base_t & trail_event)
{
  if (record_stream.is_open())
  {
    trail_event.save(record_stream);
    recorded_something_ = true;
  }
}

//----------------------------------------------------------------
// the_trail_t::timeout
//
void
the_trail_t::timeout()
{}

//----------------------------------------------------------------
// the_trail_t::replay
//
void
the_trail_t::replay()
{}

//----------------------------------------------------------------
// the_trail_t::replay_one
//
void
the_trail_t::replay_one()
{}

//----------------------------------------------------------------
// the_trail_t::replay_done
//
void
the_trail_t::replay_done()
{
  if (replay_stream.rdbuf()->is_open())
  {
    replay_stream.close();
    cerr << "NOTE: finished event replay..." << endl;
  }

  dont_post_events_ = false;
  dont_load_events_ = true;

  // this is in case the trail ends in a state
  // where some key is pressed down:
  keybd_.forget_pressed_keys();
}

//----------------------------------------------------------------
// the_trail_t::next_milestone_achieved
//
void
the_trail_t::next_milestone_achieved()
{
  milestone_++;
}

//----------------------------------------------------------------
// the_trail_t::stop
//
void
the_trail_t::stop()
{
  replay_done();
}
