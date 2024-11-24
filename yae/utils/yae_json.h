// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Wed Nov 27 21:43:00 MST 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_JSON_H_
#define YAE_JSON_H_

// std includes:
#include <bitset>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <time.h>
#include <vector>

#if (defined(__clang__) && __clang__) || (defined(__GNUC__) && __GNUC__ > 4)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

// boost:
#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#endif

#if (defined(__clang__) && __clang__) || (defined(__GNUC__) && __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif

// jsoncpp:
#include "json/json.h"

// yae includes:
#include "../api/yae_api.h"
#include "../api/yae_shared_ptr.h"
#include "../utils/yae_utils.h"
#include "../utils/yae_time.h"


namespace yae
{
  // forward declarations:
  struct TTime;
  struct Timespan;

  // std::map
  template <typename TKey, typename TData>
  void save(Json::Value & json, const std::map<TKey, TData> & kv);

  template <typename TKey, typename TData>
  void load(const Json::Value & json, std::map<TKey, TData> & kv);

  // std::set
  template <typename TData>
  void save(Json::Value & json, const std::set<TData> & v);

  template <typename TData>
  void load(const Json::Value & json, std::set<TData> & v);

  // std::list
  template <typename TData>
  void save(Json::Value & json, const std::list<TData> & v);

  template <typename TData>
  void load(const Json::Value & json, std::list<TData> & v);

  // std::vector
  template <typename TData>
  void save(Json::Value & json, const std::vector<TData> & v);

  template <typename TData>
  void load(const Json::Value & json, std::vector<TData> & v);

  // struct tm:
  YAE_API void save(Json::Value & json, const struct tm & tm);
  YAE_API void load(const Json::Value & json, struct tm & tm);

  // yae::TTime:
  YAE_API void save(Json::Value & json, const yae::TTime & t);
  YAE_API void load(const Json::Value & json, yae::TTime & t);

  // yae::Timespan:
  YAE_API void save(Json::Value & json, const yae::Timespan & timespan);
  YAE_API void load(const Json::Value & json, yae::Timespan & timespan);

  //----------------------------------------------------------------
  // get_u8
  //
  inline uint8_t
  get_u8(const Json::Value & json, const char * key)
  {
    return uint8_t(json[key].asUInt());
  }

  //----------------------------------------------------------------
  // get_u16
  //
  inline uint16_t
  get_u16(const Json::Value & json, const char * key)
  {
    return uint16_t(json[key].asUInt());
  }

  //----------------------------------------------------------------
  // get_u32
  //
  inline uint32_t
  get_u32(const Json::Value & json, const char * key)
  {
    return uint32_t(json[key].asUInt());
  }

  //----------------------------------------------------------------
  // get_u64
  //
  inline uint64_t
  get_u64(const Json::Value & json, const char * key)
  {
    return uint64_t(json[key].asUInt64());
  }

  //----------------------------------------------------------------
  // get_i8
  //
  inline uint8_t
  get_i8(const Json::Value & json, const char * key)
  {
    return int8_t(json[key].asInt());
  }

  //----------------------------------------------------------------
  // get_i16
  //
  inline uint16_t
  get_i16(const Json::Value & json, const char * key)
  {
    return int16_t(json[key].asInt());
  }

  //----------------------------------------------------------------
  // get_i32
  //
  inline uint32_t
  get_i32(const Json::Value & json, const char * key)
  {
    return int32_t(json[key].asInt());
  }

  //----------------------------------------------------------------
  // get_i64
  //
  inline uint64_t
  get_i64(const Json::Value & json, const char * key)
  {
    return int64_t(json[key].asInt64());
  }

  //----------------------------------------------------------------
  // get_str
  //
  inline std::string
  get_str(const Json::Value & json, const char * key)
  {
    return json[key].asString();
  }

  //----------------------------------------------------------------
  // get_bool
  //
  inline bool
  get_bool(const Json::Value & json, const char * key)
  {
    return json[key].asBool();
  }

  // Json::Value
  inline void save(Json::Value & json, const Json::Value & v)
  { json = v; }

  inline void load(const Json::Value & json, Json::Value & v)
  { v = json; }

  // uint8_t
  inline void save(Json::Value & json, uint8_t u8)
  { json = Json::Value(Json::Value::UInt(u8)); }

