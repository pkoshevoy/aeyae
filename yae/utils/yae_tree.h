// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Apr  8 11:11:05 MDT 2012
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TREE_H_
#define YAE_TREE_H_

// std includes:
#include <assert.h>
#include <list>
#include <map>

// yae includes:
#include "yae/api/yae_assert.h"


namespace yae
{

  //----------------------------------------------------------------
  // Tree
  //
  template <typename TKey, typename TValue>
  struct Tree
  {
    typedef Tree<TKey, TValue> TTree;

    // store value in a node specified by the given key path:
    void set(const std::list<TKey> & path, const TValue & value)
    {
      TTree * root = this;

      for (typename std::list<TKey>::const_iterator i = path.begin();
           i != path.end(); ++i)
      {
        const TKey & key = *i;
        root = &(root->tree_[key]);
      }

      root->value_ = value;
    }

    //----------------------------------------------------------------
    // FringeGroup
    //
    // A fringe group is a set of sibling (same parent) leaf nodes:
    //
    struct FringeGroup
    {
      std::list<TKey> fullPath_;
      std::list<TKey> abbreviatedPath_;
      std::map<TKey, TValue> siblings_;
    };

    // assemble the fringe group list:
    void get(std::list<FringeGroup> & fringes, bool hasItems = false) const
    {
      if (tree_.empty())
      {
        return;
      }

      if (fringes.empty())
      {
        // this must be the root node, start with an empty fringe group:
        fringes.push_back(FringeGroup());
      }

      for (typename std::map<TKey, TTree>::const_iterator i = tree_.begin();
           i != tree_.end(); ++i)
      {
        const TKey & key = i->first;
        const TTree & next = i->second;

        if (next.tree_.empty())
        {
          FringeGroup & group = fringes.back();
          group.siblings_[key] = next.value_;
          hasItems = true;
        }
      }

      std::list<TKey> fullPath = fringes.back().fullPath_;
      std::list<TKey> abbreviatedPath;
      if (hasItems || tree_.size() > 1)
      {
        abbreviatedPath = fringes.back().abbreviatedPath_;
      }

      for (typename std::map<TKey, TTree>::const_iterator i = tree_.begin();
           i != tree_.end(); ++i)
      {
        const TKey & key = i->first;
        const TTree & next = i->second;

        if (!next.tree_.empty())
        {
          if (!fringes.back().siblings_.empty())
          {
            fringes.push_back(FringeGroup());
          }

          FringeGroup & group = fringes.back();
          group.abbreviatedPath_ = abbreviatedPath;
          group.abbreviatedPath_.push_back(key);

          group.fullPath_ = fullPath;
          group.fullPath_.push_back(key);

          next.get(fringes, hasItems);
        }
      }
    }

    // find the value of the first item of the first fringe group:
    const TValue * findFirstFringeItemValue() const
    {
      if (tree_.empty())
      {
        return NULL;
      }

      for (typename std::map<TKey, TTree>::const_iterator i = tree_.begin();
           i != tree_.end(); ++i)
      {
        const TTree & next = i->second;

        if (next.tree_.empty())
        {
          return &next.value_;
        }
      }

      for (typename std::map<TKey, TTree>::const_iterator i = tree_.begin();
           i != tree_.end(); ++i)
      {
        const TTree & next = i->second;

        if (!next.tree_.empty())
        {
          return next.findFirstFringeItemValue();
        }
      }

      YAE_ASSERT(false);
      return &value_;
    }

    bool remove(const std::list<TKey> & keyPath)
    {
      typename std::list<TKey>::const_iterator keyIter = keyPath.begin();
      typename std::list<TKey>::const_iterator pathEnd = keyPath.end();
      return remove(keyIter, pathEnd);
    }

    // NOTE: this is recursive:
    bool remove(typename std::list<TKey>::const_iterator & keyIter,
                const typename std::list<TKey>::const_iterator & pathEnd)
    {
      if (keyIter == pathEnd)
      {
        assert(false);
        return false;
      }

      // find the key mapping:
      const TKey & key = *keyIter;
      typename std::map<TKey, TTree>::iterator found = tree_.find(key);

      if (found != tree_.end())
      {
        ++keyIter;

        if (keyIter != pathEnd)
        {
          if (!found->second.remove(keyIter, pathEnd))
          {
            return false;
          }
        }

        if (found->second.tree_.empty())
        {
          tree_.erase(found);
        }

        return true;
      }

      // key not found:
      return false;
    }

    std::map<TKey, TTree> tree_;
    TValue value_;
  };

}


#endif // YAE_TREE_H_
