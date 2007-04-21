// File         : the_trail.cxx
// Author       : Paul A. Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003, 2004
// License      : GPL.
// Description  : event trail recoring/playback (regression testing).

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


//----------------------------------------------------------------
// the_trail_t::trail_
// 
the_trail_t * the_trail_t::trail_ = NULL;

//----------------------------------------------------------------
// the_trail_t::the_trail_t
// 
the_trail_t::the_trail_t(int & argc, char ** argv, bool record_by_default):
  record_by_default_(record_by_default),
  line_num_(0),
  milestone_(0),
  single_step_replay_(false),
  dont_save_events_(false),
  dont_post_events_(false),
  seconds_to_wait_(std::numeric_limits<unsigned int>::max())
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
// load_address
// 
bool
load_address(istream & si, uint64_t & address)
{
  std::string txt;
  si >> txt;
  
  address = uint64_t(0);
  uint64_t ten_to_i = 1;
  int sign = 1;
  
  unsigned int digits = txt.size();
  for (unsigned int i = 0; i < digits; i++)
  {
    char c = txt[digits - 1 - i];
    
    if (c >= '0' && c <= '9')
    {
      uint64_t d = uint64_t(c) - uint64_t('0');
      address += d * ten_to_i;
      ten_to_i *= 10;
    }
    else if (c == '-')
    {
      sign = -1;
    }
    else
    {
      return false;
    }
  }
  
  address *= sign;
  return true;
}

//----------------------------------------------------------------
// save_address
// 
void
save_address(ostream & so, uint64_t address)
{
  std::list<char> txt;
  do {
    uint64_t d = address % 10;
    txt.push_back(d + '0');
    address /= 10;
    
  } while (address != 0);
  
  std::list<char>::const_iterator iter;
  for (iter = txt.begin(); iter != txt.end(); ++iter)
  {
    so << *iter;
  }
}

//----------------------------------------------------------------
// operator >>
// 
istream &
operator >> (istream & si, uint64_t & address)
{
  load_address(si, address);
  return si;
}

//----------------------------------------------------------------
// operator <<
// 
ostream &
operator << (ostream & so, const uint64_t & address)
{
  save_address(so, address);
  return so;
}