  inline void load(const Json::Value & json, uint8_t & u8)
  { u8 = uint8_t(json.asUInt()); }

  // uint16_t
  inline void save(Json::Value & json, uint16_t u16)
  { json = Json::Value(Json::Value::UInt(u16)); }

  inline void load(const Json::Value & json, uint16_t & u16)
  { u16 = uint16_t(json.asUInt()); }


  // uint32_t
  inline void save(Json::Value & json, uint32_t u32)
  { json = Json::Value(Json::Value::UInt(u32)); }

  inline void load(const Json::Value & json, uint32_t & u32)
  { u32 = uint32_t(json.asUInt()); }

  // uint64_t
  inline void save(Json::Value & json, uint64_t u64)
  { json = Json::Value(Json::Value::UInt64(u64)); }

  inline void load(const Json::Value & json, uint64_t & u64)
  { u64 = uint64_t(json.asUInt64()); }

  // int8_t
  inline void save(Json::Value & json, int8_t i8)
  { json = Json::Value(Json::Value::Int(i8)); }

  inline void load(const Json::Value & json, int8_t & i8)
  { i8 = int8_t(json.asInt()); }

  // int16_t
  inline void save(Json::Value & json, int16_t i16)
  { json = Json::Value(Json::Value::Int(i16)); }

  inline void load(const Json::Value & json, int16_t & i16)
  { i16 = int16_t(json.asInt()); }

  // int32_t
  inline void save(Json::Value & json, int32_t i32)
  { json = Json::Value(Json::Value::Int(i32)); }

  inline void load(const Json::Value & json, int32_t & i32)
  { i32 = int32_t(json.asInt()); }

  // int64_t
  inline void save(Json::Value & json, int64_t i64)
  { json = Json::Value(Json::Value::Int64(i64)); }

  inline void load(const Json::Value & json, int64_t & i64)
  { i64 = int64_t(json.asInt64()); }

  // bool
  inline void save(Json::Value & json, bool v)
  { json = Json::Value(v); }

  inline void load(const Json::Value & json, bool & v)
  { v = json.asBool(); }

  // float
  inline void save(Json::Value & json, float v)
  { json = Json::Value(double(v)); }

  inline void load(const Json::Value & json, float & v)
  { v = float(json.asDouble()); }

  // double
  inline void save(Json::Value & json, double v)
  { json = Json::Value(v); }

  inline void load(const Json::Value & json, double & v)
  { v = json.asDouble(); }

  // std::string
  inline void save(Json::Value & json, const std::string & v)
  { json = Json::Value(v); }

  inline void load(const Json::Value & json, std::string & v)
  { v = json.asString(); }

  // std::pair
  template <typename TKey, typename TValue>
  void
  save(Json::Value & json, const std::pair<TKey, TValue> & kv)
  {
    Json::Value array(Json::arrayValue);
    save(array[0], kv.first);
    save(array[1], kv.second);
    json = array;
  }

  template <typename TKey, typename TValue>
  void
  load(const Json::Value & json, std::pair<TKey, TValue> & kv)
  {
    load(json[0], kv.first);
    load(json[1], kv.second);
  }

  // std::bitset
  template <std::size_t nbits>
  void
  save(Json::Value & json, const std::bitset<nbits> & bits)
  {
    std::string str = bits.to_string();
    save(json, str);
  }

  template <std::size_t nbits>
  void
  load(const Json::Value & json, std::bitset<nbits> & bits)
  {
    std::string str;
    load(json, str);
    bits = std::bitset<nbits>(str);
  }

  // yae::shared_ptr
  template <typename TData>
  void
  save(Json::Value & json, const yae::shared_ptr<TData> & ptr)
  {
    if (ptr)
    {
      save(json, *ptr);
    }
    else
    {
      json = Json::Value::nullRef;
    }
  }

  template <typename TData>
  void
  load(const Json::Value & json, yae::shared_ptr<TData> & ptr)
  {
    if (json.isNull())
    {
      ptr.reset();
    }
    else
    {
      ptr.reset(new TData());
      load(json, *ptr);
    }
  }

  // yae::optional
  template <typename TData>
  void
  save(Json::Value & json, const char * key, const yae::optional<TData> & opt)
  {
    if (opt)
    {
      save(json[key], *opt);
    }
  }

