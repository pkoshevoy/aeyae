/*
Copyright 2008 Pavel Koshevoy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


// File         : instance_method_call.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 14 11:17:00 MDT 2008
// Copyright    : (C) 2008
// License      : MIT
// Description  : Generic trail record/replay mechanisms.

// local includes:
#include "utils/instance_method_call.hxx"


//----------------------------------------------------------------
// instance_t::map_load_
// 
std::map<std::string, uint64_t> instance_t::map_load_;

//----------------------------------------------------------------
// instance_t::map_save_
// 
std::map<std::string, void *> instance_t::map_save_;

//----------------------------------------------------------------
// instance_t::map_signature_
// 
std::map<void *, std::string> instance_t::map_signature_;

//----------------------------------------------------------------
// instance_t::map_address_
// 
std::map<uint64_t, void *> instance_t::map_address_;

//----------------------------------------------------------------
// instance_t::instance_t
// 
instance_t::instance_t():
  address_(NULL)
{}

//----------------------------------------------------------------
// instance_t::instance_t
// 
instance_t::instance_t(const std::string & signature):
  signature_(signature),
  address_(NULL)
{
  std::map<std::string, void *>::iterator i = map_save_.find(signature_);
  if (i != map_save_.end())
  {
    address_ = i->second;
  }
}

//----------------------------------------------------------------
// instance_t::instance_t
// 
instance_t::instance_t(const std::string & signature, void * address):
  signature_(signature),
  address_(address)
{}

//----------------------------------------------------------------
// instance_t::instance_t
// 
instance_t::instance_t(void * address):
  address_(address)
{
  std::map<void *, std::string>::iterator i = map_signature_.find(address_);
  if (i != map_signature_.end())
  {
    signature_ = i->second;
  }
}

//----------------------------------------------------------------
// instance_t::save
// 
void
instance_t::save(std::ostream & so) const
{
  bool ok_to_save = is_open(so);
  if (ok_to_save)
  {
    so << "instance_t ";
  }
  
  // first, check whether you've already been saved before:
  std::map<std::string, void *>::iterator i = map_save_.find(signature_);
  bool update_map = (i == map_save_.end()) || (i->second != address_);
  if (update_map)
  {
    if (i != map_save_.end())
    {
      // erase the old mapping:
      map_signature_.erase(address_);
    }
    
    // save the instance signature:
    if (ok_to_save)
    {
      so << "signature_ ";
      ::save(so, signature_);
      so << "instance_t ";
    }
    
    // update the map so we wouldn't have to save the signature next time:
    map_save_[signature_] = address_;
    map_signature_[address_] = signature_;
  }
  
  // save the instance address:
  if (ok_to_save)
  {
    so << "address_ " << address_;
    if (update_map)
    {
      so << std::endl;
    }
    else
    {
      so << ' ';
    }
  }
}

//----------------------------------------------------------------
// instance_t::load
// 
bool
instance_t::load(std::istream & si, const std::string & magic)
{
  // load the magic word:
  if (magic != "instance_t")
  {
    return false;
  }
  
  std::string token;
  si >> token;
  if (si.eof()) return false;
  
  bool load_signature = (token == "signature_");
  if (load_signature)
  {
    if (!::load(si, signature_))
    {
      return false;
    }
    
    // the next token should be "instance_t" again:
    si >> token;
    if (si.eof()) return false;
    if (token != "instance_t") return false;
    
    // the next token should be "address_":
    si >> token;
    if (si.eof()) return false;
  }
  
  if (token != "address_")
  {
    return false;
  }
  
  uint64_t addr = 0;
  if (!load_address(si, addr))
  {
    return false;
  }
  
  if (load_signature)
  {
    // update the loaded instance map:
    map_load_[signature_] = addr;
    
    // lookup the current address:
    std::map<std::string, void *>::iterator i = map_save_.find(signature_);
    if (i == map_save_.end())
    {
      // no instance with this signature has been registered (saved) yet:
      return false;
    }
    
    address_ = i->second;
    map_address_[addr] = address_;
  }
  else
  {
    std::map<uint64_t, void *>::iterator i = map_address_.find(addr);
    if (i == map_address_.end())
    {
      // unrecognized address:
      return false;
    }
    
    address_ = i->second;
  }
  
  return true;
}


//----------------------------------------------------------------
// args_t::save
// 
void
args_t::save(std::ostream & so) const
{
  so << "args_t ";
}

//----------------------------------------------------------------
// args_t::load
// 
bool
args_t::load(std::istream & si, const std::string & magic)
{
  return (magic == "args_t");
}


//----------------------------------------------------------------
// method_t::methods_
// 
std::map<std::string, const method_t *> method_t::methods_;

//----------------------------------------------------------------
// method_t::method_t
// 
method_t::method_t(const char * signature):
  signature_(signature)
{
  methods_[signature_] = this;
}

//----------------------------------------------------------------
// method_t::lookup
// 
const method_t *
method_t::lookup(const std::string & signature)
{
  std::map<std::string, const method_t *>::iterator i =
    methods_.find(signature);
  
  if (i == methods_.end())
  {
    return NULL;
  }
  
  return i->second;
}


//----------------------------------------------------------------
// call_t::call_t
// 
call_t::call_t(const instance_t & instance,
	       const char * method_signature,
	       const boost::shared_ptr<args_t> & args):
  instance_(instance),
  method_(NULL),
  args_(args)
{
  // lookup the method:
  if (method_signature)
  {
    method_ = method_t::lookup(method_signature);
    assert(method_);
  }
}

//----------------------------------------------------------------
// call_t::call_t
// 
call_t::call_t(void * address,
	       const char * method_signature):
  instance_(address)
{
  // lookup the method:
  if (method_signature)
  {
    method_ = method_t::lookup(method_signature);
    assert(method_);
  }
}

//----------------------------------------------------------------
// call_t::save
// 
void
call_t::save(std::ostream & so) const
{
  if (!method_) return;
  bool ok_to_save = is_open(so);
  
  // save the magic token:
  if (ok_to_save)
  {
    so << "call_t ";
  }
  
  // save the instance:
  instance_.save(so);
  
  if (ok_to_save)
  {
    ::save(so, method_->signature());
  }
  
  if (args_)
  {
    args_->save(so);
  }
  
  if (ok_to_save)
  {
    so << std::endl;
  }
}

//----------------------------------------------------------------
// call_t::load
// 
bool
call_t::load(std::istream & si, const std::string & magic)
{
  // verify the magic word:
  if (magic != "call_t")
  {
    return false;
  }
  
  std::string token;
  si >> token;
  
  // load the instance pointer:
  if (!instance_.load(si, token))
  {
    return false;
  }
  
  // load the method signature:
  std::string signature;
  if (!::load(si, signature))
  {
    return false;
  }
  
  // lookup the method matching the loaded signature:
  method_ = method_t::lookup(signature);
  if (!method_) return false;
  
  // load method call arguments:
  bool ok = method_->load(si, args_);
  if (ok)
  {
    execute();
  }
  
  return ok;
}

//----------------------------------------------------------------
// call_t::execute
// 
void
call_t::execute() const
{
  if (method_)
  {
    method_->execute(instance_, args_);
  }
}