  template <typename TData>
  void
  load(const Json::Value & json, const char * key, yae::optional<TData> & opt)
  {
    if (json.isMember(key))
    {
      TData data;
      load(json[key], data);
      opt.reset(data);
    }
  }

  // optional std::string
  template <typename TContainer>
  inline void
  save(Json::Value & json, const char * key, const TContainer & opt)
  {
    if (!opt.empty())
    {
      save(json[key], opt);
    }
  }

  template <typename TContainer>
  inline void
  load(const Json::Value & json, const char * key, TContainer & opt)
  {
    if (json.isMember(key))
    {
      load(json[key], opt);
    }
  }

  // std::map
  template <typename TKey, typename TData>
  void
  save(Json::Value & json, const std::map<TKey, TData> & kv)
  {
    json = Json::Value(Json::objectValue);

    typedef std::map<TKey, TData> TMap;
    for (typename TMap::const_iterator i = kv.begin(); i != kv.end(); ++i)
    {
      std::string key = boost::lexical_cast<std::string>(i->first);
      save(json[key], i->second);
    }
  }

  template <typename TKey, typename TData>
  void
  load(const Json::Value & json, std::map<TKey, TData> & kv)
  {
    kv.clear();
    for (Json::Value::const_iterator i = json.begin(); i != json.end(); ++i)
    {
      std::string key_str = i.key().asString();
      try
      {
        TKey key = boost::lexical_cast<TKey>(key_str);
        load(*i, kv[key]);
        continue;
      }
      catch (const boost::bad_lexical_cast &)
      {}

      // try again:
      yae::replace_inplace(key_str, ",", "");
      TKey key = boost::lexical_cast<TKey>(key_str);
      load(*i, kv[key]);
    }
  }

  // std::set
  template <typename TData>
  void
  save(Json::Value & json, const std::set<TData> & v)
  {
    Json::Value array(Json::arrayValue);

    typedef std::set<TData> TContainer;
    for (typename TContainer::const_iterator i = v.begin(); i != v.end(); ++i)
    {
      Json::Value value;
      save(value, *i);
      array.append(value);
    }

    json = array;
  }

  template <typename TData>
  void
  load(const Json::Value & json, std::set<TData> & v)
  {
    v.clear();
    for (Json::Value::const_iterator i = json.begin(); i != json.end(); ++i)
    {
      TData value;
      load(*i, value);
      v.insert(value);
    }
  }

  // std::list
  template <typename TData>
  void
  save(Json::Value & json, const std::list<TData> & v)
  {
    Json::Value array(Json::arrayValue);

    typedef std::list<TData> TContainer;
    for (typename TContainer::const_iterator i = v.begin(); i != v.end(); ++i)
    {
      Json::Value value;
      save(value, *i);
      array.append(value);
    }

    json = array;
  }

  template <typename TData>
  void
  load(const Json::Value & json, std::list<TData> & v)
  {
    v.clear();
    for (Json::Value::const_iterator i = json.begin(); i != json.end(); ++i)
    {
      TData value;
      load(*i, value);
      v.push_back(value);
    }
  }

  // std::vector
  template <typename TData>
  void
  save(Json::Value & json, const std::vector<TData> & v)
  {
    Json::Value array(Json::arrayValue);

    typedef std::vector<TData> TContainer;
    for (typename TContainer::const_iterator i = v.begin(); i != v.end(); ++i)
    {
      Json::Value value;
      save(value, *i);
      array.append(value);
    }

    json = array;
  }

  template <typename TData>
  void
  load(const Json::Value & json, std::vector<TData> & v)
  {
    v.clear();
    for (Json::Value::const_iterator i = json.begin(); i != json.end(); ++i)
    {
      TData value;
      load(*i, value);
      v.push_back(value);
    }
  }

  //----------------------------------------------------------------
  // to_str
  //
  inline std::string
  to_str(const Json::Value & v, const char * indent = "")
  {
    std::ostringstream oss;

    if (indent && *indent)
    {
      Json::StyledStreamWriter(indent).write(oss, v);
      return oss.str();
    }

    Json::FastWriter writer;
    return writer.write(v);
  }

}


#endif // YAE_JSON_H_
