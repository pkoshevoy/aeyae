// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu Dec 26 13:36:42 MST 2019
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <utility>

// boost:
#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#endif

// aeyae:
#include "yae/utils/yae_utils.h"

// yaeui:
#include "yaeAxisItem.h"
#include "yaeCheckboxItem.h"
#include "yaeFlickableArea.h"
#include "yaeImage.h"
#include "yaeItemFocus.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeTextInput.h"
#include "yaeTexturedRect.h"
#include "yaeUtilsQt.h"

// local:
#include "yaeAppView.h"


namespace yae
{

  //----------------------------------------------------------------
  // IsSelected
  //
  struct IsSelected : public TBoolExpr
  {
    IsSelected(const std::string & sel, const std::string & key):
      sel_(sel),
      key_(key)
    {}

    IsSelected(const std::string & sel, const char * key):
      sel_(sel),
      key_(key)
    {}

    // virtual:
    void evaluate(bool & result) const
    { result = (sel_ == key_); }

    const std::string & sel_;
    std::string key_;
  };

  //----------------------------------------------------------------
  // Select
  //
  struct Select : public InputArea
  {
    Select(const char * id, AppView & view, std::string & sel):
      InputArea(id),
      view_(view),
      sel_(sel)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      sel_ = Item::id_;
      view_.dataChanged();
      return true;
    }

    AppView & view_;
    std::string & sel_;
  };

  //----------------------------------------------------------------
  // AddWishlistItem
  //
  struct AddWishlistItem : public InputArea
  {
    AddWishlistItem(const char * id, AppView & view):
      InputArea(id),
      view_(view)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      view_.add_wishlist_item();
      view_.dataChanged();
      return true;
    }

    AppView & view_;
  };

  //----------------------------------------------------------------
  // EditWishlistItem
  //
  struct EditWishlistItem : public InputArea
  {
    EditWishlistItem(const char * id, AppView & view, std::string & sel):
      InputArea(id),
      view_(view),
      sel_(sel)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      sel_ = Item::id_;
      view_.edit_wishlist_item(Item::id_);
      view_.dataChanged();
      return true;
    }

    AppView & view_;
    std::string & sel_;
  };

  //----------------------------------------------------------------
  // IsWishlistSelected
  //
  struct IsWishlistSelected : public TBoolExpr
  {
    IsWishlistSelected(const std::string & sel):
      sel_(sel)
    {}

    // virtual:
    void evaluate(bool & result) const
    { result = al::starts_with(sel_, "wl: "); }

    const std::string & sel_;
    std::string key_;
  };

  //----------------------------------------------------------------
  // GetWishlistItemChannel
  //
  struct GetWishlistItemChannel : public TVarExpr
  {
    GetWishlistItemChannel(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        std::string ch_num = wi.channel_ ? wi.ch_txt() : "";
        result = TVar(ch_num);
        return;
      }

      result = TVar("");
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // GetWishlistItemTitle
  //
  struct GetWishlistItemTitle : public TVarExpr
  {
    GetWishlistItemTitle(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        result = TVar(wi.title_);
        return;
      }

      result = TVar("");
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // GetWishlistItemDescription
  //
  struct GetWishlistItemDescription : public TVarExpr
  {
    GetWishlistItemDescription(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        result = TVar(wi.description_);
        return;
      }

      result = TVar("");
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // GetWishlistItemStart
  //
  struct GetWishlistItemStart : public TVarExpr
  {
    GetWishlistItemStart(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        if (wi.when_)
        {
          std::string hhmm = wi.when_->t0_.to_hhmm();
          result = TVar(hhmm);
          return;
        }
      }

      result = TVar("");
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // GetWishlistItemEnd
  //
  struct GetWishlistItemEnd : public TVarExpr
  {
    GetWishlistItemEnd(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        if (wi.when_)
        {
          std::string hhmm = wi.when_->t1_.to_hhmm();
          result = TVar(hhmm);
          return;
        }
      }

      result = TVar("");
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // GetWishlistItemDate
  //
  struct GetWishlistItemDate : public TVarExpr
  {
    GetWishlistItemDate(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        if (wi.date_)
        {
          const struct tm & tm = *wi.date_;
          std::string yyyymmdd = yae::to_yyyymmdd(tm);
          result = TVar(yyyymmdd);
          return;
        }
      }

      result = TVar("");
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // WishlistWeekdayBtnColor
  //
  struct WishlistWeekdayBtnColor : TColorExpr
  {
    WishlistWeekdayBtnColor(const AppView & view, uint16_t wday):
      view_(view),
      wday_(wday)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      const AppStyle & style = *(view_.style_);
      if (view_.wi_edit_)
      {
        const Wishlist::Item & wi = view_.wi_edit_->second;
        if (wi.weekday_mask_)
        {
          uint16_t mask = *wi.weekday_mask_;
          if ((mask & wday_) == wday_)
          {
            result = style.cursor_.get();
            return;
          }
        }
      }

      result = style.bg_epg_tile_.get().a_scaled(0.5);
    }

    const AppView & view_;
    const uint16_t wday_;
  };

  //----------------------------------------------------------------
  // WishlistWeekdayTxtColor
  //
  struct WishlistWeekdayTxtColor : TColorExpr
  {
    WishlistWeekdayTxtColor(const AppView & view, uint16_t wday):
      view_(view),
      wday_(wday)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      const AppStyle & style = *(view_.style_);
      if (view_.wi_edit_)
      {
        const Wishlist::Item & wi = view_.wi_edit_->second;
        if (wi.weekday_mask_)
        {
          uint16_t mask = *wi.weekday_mask_;
          if ((mask & wday_) == wday_)
          {
            result = style.cursor_fg_.get();
            return;
          }
        }
      }

      result = style.fg_epg_.get().a_scaled(0.5);
    }

    const AppView & view_;
    const uint16_t wday_;
  };

  //----------------------------------------------------------------
  // WishlistWeekdayToggle
  //
  struct WishlistWeekdayToggle : public InputArea
  {
    WishlistWeekdayToggle(const char * id, AppView & view, uint16_t wday):
      InputArea(id),
      view_(view),
      wday_(wday)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        uint16_t mask = wi.weekday_mask_ ? *wi.weekday_mask_ : 0;
        mask ^= wday_;
        wi.weekday_mask_ = mask;
      }

      parent_->uncache();
      view_.requestRepaint();
      return true;
    }

    AppView & view_;
    const uint16_t wday_;
  };

  //----------------------------------------------------------------
  // GetWishlistItemMinDuration
  //
  struct GetWishlistItemMinDuration : public TVarExpr
  {
    GetWishlistItemMinDuration(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        if (wi.min_minutes_ && *wi.min_minutes_)
        {
          uint16_t n = *wi.min_minutes_;
          std::string txt = boost::lexical_cast<std::string>(n);
          result = TVar(txt);
          return;
        }
      }

      result = TVar("");
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // GetWishlistItemMaxDuration
  //
  struct GetWishlistItemMaxDuration : public TVarExpr
  {
    GetWishlistItemMaxDuration(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        if (wi.max_minutes_ && *wi.max_minutes_)
        {
          uint16_t n = *wi.max_minutes_;
          std::string txt = boost::lexical_cast<std::string>(n);
          result = TVar(txt);
          return;
        }
      }

      result = TVar("");
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // GetWishlistItemMax
  //
  struct GetWishlistItemMax : public TVarExpr
  {
    GetWishlistItemMax(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        if (wi.max_recordings_)
        {
          uint16_t n = *wi.max_recordings_;
          std::string txt = boost::lexical_cast<std::string>(n);
          result = TVar(txt);
          return;
        }
      }

      result = TVar("");
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // GetWishlistItemSkipDuplicates
  //
  struct GetWishlistItemSkipDuplicates : public TBoolExpr
  {
    GetWishlistItemSkipDuplicates(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        result = (wi.skip_duplicates_ && *wi.skip_duplicates_);
        return;
      }

      result = false;
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // OnToggleSkipDuplicates
  //
  struct OnToggleSkipDuplicates : public CheckboxItem::Action
  {
    OnToggleSkipDuplicates(const AppView & view):
      view_(view)
    {}

    // virtual:
    void operator()(const CheckboxItem & cbox) const
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        if (cbox.checked_.get())
        {
          wi.skip_duplicates_.reset();
        }
        else
        {
          wi.skip_duplicates_ = true;
        }
      }
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // GetWishlistItemEnabled
  //
  struct GetWishlistItemEnabled : public TBoolExpr
  {
    GetWishlistItemEnabled(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        result = !(wi.disabled_ && *wi.disabled_);
        return;
      }

      result = false;
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // OnToggleWishlistItemEnabled
  //
  struct OnToggleWishlistItemEnabled : public CheckboxItem::Action
  {
    OnToggleWishlistItemEnabled(const AppView & view):
      view_(view)
    {}

    // virtual:
    void operator()(const CheckboxItem & cbox) const
    {
      if (view_.wi_edit_)
      {
        Wishlist::Item & wi = view_.wi_edit_->second;
        if (cbox.checked_.get())
        {
          wi.disabled_ = true;
        }
        else
        {
          wi.disabled_.reset();
        }
      }
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // GetWishlistItemRemoveBtnText
  //
  struct GetWishlistItemRemoveBtnText : public TVarExpr
  {
    GetWishlistItemRemoveBtnText(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (view_.wi_edit_)
      {
        const std::string & wi_key = view_.wi_edit_->first;
        if (!wi_key.empty())
        {
          result = TVar("Remove");
          return;
        }
      }

      result = TVar("Cancel");
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // RemoveWishlistItem
  //
  struct RemoveWishlistItem : public InputArea
  {
    RemoveWishlistItem(const char * id, AppView & view):
      InputArea(id),
      view_(view)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      YAE_ASSERT(view_.wi_edit_);

      if (view_.wi_edit_ && !view_.wi_edit_->first.empty())
      {
        std::string wi_key = view_.wi_edit_->first;
        view_.remove_wishlist_item(wi_key);
      }

      view_.wi_edit_.reset();
      view_.sidebar_sel_ = "view_mode_program_guide";
      parent_->uncache();
      return true;
    }

    AppView & view_;
  };

  //----------------------------------------------------------------
  // SaveWishlistItem
  //
  struct SaveWishlistItem : public InputArea
  {
    SaveWishlistItem(const char * id, AppView & view):
      InputArea(id),
      view_(view)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      view_.save_wishlist_item();
      view_.wi_edit_.reset();
      view_.sidebar_sel_ = "view_mode_program_guide";
      parent_->uncache();
      return true;
    }

    AppView & view_;
  };


  //----------------------------------------------------------------
  // WatchLive
  //
  struct WatchLive : public InputArea
  {
    WatchLive(const char * id, AppView & view, uint32_t ch_num):
      InputArea(id),
      view_(view),
      ch_num_(ch_num)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onDoubleClick(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysPoint)
    {
      yae::queue_call(view_, &AppView::on_watch_live, ch_num_);
      return true;
    }

    AppView & view_;
    const uint32_t ch_num_;
  };


  //----------------------------------------------------------------
  // IsOddRow
  //
  struct IsOddRow : public TBoolExpr
  {
    IsOddRow(const std::map<std::string, std::size_t> & index,
             const std::string & id):
      index_(index),
      id_(id)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      std::size_t i = yae::at(index_, id_);
      result = ((i & 1) == 1);
    }

    const std::map<std::string, std::size_t> & index_;
    const std::string & id_;
  };


  //----------------------------------------------------------------
  // SplitterPos
  //
  struct SplitterPos : TDoubleExpr
  {
    enum Side
    {
      kLeft = 0,
      kRight = 1
    };

    SplitterPos(AppView & view, Item & container, Side side = kLeft):
      view_(view),
      container_(container),
      side_(side)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double unit_size = view_.style_->unit_size_.get();

      if (side_ == kLeft)
      {
        result = container_.left() + unit_size * 0.0;
      }
      else
      {
        result = container_.right() - unit_size * 3.0;
      }
    }

    AppView & view_;
    Item & container_;
    Side side_;
  };

  //----------------------------------------------------------------
  // Splitter
  //
  struct Splitter : public InputArea
  {
    enum Orientation
    {
      kHorizontal = 1,
      kVertical = 2,
    };

    Splitter(const char * id,
             const ItemRef & lowerBound,
             const ItemRef & upperBound,
             const ItemRef & unitSize,
             Orientation orientation,
             double initialPos = 0.0):
      InputArea(id, true),
      orientation_(orientation),
      lowerBound_(lowerBound),
      upperBound_(upperBound),
      unitSize_(unitSize),
      posAnchor_(0),
      posOffset_(0)
    {
      pos_ = addExpr(new Pos(*this));

      double v_min = lowerBound_.get();
      double v_max = upperBound_.get();
      double unit_size = unitSize_.get();
      posAnchor_ = initialPos * (v_max - v_min) / unit_size;
    }

    // virtual:
    void uncache()
    {
      lowerBound_.uncache();
      upperBound_.uncache();
      unitSize_.uncache();
      pos_.uncache();
      InputArea::uncache();
    }

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      double unit_size = unitSize_.get();
      posAnchor_ = pos_.get() / unit_size;
      posOffset_ = 0;
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      TVec2D delta = rootCSysDragEnd - rootCSysDragStart;
      double d = (orientation_ == kHorizontal) ? delta.x() : delta.y();
      double unit_size = unitSize_.get();
      posOffset_ = d / unit_size;
      pos_.uncache();

#if 0
      // this avoids uncaching the scrollview content,
      // but doesn't work well when there are nested scrollviews:
      parent_->uncacheSelfAndChildren();
#else
      // scrollviews with content that should not be uncached by the Splitter
      // should have set sv.uncacheContent_ = false
      parent_->uncache();
#endif
      return true;
    }

    Orientation orientation_;
    ItemRef lowerBound_;
    ItemRef upperBound_;
    ItemRef unitSize_;

    //----------------------------------------------------------------
    // Pos
    //
    struct Pos : TDoubleExpr
    {
      Pos(Splitter & splitter):
        splitter_(splitter)
      {}

      // virtual:
      void evaluate(double & result) const
      {
        double v_min = splitter_.lowerBound_.get();
        double v_max = splitter_.upperBound_.get();
        double unit_size = splitter_.unitSize_.get();
        double v = (splitter_.posAnchor_ + splitter_.posOffset_) * unit_size;
        result = std::min(v_max, std::max(v_min, v));
      }

      Splitter & splitter_;
    };

    ItemRef pos_;

  protected:
    double posAnchor_;
    double posOffset_;
  };


  //----------------------------------------------------------------
  // ChannelRowTop
  //
  struct ChannelRowTop : TDoubleExpr
  {
    ChannelRowTop(const AppView & view, const Item & ch_list, uint32_t ch_num):
      view_(view),
      ch_list_(ch_list),
      ch_num_(ch_num)
    {}

    void evaluate(double & result) const
    {
      double unit_size = view_.style_->unit_size_.get();
      std::size_t ch_index = yae::at(view_.ch_index_, ch_num_);
      result = ch_list_.top();
      result += (unit_size * 1.12 + 1.0) * double(ch_index);
    }

    const AppView & view_;
    const Item & ch_list_;
    const uint32_t ch_num_;
  };

  //----------------------------------------------------------------
  // LayoutItemTop
  //
  struct LayoutItemTop : TDoubleExpr
  {
    LayoutItemTop(const Layout & layout, const std::string & name):
      layout_(layout),
      name_(name)
    {}

    void evaluate(double & result) const
    {
      std::size_t index = yae::at(layout_.index_, name_);
      if (index > 0)
      {
        // recursion: top of this item is the bottom of an earlier item:
        const std::string & name = layout_.names_.at(index - 1);
        const Item & prev = *(yae::at(layout_.items_, name)->item_);
        result = prev.bottom();
      }
      else
      {
        // end recursion: top of this item is the top of its container:
        result = layout_.item_->top();
      }
    }

    const Layout & layout_;
    std::string name_;
  };

  //----------------------------------------------------------------
  // WidthMinusMargin
  //
  struct WidthMinusMargin : TDoubleExpr
  {
    WidthMinusMargin(const AppView & view,
                     const Item & item,
                     double margin):
      view_(view),
      item_(item),
      margin_(margin)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = item_.width();
      double unit_size = view_.style_->unit_size_.get();
      result -= unit_size * margin_;
    }

    const AppView & view_;
    const Item & item_;
    double margin_;
  };

  //----------------------------------------------------------------
  // gps_time_round_dn
  //
  static uint32_t
  gps_time_round_dn(uint32_t t)
  {
    // avoid rounding down by almost an hour
    // if `t` is less than 60s from a whole hour already:
    t += 59;
    t -= t % 60;

    uint32_t r = t % 3600;
    return t - r;
  }

  //----------------------------------------------------------------
  // gps_time_round_up
  //
  static uint32_t
  gps_time_round_up(uint32_t t)
  {
    // avoid rounding up by almost an hour
    // if `t` is less than 60s from a whole hour already:
    t -= t % 60;

    uint32_t r = t % 3600;
    return r ? t + (3600 - r) : t;
  }

  //----------------------------------------------------------------
  // kTimePixelScaleFactor
  //
  // static const double kTimePixelScaleFactor = 6.159;
  // static const double kTimePixelScaleFactor = 7.175;
  static const double kTimePixelScaleFactor = 7.29;

  //----------------------------------------------------------------
  // seconds_to_px
  //
  static double
  seconds_to_px(const AppView & view, uint32_t sec)
  {
    double unit_size = view.style_->unit_size_.get();
    double px = (unit_size * kTimePixelScaleFactor) * (sec / 3600.0);
    return px;
  }

  //----------------------------------------------------------------
  // px_to_seconds
  //
  static double
  px_to_seconds(const AppView & view, double px)
  {
    double unit_size = view.style_->unit_size_.get();
    double sec = (px * 3600.0) / (unit_size * kTimePixelScaleFactor);
    return sec;
  }

  //----------------------------------------------------------------
  // gps_now_round_dn
  //
  static int64_t
  gps_now_round_dn()
  {
    int64_t gps_now = TTime::gps_now().get(1);

    // round-down to whole hour:
    gps_now = gps_time_round_dn(gps_now);

    return gps_now;
  }

  //----------------------------------------------------------------
  // gps_time_to_px
  //
  static double
  gps_time_to_px(const AppView & view, uint32_t gps_time)
  {
    int64_t gps_now = gps_now_round_dn();
    bool negative = (gps_time < gps_now);
    uint32_t sec = negative ? (gps_now - gps_time) : (gps_time - gps_now);
    double px = seconds_to_px(view, sec);
    return (negative ? -px : px) + 3.0;
  }


  //----------------------------------------------------------------
  // ProgramTilePos
  //
  struct ProgramTilePos : TDoubleExpr
  {
    ProgramTilePos(const AppView & view, uint32_t gps_time):
      view_(view),
      gps_time_(gps_time)
    {}

    void evaluate(double & result) const
    {
      result = gps_time_to_px(view_, gps_time_);
    }

    const AppView & view_;
    const uint32_t gps_time_;
  };

  //----------------------------------------------------------------
  // ProgramTileWidth
  //
  struct ProgramTileWidth : TDoubleExpr
  {
    ProgramTileWidth(const AppView & view, uint32_t duration):
      view_(view),
      duration_(duration)
    {}

    void evaluate(double & result) const
    {
      result = seconds_to_px(view_, duration_);
    }

    const AppView & view_;
    const uint32_t duration_;
  };

  //----------------------------------------------------------------
  // ProgramRowPos
  //
  struct ProgramRowPos : TDoubleExpr
  {
    ProgramRowPos(const AppView & view, uint32_t ch_num):
      view_(view),
      ch_num_(ch_num)
    {}

    void evaluate(double & result) const
    {
      const yae::mpeg_ts::EPG::Channel & channel =
        yae::at(view_.epg_.channels_, ch_num_);

      if (channel.programs_.empty())
      {
        result = 0.0;
      }
      else
      {
        const yae::mpeg_ts::EPG::Program & p0 = channel.programs_.front();
        uint32_t t0 = gps_time_round_dn(p0.gps_time_);
        result = gps_time_to_px(view_, t0);
      }
    }

    const AppView & view_;
    const uint32_t ch_num_;
  };

  //----------------------------------------------------------------
  // ProgramRowLen
  //
  struct ProgramRowLen : TDoubleExpr
  {
    ProgramRowLen(const AppView & view, uint32_t ch_num):
      view_(view),
      ch_num_(ch_num)
    {}

    void evaluate(double & result) const
    {
      const yae::mpeg_ts::EPG::Channel & channel =
        yae::at(view_.epg_.channels_, ch_num_);

      if (channel.programs_.empty())
      {
        result = 0.0;
      }
      else
      {
        const yae::mpeg_ts::EPG::Program & p0 = channel.programs_.front();
        const yae::mpeg_ts::EPG::Program & p1 = channel.programs_.back();
        uint32_t t0 = gps_time_round_dn(p0.gps_time_);
        uint32_t t1 = gps_time_round_up(p1.gps_time_ + p1.duration_);
        uint32_t dt = t1 - t0;
        result = seconds_to_px(view_, dt);
      }
    }

    const AppView & view_;
    const uint32_t ch_num_;
  };

  //----------------------------------------------------------------
  // CalcDistanceLeftRight
  //
  struct CalcDistanceLeftRight : TDoubleExpr
  {
    CalcDistanceLeftRight(const Item & a,
                          const Item & b):
      a_(a),
      b_(b)
    {}

    void evaluate(double & result) const
    {
      double a = a_.right();
      double b = b_.left();
      result = b - a;
    }

    const Item & a_;
    const Item & b_;
  };


  //----------------------------------------------------------------
  // CalcViewPositionX
  //
  struct CalcViewPositionX : TDoubleExpr
  {
    CalcViewPositionX(const Scrollview & src, const Scrollview & dst):
      src_(src),
      dst_(dst)
    {}

    void evaluate(double & result) const
    {
      double x = 0;
      double w = 0;
      src_.get_content_view_x(x, w);

      const Segment & scene = dst_.content_->xExtent();
      const Segment & view = dst_.xExtent();

      if (scene.length_ <= view.length_)
      {
        result = 0.0;
        return;
      }

#if 0 // disabled to avoid introducing a misalignment between src/dst
      // at position == 1 ... for dst to remain aligned with src
      // position has to be allowed to exceen [0, 1] range:
      if (scene.end() < x + view.length_)
      {
        x = scene.end() - view.length_;
      }
#endif

      double range = scene.length_ - view.length_;
      result = (x - scene.origin_) / range;
    }

    const Scrollview & src_;
    const Scrollview & dst_;
  };


  //----------------------------------------------------------------
  // NowMarkerPos
  //
  struct NowMarkerPos : TDoubleExpr
  {
    NowMarkerPos(const AppView & view,
                 const Item & epg_header,
                 const Scrollview & hsv):
      view_(view),
      epg_header_(epg_header),
      hsv_(hsv)
    {}

    void evaluate(double & result) const
    {
      double x = 0;
      double w = 0;
      hsv_.get_content_view_x(x, w);
      double sec = px_to_seconds(view_, x);

      int64_t gps_now = TTime::gps_now().get(1);
      int64_t origin = gps_time_round_dn(gps_now);
      int64_t dt = gps_now - origin;
      result = epg_header_.left() + seconds_to_px(view_, dt - sec);
    }

    const AppView & view_;
    const Item & epg_header_;
    const Scrollview & hsv_;
  };


  //----------------------------------------------------------------
  // IsCancelled
  //
  struct IsCancelled : TBoolExpr
  {
    IsCancelled(const AppView & view, uint32_t ch_num, uint32_t gps_time):
      view_(view),
      ch_num_(ch_num),
      gps_time_(gps_time)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = true;

      std::map<uint32_t, TScheduledRecordings>::const_iterator
        found_ch = view_.schedule_.find(ch_num_);
      if (found_ch != view_.schedule_.end())
      {
        const TScheduledRecordings & schedule = found_ch->second;
        TScheduledRecordings::const_iterator found = schedule.find(gps_time_);

        if (found != schedule.end())
        {
          const TRecPtr rec_ptr = found->second->get_rec();
          const Recording::Rec & rec = *rec_ptr;
          result = rec.cancelled_;
        }
      }
    }

    const AppView & view_;
    uint32_t ch_num_;
    uint32_t gps_time_;
  };

  //----------------------------------------------------------------
  // OnToggleSchedule
  //
  struct OnToggleSchedule : CheckboxItem::Action
  {
    OnToggleSchedule(AppView & view, uint32_t ch_num, uint32_t gps_time):
      view_(view),
      ch_num_(ch_num),
      gps_time_(gps_time)
    {}

    // virtual:
    void operator()(const CheckboxItem &) const
    {
      view_.toggle_recording(ch_num_, gps_time_);
      view_.dataChanged();
    }

    AppView & view_;
    uint32_t ch_num_;
    uint32_t gps_time_;
  };

  //----------------------------------------------------------------
  // RecButtonColor
  //
  struct RecButtonColor : TColorExpr
  {
    RecButtonColor(const AppView & view, uint32_t ch_num, uint32_t gps_time):
      view_(view),
      ch_num_(ch_num),
      gps_time_(gps_time)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      const AppStyle & style = *(view_.style_);

      std::map<uint32_t, TScheduledRecordings>::const_iterator
        found_ch = view_.schedule_.find(ch_num_);
      if (found_ch != view_.schedule_.end())
      {
        const TScheduledRecordings & schedule = found_ch->second;
        TScheduledRecordings::const_iterator found = schedule.find(gps_time_);

        if (found != schedule.end())
        {
          const TRecPtr rec_ptr = found->second->get_rec();
          const Recording::Rec & rec = *rec_ptr;

          result = (rec.cancelled_ ?
                    style.bg_epg_cancelled_.get() :
                    style.bg_epg_rec_.get());
          return;
        }
      }

      result = style.fg_epg_.get().a_scaled(0.2);
    }

    const AppView & view_;
    uint32_t ch_num_;
    uint32_t gps_time_;
  };


  //----------------------------------------------------------------
  // DoesItemFit
  //
  struct DoesItemFit : TBoolExpr
  {
    DoesItemFit(const ItemRef & x0,
                const ItemRef & x1,
                const Item & item):
      x0_(x0),
      x1_(x1),
      item_(item)
    {
      x0_.disableCaching();
      x1_.disableCaching();
    }

    // virtual:
    void evaluate(bool & result) const
    {
      double x = item_.left();
      if (x < x0_.get())
      {
        result = false;
        return;
      }

      double w = item_.width();
      if (x + w > x1_.get())
      {
        result = false;
        return;
      }

      result = true;
    }

    ItemRef x0_;
    ItemRef x1_;
    const Item & item_;
  };


  //----------------------------------------------------------------
  // ProgramDetailsVisible
  //
  struct ProgramDetailsVisible : TBoolExpr
  {
    ProgramDetailsVisible(const AppView & view):
      view_(view)
    {}

     // virtual:
    void evaluate(bool & result) const
    {
      result = (view_.sidebar_sel_ == "view_mode_program_guide" &&
                view_.program_sel_);
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // GetProgramDetailsStatus
  //
  struct GetProgramDetailsStatus : public TVarExpr
  {
    GetProgramDetailsStatus(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (view_.program_sel_)
      {
        // shortcuts:
        uint32_t ch_num = view_.program_sel_->ch_num_;
        uint32_t gps_time = view_.program_sel_->gps_time_;

        const yae::mpeg_ts::EPG::Channel * channel = NULL;
        const yae::mpeg_ts::EPG::Program * program = NULL;

        if (view_.epg_.find(ch_num, gps_time, channel, program))
        {
          std::map<uint32_t, TScheduledRecordings>::const_iterator
            found_ch = view_.schedule_.find(ch_num);

          if (found_ch != view_.schedule_.end())
          {
            const TScheduledRecordings & sched = found_ch->second;
            TScheduledRecordings::const_iterator found = sched.find(gps_time);

            if (found != sched.end())
            {
              const TRecPtr rec_ptr = found->second->get_rec();
              const Recording::Rec & rec = *rec_ptr;
              if (!rec.cancelled_)
              {
                result = TVar(std::string("Scheduled to record."));
                return;
              }
            }
          }
        }
      }

      result = TVar(std::string("Not scheduled to record."));
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // find_recording
  //
  static TRecordingPtr
  find_recording(const yae::mpeg_ts::EPG & epg,
                 const std::map<uint32_t, TScheduledRecordings> & schedule,
                 const yae::shared_ptr<DVR::ChanTime> & program_sel)
  {
    TRecordingPtr recording_ptr;

    if (program_sel)
    {
      // shortcuts:
      uint32_t ch_num = program_sel->ch_num_;
      uint32_t gps_time = program_sel->gps_time_;

      const yae::mpeg_ts::EPG::Channel * channel = NULL;
      const yae::mpeg_ts::EPG::Program * program = NULL;

      if (epg.find(ch_num, gps_time, channel, program))
      {
        std::map<uint32_t, TScheduledRecordings>::const_iterator
          found_ch = schedule.find(ch_num);

        if (found_ch != schedule.end())
        {
          const TScheduledRecordings & sched = found_ch->second;
          TScheduledRecordings::const_iterator found = sched.find(gps_time);

          if (found != sched.end())
          {
            recording_ptr = found->second;
            return recording_ptr;
          }
        }

        TRecPtr rec_ptr(new Recording::Rec(*channel, *program));
        Recording::Rec & rec = *rec_ptr;
        rec.cancelled_ = true;
        recording_ptr.reset(new Recording(rec_ptr));
      }
    }

    return recording_ptr;
  }

  //----------------------------------------------------------------
  // GetProgramDetailsToggleText
  //
  struct GetProgramDetailsToggleText : public TVarExpr
  {
    GetProgramDetailsToggleText(const AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      TRecordingPtr recording_ptr = yae::find_recording(view_.epg_,
                                                        view_.schedule_,
                                                        view_.program_sel_);
      if (recording_ptr)
      {
        const TRecPtr rec_ptr = recording_ptr->get_rec();
        const Recording::Rec & rec = *rec_ptr;

        yae::shared_ptr<DVR::Playback> ready =
          view_.model()->is_ready_to_play(rec);

        if (ready)
        {
          result = TVar(std::string("Watch Now"));
          return;
        }

        if (rec.is_recordable())
        {
          result =
            rec.cancelled_ ?
            result = TVar(std::string("Record")) :
            TVar(std::string("Cancel"));
          return;
        }
      }

      result = TVar(std::string("Not Available"));
    }

    const AppView & view_;
  };

  //----------------------------------------------------------------
  // ShowProgramDetails
  //
  struct ShowProgramDetails : public InputArea
  {
    ShowProgramDetails(const char * id,
                       AppView & view,
                       uint32_t ch_num,
                       uint32_t gps_time):
      InputArea(id),
      view_(view),
      ch_num_(ch_num),
      gps_time_(gps_time)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onDoubleClick(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysPoint)
    {
      yae::queue_call(view_,
                      &AppView::show_program_details,
                      ch_num_,
                      gps_time_);
      return true;
    }

    AppView & view_;
    uint32_t ch_num_;
    uint32_t gps_time_;
  };

  //----------------------------------------------------------------
  // ProgramDetailsToggleRecording
  //
  struct ProgramDetailsToggleRecording : public InputArea
  {
    ProgramDetailsToggleRecording(const char * id, AppView & view):
      InputArea(id),
      view_(view)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      YAE_ASSERT(view_.program_sel_);
      if (!view_.program_sel_)
      {
        return false;
      }

      TRecordingPtr recording_ptr = yae::find_recording(view_.epg_,
                                                        view_.schedule_,
                                                        view_.program_sel_);
      if (recording_ptr)
      {
        const TRecPtr rec_ptr = recording_ptr->get_rec();
        const Recording::Rec & rec = *rec_ptr;

        yae::shared_ptr<DVR::Playback> playback_ptr =
          view_.model()->is_ready_to_play(rec);

        if (playback_ptr)
        {
          yae::queue_call(view_, &AppView::watch_now, playback_ptr, rec_ptr);
          return true;
        }

        if (!rec.is_recordable())
        {
          // this program was not recorded and is in the past,
          // so there is nothing to do here:
          return true;
        }
      }

      const DVR::ChanTime & program_details = *(view_.program_sel_);
      view_.toggle_recording(program_details.ch_num_,
                             program_details.gps_time_);
      parent_->uncache();
      return true;
    }

    AppView & view_;
  };

  //----------------------------------------------------------------
  // CloseProgramDetails
  //
  struct CloseProgramDetails : public InputArea
  {
    CloseProgramDetails(const char * id, AppView & view):
      InputArea(id),
      view_(view)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      view_.program_sel_.reset();
      view_.requestUncache(view_.pd_layout_.item_.get());
      view_.requestRepaint();
      return true;
    }

    AppView & view_;
  };

  //----------------------------------------------------------------
  // ProgramDetailsAddWishlist
  //
  struct ProgramDetailsAddWishlist : public InputArea
  {
    ProgramDetailsAddWishlist(const char * id, AppView & view):
      InputArea(id),
      view_(view)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      yae::shared_ptr<DVR::ChanTime> program_sel = view_.program_sel_;
      view_.program_sel_.reset();
      view_.requestUncache();
      view_.add_wishlist_item(program_sel);
      return true;
    }

    AppView & view_;
  };


  //----------------------------------------------------------------
  // ToggleRecording
  //
  struct ToggleRecording : public InputArea
  {
    ToggleRecording(const char * id,
                    AppView & view,
                    uint32_t ch_num,
                    uint32_t gps_time):
      InputArea(id),
      view_(view),
      ch_num_(ch_num),
      gps_time_(gps_time)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      view_.toggle_recording(ch_num_, gps_time_);
      parent_->uncache();
      return true;
    }

    AppView & view_;
    uint32_t ch_num_;
    uint32_t gps_time_;
  };


  //----------------------------------------------------------------
  // PlaybackRecording
  //
  struct PlaybackRecording : public InputArea
  {
    PlaybackRecording(const char * id,
                      AppView & view,
                      const std::string & rec):
      InputArea(id),
      view_(view),
      rec_(rec)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onDoubleClick(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysPoint)
    {
      yae::queue_call(view_, &AppView::playback_recording, rec_);
      return true;
    }

    AppView & view_;
    std::string rec_;
  };


  //----------------------------------------------------------------
  // DeleteRecording
  //
  struct DeleteRecording : public InputArea
  {
    DeleteRecording(const char * id, AppView & view, const std::string & rec):
      InputArea(id),
      view_(view),
      rec_(rec)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      view_.delete_recording(rec_);
      return true;
    }

    AppView & view_;
    std::string rec_;
  };


  //----------------------------------------------------------------
  // CollapseExpand
  //
  struct CollapseExpand : public InputArea
  {
    CollapseExpand(const char * id, AppView & view, const Item & item):
      InputArea(id),
      view_(view),
      item_(item)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      if (yae::has(view_.collapsed_, item_.id_))
      {
        view_.collapsed_.erase(item_.id_);
      }
      else
      {
        view_.collapsed_.insert(item_.id_);
      }

      item_.parent_->uncache();
      view_.requestRepaint();
      return true;
    }

    AppView & view_;
    const Item & item_;
  };

  //----------------------------------------------------------------
  // ListItemTop
  //
  struct ListItemTop : TDoubleExpr
  {
    ListItemTop(const AppView & view,
                const Item & body,
                const std::map<std::string, std::size_t> & index,
                const std::string & key,
                double scale = 0.6,
                double offset = 0.0):
      view_(view),
      body_(body),
      index_(index),
      key_(key),
      scale_(scale),
      offset_(offset)
    {}

    void evaluate(double & result) const
    {
      double unit_size = view_.style_->unit_size_.get();
      std::size_t index = yae::at(index_, key_);
      result = body_.top();
      result += double(index) * (unit_size * scale_ + offset_);
    }

    const AppView & view_;
    const Item & body_;
    const std::map<std::string, std::size_t> & index_;
    const std::string & key_;
    double scale_;
    double offset_;
  };

  //----------------------------------------------------------------
  // GetPlaylistSize
  //
  struct GetPlaylistSize : public TVarExpr
  {
    GetPlaylistSize(const AppView & view, const std::string & name):
      view_(view),
      name_(name)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      std::size_t n = 0;

      std::map<std::string, TRecs>::const_iterator
        found = view_.playlists_.find(name_);

      if (found != view_.playlists_.end())
      {
        n = found->second.size();
      }

      result = QVariant(QString::number(n));
    }

    const AppView & view_;
    std::string name_;
  };

  //----------------------------------------------------------------
  // humanize_size
  //
  static std::string
  humanize_size(uint64_t nbytes)
  {
    if (nbytes < 1000)
    {
      std::string sz = strfmt("%" PRIu64 " Bytes", nbytes);
      return sz;
    }

    if (nbytes < 1000000)
    {
      uint64_t kb = nbytes / 1000;
      std::string sz = strfmt("%" PRIu64 " KB", kb);
      return sz;
    }

    if (nbytes < 1000000000)
    {
      uint64_t mb = nbytes / 1000000;
      std::string sz = strfmt("%" PRIu64" MB", mb);
      return sz;
    }

    double gb = double(nbytes) * 1e-9;
    std::string sz = strfmt("%.2f GB", gb);
    return sz;
  }

  //----------------------------------------------------------------
  // GetFileSize
  //
  struct GetFileSize : public TVarExpr
  {
    GetFileSize(const std::string & path):
      path_(path)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      uint64_t nbytes = yae::stat_filesize(path_.c_str());
      std::string sz = humanize_size(nbytes);
      result = QVariant(QString::fromUtf8(sz.c_str()));
    }

    std::string path_;
  };

  //----------------------------------------------------------------
  // get_rec_attrs
  //
  static bool
  is_now_playing(const AppView & view, const std::string & basepath)
  {
    if (view.now_playing_)
    {
      const DVR::Playback & now_playing = *(view.now_playing_);
      return (now_playing.basepath_ == basepath);
    }

    return false;
  }

  //----------------------------------------------------------------
  // has_not_watched
  //
  static bool
  has_not_watched(const AppView & view, const std::string & basepath)
  {
    return !fs::exists(basepath + ".seen");
  }

  //----------------------------------------------------------------
  // GetBadgeText
  //
  struct GetBadgeText : public TVarExpr
  {
    GetBadgeText(const AppView & view, const std::string & basepath):
      view_(view),
      basepath_(basepath)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (is_now_playing(view_, basepath_))
      {
        result = QVariant(QString::fromUtf8("NOW PLAYING"));
        return;
      }

      if (has_not_watched(view_, basepath_))
      {
        result = QVariant(QString::fromUtf8("NEW"));
        return;
      }
    }

    const AppView & view_;
    std::string basepath_;
  };

  //----------------------------------------------------------------
  // ShowBadge
  //
  struct ShowBadge : public TBoolExpr
  {
    ShowBadge(const AppView & view, const std::string & basepath):
      view_(view),
      basepath_(basepath)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = (is_now_playing(view_, basepath_) ||
                has_not_watched(view_, basepath_));
    }

    const AppView & view_;
    std::string basepath_;
  };


  //----------------------------------------------------------------
  // GetTexCollapsed
  //
  struct GetTexCollapsed : public TTextureExpr
  {
    GetTexCollapsed(AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TTexturePtr & result) const
    {
      const AppStyle & style = *(view_.style());
      Item & root = *(view_.root());
      Item & hidden = root.get<Item>("hidden");

      uint32_t l = yae::floor_log2<double>(hidden.width());
      l = std::min<uint32_t>(l, 8);
      uint32_t tex_width = 1 << l;

      QImage img = triangleImage(// texture width, power of 2:
                                 tex_width,
                                 // color:
                                 style.fg_epg_.get().a_scaled(0.7),
                                 // background color:
                                 style.bg_sidebar_.get().transparent(),
                                 // rotation angle:
                                 90.0);

      style.collapsed_->setImage(img);
      result = style.collapsed_;
    }

    AppView & view_;
  };


  //----------------------------------------------------------------
  // GetTexExpanded
  //
  struct GetTexExpanded : public TTextureExpr
  {
    GetTexExpanded(AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TTexturePtr & result) const
    {
      const AppStyle & style = *(view_.style());
      Item & root = *(view_.root());
      Item & hidden = root.get<Item>("hidden");

      uint32_t l = yae::floor_log2<double>(hidden.width());
      l = std::min<uint32_t>(l, 8);
      uint32_t tex_width = 1 << l;

      QImage img = triangleImage(// texture width, power of 2:
                                 tex_width,
                                 // color:
                                 style.fg_epg_.get().a_scaled(0.7),
                                 // background color:
                                 style.bg_sidebar_.get().transparent(),
                                 // rotation angle:
                                 180.0);

      style.expanded_->setImage(img);
      result = style.expanded_;
    }

    AppView & view_;
  };


  //----------------------------------------------------------------
  // IsCollapsed
  //
  struct IsCollapsed : public TBoolExpr
  {
    IsCollapsed(const AppView & view, const Item & item):
      view_(view),
      item_(item)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = yae::has(view_.collapsed_, item_.id_);
    }

    const AppView & view_;
    const Item & item_;
  };


  //----------------------------------------------------------------
  // add_collapsible_list
  //
  static Item &
  add_collapsible_list(AppView & view,
                       AppStyle & style,
                       Item & container,
                       const char * group_name,
                       const char * title_text)
  {
    Item & root = *(view.root());
    Item & hidden = root.get<Item>("hidden");

    Item & group = container.addNew<Item>(group_name);
    group.anchors_.left_ = ItemRef::reference(container, kPropertyLeft);
    group.anchors_.right_ = ItemRef::reference(container, kPropertyRight);

    Item & header = group.addNew<Item>("header");
    header.anchors_.fill(group);
    header.anchors_.bottom_.reset();
    header.height_ = ItemRef::reference(hidden, kUnitSize, 0.67);

    CollapseExpand & toggle = header.
      add(new CollapseExpand("toggle", view, group));

    TexturedRect & collapsed = toggle.addNew<TexturedRect>("collapsed");
    TexturedRect & expanded = toggle.addNew<TexturedRect>("expanded");

    Text & title = header.addNew<Text>("title");

    Item & baseline = header.addNew<Item>("baseline_xheight");
    baseline.anchors_.fill(header);
    baseline.anchors_.top_.reset();
    baseline.anchors_.bottom_ =
      ItemRef::reference(title, kPropertyBottom);
    baseline.margins_.
      set_bottom(ItemRef::reference(title, kPropertyFontDescent, 1, -1));
    baseline.height_ =
      baseline.addExpr(new CalcGlyphHeight(title, QChar('X')), 1, 2);

    // open/close disclosure [>] button:
    toggle.height_ = ItemRef::reference(baseline, kPropertyHeight);
    toggle.width_ = ItemRef::reference(toggle, kPropertyHeight);
    toggle.anchors_.bottom_ = ItemRef::reference(baseline, kPropertyBottom);
    toggle.anchors_.left_ = ItemRef::reference(baseline, kPropertyLeft);
    toggle.margins_.set_left(ItemRef::reference(baseline, kPropertyHeight));

    expanded.visible_ = expanded.addInverse(new IsCollapsed(view, group));
    expanded.texture_ = expanded.addExpr(new GetTexExpanded(view));
    expanded.height_ = ItemRef::reference(toggle, kPropertyHeight);
    expanded.width_ = ItemRef::reference(expanded, kPropertyHeight);
    expanded.anchors_.bottom_ = ItemRef::reference(toggle, kPropertyBottom);
    expanded.anchors_.right_ = ItemRef::reference(toggle, kPropertyRight);
    expanded.margins_.
      set_top(ItemRef::reference(expanded, kPropertyHeight, -0.125));
    expanded.margins_.
      set_bottom(ItemRef::reference(expanded, kPropertyHeight, 0.125));

    collapsed.visible_ = collapsed.addExpr(new IsCollapsed(view, group));
    collapsed.texture_ = collapsed.addExpr(new GetTexCollapsed(view));
    collapsed.height_ = ItemRef::reference(toggle, kPropertyHeight);
    collapsed.width_ = ItemRef::reference(collapsed, kPropertyHeight);
    collapsed.anchors_.bottom_ = ItemRef::reference(toggle, kPropertyBottom);
    collapsed.anchors_.right_ = ItemRef::reference(toggle, kPropertyRight);
    collapsed.margins_.
      set_left(ItemRef::reference(collapsed, kPropertyHeight, -0.125));
    collapsed.margins_.
      set_right(ItemRef::reference(collapsed, kPropertyHeight, 0.125));

    title.font_ = style.font_;
    title.font_.setBold(true);
    title.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
    title.anchors_.vcenter_ = ItemRef::reference(header, kPropertyVCenter);
    title.anchors_.left_ = ItemRef::reference(toggle, kPropertyRight);
    title.anchors_.right_ = ItemRef::reference(header, kPropertyRight);
    title.margins_.
      set_left(ItemRef::reference(baseline, kPropertyHeight, 0.5));

    title.elide_ = Qt::ElideRight;
    title.color_ = title.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
    title.background_ = title.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    title.text_ = TVarRef::constant(TVar(title_text));

    Item & body = group.addNew<Item>("body");
    body.anchors_.top_ = ItemRef::reference(header, kPropertyBottom);
    body.anchors_.left_ = ItemRef::reference(title, kPropertyLeft);
    body.anchors_.right_ = ItemRef::reference(header, kPropertyRight);
    body.visible_ = collapsed.addInverse(new IsCollapsed(view, group));
    body.height_ = body.addExpr(new InvisibleItemZeroHeight(body));

    return group;
  }


  //----------------------------------------------------------------
  // IsBlacklisted
  //
  struct IsBlacklisted : public TBoolExpr
  {
    IsBlacklisted(const AppView & view, uint16_t major, uint16_t minor):
      view_(view),
      ch_num_(yae::mpeg_ts::channel_number(major, minor))
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = yae::has(view_.blacklist_.channels_, ch_num_);
    }

    const AppView & view_;
    uint32_t ch_num_;
  };

  //----------------------------------------------------------------
  // OnToggleBlacklist
  //
  struct OnToggleBlacklist : CheckboxItem::Action
  {
    OnToggleBlacklist(AppView & view, uint16_t major, uint16_t minor):
      view_(view),
      ch_num_(yae::mpeg_ts::channel_number(major, minor))
    {}

    // virtual:
    void operator()(const CheckboxItem &) const
    {
      view_.model()->toggle_blacklist(ch_num_);
      view_.model()->save_blacklist();
      view_.sync_ui();
    }

    AppView & view_;
    uint32_t ch_num_;
  };


  //----------------------------------------------------------------
  // AppView::AppView
  //
  AppView::AppView():
    ItemView("AppView"),
    dvr_(NULL),
    sync_ui_(this),
    gps_hour_(gps_now_round_dn()),
    sidebar_sel_("view_mode_program_guide"),
    epg_lastmod_(0, 0)
  {
    Item & root = *root_;

    // add style to the root item, so it could be uncached automatically:
    style_.reset(new AppStyle("AppStyle", *this));

    bool ok = connect(&sync_ui_, SIGNAL(timeout()), this, SLOT(sync_ui()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // AppView::setEnabled
  //
  void
  AppView::setEnabled(bool enable)
  {
    if (isEnabled() == enable)
    {
      return;
    }

    ItemFocus::singleton().enable("wishlist_ui", enable);
    ItemView::setEnabled(enable);

    if (enable)
    {
      sync_ui_.start(1000);
    }
    else
    {
      sync_ui_.stop();
    }
  }

  //----------------------------------------------------------------
  // AppView::setContext
  //
  void
  AppView::setContext(const yae::shared_ptr<IOpenGLContext> & context)
  {
    if (!context)
    {
      sync_ui_.stop();
      worker_.stop();
      setEnabled(false);

      sidebar_sel_.clear();
      collapsed_.clear();
      sideview_.reset();
      mainview_.reset();
      epg_view_.reset();
      blacklist_.clear();
      channels_.clear();
      schedule_.clear();
      recordings_.clear();
      rec_by_channel_.clear();
      now_playing_.reset();
      program_sel_.reset();
      ch_index_.clear();
      ch_tile_.clear();
      ch_row_.clear();
      ch_prog_.clear();
      tickmark_.clear();
      rec_highlight_.clear();
      pd_layout_.clear();
      ch_layout_.clear();
      sch_layout_.clear();
      pl_index_.clear();
      pl_sidebar_.clear();
      pl_layout_.clear();
      wl_index_.clear();
      wl_sidebar_.clear();
      wishlist_ui_.reset();
      wi_edit_.reset();
      root_.reset();
      style_.reset();
    }

    ItemView::setContext(context);

    if (context)
    {
      sync_ui_.start(1000);
    }
  }

  //----------------------------------------------------------------
  // AppView::setModel
  //
  void
  AppView::setModel(yae::DVR * dvr)
  {
    if (dvr_ == dvr)
    {
      return;
    }

    // FIXME: disconnect previous model:
    YAE_ASSERT(!dvr_);

    dvr_ = dvr;
  }

  //----------------------------------------------------------------
  // AppView::resizeTo
  //
  bool
  AppView::resizeTo(const Canvas * canvas)
  {
    if (!ItemView::resizeTo(canvas))
    {
      return false;
    }

    if (epg_view_)
    {
      Item & epg_view = *epg_view_;
      Scrollview & vsv = epg_view.get<Scrollview>("vsv");
      Item & vsv_content = *(vsv.content_);
      Scrollview & hsv = vsv_content.get<Scrollview>("hsv");
      requestUncache(hsv.content_);
    }

    return true;
  }

  //----------------------------------------------------------------
  // AppView::processKeyEvent
  //
  bool
  AppView::processKeyEvent(Canvas * canvas, QKeyEvent * e)
  {
#if 0 // ndef NDEBUG
    // FIXME: pkoshevoy: for debvugging only:
    if (root_)
    {
      root_->dump(std::cerr);
    }
#endif
    return ItemView::processKeyEvent(canvas, e);
  }

  //----------------------------------------------------------------
  // AppView::processMouseTracking
  //
  bool
  AppView::processMouseTracking(const TVec2D & mousePt)
  {
    if (!this->isEnabled())
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // find_item_under_mouse
  //
  template <typename TItem>
  static TItem *
  find_item_under_mouse(const std::list<VisibleItem> & mouseOverItems)
  {
    for (std::list<VisibleItem>::const_iterator
           i = mouseOverItems.begin(); i != mouseOverItems.end(); ++i)
    {
      Item * item = i->item_.lock().get();
      TItem * found = dynamic_cast<TItem *>(item);
      if (found)
      {
        return found;
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // AppView::processMouseEvent
  //
  bool
  AppView::processMouseEvent(Canvas * canvas, QMouseEvent * event)
  {
    bool r = ItemView::processMouseEvent(canvas, event);

    QEvent::Type et = event->type();
    if (et == QEvent::MouseButtonPress)
    {
#if 0
      ProgramItem * prog = find_item_under_mouse<ProgramItem>(mouseOverItems_);
      if (prog)
      {
        cursor->set_program(prog->index());
        requestUncache(cursor);
        requestRepaint();
      }
#endif
    }

    return r;
  }

  //----------------------------------------------------------------
  // AppView::layoutChanged
  //
  void
  AppView::layoutChanged()
  {
#if 0 // ndef NDEBUG
    std::cerr << "AppView::layoutChanged" << std::endl;
#endif

    TMakeCurrentContext currentContext(*context());

    if (pressed_)
    {
      if (dragged_)
      {
        InputArea * ia = dragged_->inputArea();
        if (ia)
        {
          ia->onCancel();
        }

        dragged_ = NULL;
      }

      InputArea * ia = pressed_->inputArea();
      if (ia)
      {
        ia->onCancel();
      }

      pressed_ = NULL;
    }
    inputHandlers_.clear();

    Item & root = *root_;
    root.children_.clear();
    root.addHidden(style_);
    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);
    root.uncache();
    uncache_.clear();

    layout(*this, *style_, *root_);

#if 0 // ndef NDEBUG
    root.dump(std::cerr);
#endif
  }

  //----------------------------------------------------------------
  // AppView::dataChanged
  //
  void
  AppView::dataChanged()
  {
    requestUncache();
    requestRepaint();
  }

  //----------------------------------------------------------------
  // AppView::requestUncacheEPG
  //
  void
  AppView::requestUncacheEPG()
  {
    if (!epg_view_)
    {
      return;
    }

    Item & epg_view = *epg_view_;
    Scrollview & vsv = epg_view.get<Scrollview>("vsv");
    Item & vsv_content = *(vsv.content_);
    Scrollview & hsv = vsv_content.get<Scrollview>("hsv");
    Item & hsv_content = *(hsv.content_);
    requestUncache(&hsv_content);
  }

  //----------------------------------------------------------------
  // AppView::now_playing
  //
  TRecPtr
  AppView::now_playing() const
  {
    if (now_playing_)
    {
      TRecs::const_iterator found =
        recordings_.find(now_playing_->filename_);
      if (found != recordings_.end())
      {
        const TRecPtr & rec_ptr = found->second;
        return rec_ptr;
      }
    }

    return TRecPtr();
  }

  //----------------------------------------------------------------
  // GetRecordings
  //
  struct GetRecordings : yae::Worker::Task
  {
    GetRecordings(AppView & app):
      app_(app)
    {}

    // virtual:
    void execute(const yae::Worker & worker)
    {
      DVR * dvr = app_.model();
      if (dvr)
      {
        TFoundRecordingsPtr found_ptr(new FoundRecordings());
        FoundRecordings & found = *found_ptr;
        dvr->get_existing_recordings(found);
        app_.found_recordings(found_ptr);
      }
    }

    AppView & app_;
  };

  //----------------------------------------------------------------
  // AppView::set_found_recordings
  //
  void
  AppView::found_recordings(const TFoundRecordingsPtr & found)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    found_recordings_ = found;
  }

  //----------------------------------------------------------------
  // AppView::sync_ui
  //
  void
  AppView::sync_ui()
  {
    if (!dvr_)
    {
      return;
    }

    TMakeCurrentContext currentContext(*context());

    // check if model data has changed:
    {
      bool same_program_guide = !dvr_->get_cached_epg(epg_lastmod_, epg_);

      std::map<std::string, Wishlist::Item> wishlist;
      dvr_->get(wishlist);
      bool same_wishlist = (wishlist == wishlist_);

      DVR::Blacklist blacklist;
      dvr_->get(blacklist);
      bool same_blacklist = (blacklist.channels_ == blacklist_.channels_);

      std::map<std::string, TChannels> channels;
      dvr_->get_channels(channels);
      bool same_channels = (channels == channels_);

      std::map<uint32_t, TScheduledRecordings> schedule;
      dvr_->schedule_.get(schedule);
      bool same_schedule = (schedule == schedule_);

      TRecs recordings;
      std::map<std::string, TRecs> playlists;
      std::map<uint32_t, TRecsByTime> rec_by_channel;
      {
        boost::unique_lock<boost::mutex> lock(mutex_);
        if (found_recordings_)
        {
          const FoundRecordings & found = *found_recordings_;
          recordings = found.by_filename_;
          playlists = found.by_playlist_;
          rec_by_channel = found.by_channel_;
        }
      }

      if (worker_.is_idle())
      {
        yae::shared_ptr<GetRecordings, yae::Worker::Task> task;
        task.reset(new GetRecordings(*this));
        worker_.add(task);
      }

      bool same_recordings = (recordings == recordings_);
      if (!same_recordings)
      {
        recordings_.swap(recordings);
        playlists_.swap(playlists);
        rec_by_channel_.swap(rec_by_channel);
        sync_ui_playlists();
      }

      if (!(same_channels && same_blacklist))
      {
        blacklist_.channels_.swap(blacklist.channels_);
        channels_.swap(channels);
        sync_ui_channels();
      }

      if (!same_schedule)
      {
        schedule_.swap(schedule);
        sync_ui_schedule();
      }

      if (!same_wishlist)
      {
        wishlist_.swap(wishlist);
        sync_ui_wishlist();
      }

      // when EPG layout origin jumps -- uncache EPG layout
      // so that all the programs would be redrawn
      // at the correct coordinates:
      int64_t gps_hour = gps_now_round_dn();
      if (gps_hour != gps_hour_)
      {
        gps_hour_ = gps_hour;

        Item & panel = *epg_view_;
        Scrollview & vsv = panel.get<Scrollview>("vsv");
        Item & vsv_content = *(vsv.content_);
        Scrollview & hsv = vsv_content.get<Scrollview>("hsv");
        Item & hsv_content = *(hsv.content_);
        hsv_content.uncache();
      }

      if (same_program_guide &&
          same_blacklist &&
          same_wishlist &&
          same_schedule)
      {
        requestRepaint();
        return;
      }

      // update:
      sync_ui_epg();
    }
  }

  //----------------------------------------------------------------
  // AppView::sync_ui_epg
  //
  void
  AppView::sync_ui_epg()
  {
    // shortcuts:
    AppView & view = *this;
    AppStyle & style = *style_;
    Item & root = *root_;
    Item & hidden = root.get<Item>("hidden");
    Item & panel = *epg_view_;
    Gradient & epg_header = panel.get<Gradient>("epg_header");
    Scrollview & vsv = panel.get<Scrollview>("vsv");
    Item & vsv_content = *(vsv.content_);
    Item & ch_list = vsv_content.get<Item>("ch_list");
    Scrollview & hsv = vsv_content.get<Scrollview>("hsv");
    Item & hsv_content = *(hsv.content_);
    Scrollview & timeline = epg_header.Item::get<Scrollview>("timeline");
    Item & tc = *(timeline.content_);

    // update the layout:
    uint32_t gps_t0 = std::numeric_limits<uint32_t>::max();
    uint32_t gps_t1 = std::numeric_limits<uint32_t>::min();
    std::size_t num_channels = 0;
    ch_index_.clear();

    std::map<uint32_t, yae::shared_ptr<Gradient, Item> > ch_tiles;
    std::map<uint32_t, yae::shared_ptr<Item> > ch_rows;
    std::map<uint32_t, std::map<uint32_t, yae::shared_ptr<Item> > > ch_progs;
    std::map<uint32_t, yae::shared_ptr<Item> > tickmarks;
    std::map<uint32_t, yae::shared_ptr<Rectangle, Item> > rec_highlights;

    for (std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
           i = epg_.channels_.begin(); i != epg_.channels_.end(); ++i)
    {
      const uint32_t ch_num = i->first;
      const yae::mpeg_ts::EPG::Channel & channel = i->second;

      if (yae::has(blacklist_.channels_, ch_num))
      {
        continue;
      }

      // calculate [t0, t1) GPS time bounding box over all channels/programs:
      if (!channel.programs_.empty())
      {
        const yae::mpeg_ts::EPG::Program & p0 = channel.programs_.front();
        const yae::mpeg_ts::EPG::Program & p1 = channel.programs_.back();
        gps_t0 = std::min(gps_t0, p0.gps_time_);
        gps_t1 = std::max(gps_t1, p1.gps_time_ + p1.duration_);
      }

      ch_index_[ch_num] = num_channels;
      num_channels++;

      yae::shared_ptr<Gradient, Item> & tile_ptr = ch_tile_[ch_num];
      if (!tile_ptr)
      {
        std::string ch_str = strfmt("%i-%i", channel.major_, channel.minor_);
        tile_ptr.reset(new Gradient(ch_str.c_str()));

        Gradient & tile = ch_list.add<Gradient>(tile_ptr);
        tile.orientation_ = Gradient::kVertical;
        tile.color_ = style.bg_epg_channel_;
        tile.height_ = ItemRef::reference(hidden, kUnitSize, 1.12);
        tile.anchors_.left_ = ItemRef::reference(ch_list, kPropertyLeft);
        tile.anchors_.right_ = ItemRef::reference(ch_list, kPropertyRight);
        tile.anchors_.top_ = tile.
          addExpr(new ChannelRowTop(view, ch_list, ch_num));

        Text & maj_min = tile.addNew<Text>("maj_min");
        maj_min.font_ = style.font_;
        maj_min.font_.setWeight(57);
        maj_min.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.36);
        maj_min.anchors_.top_ = ItemRef::reference(tile, kPropertyTop);
        maj_min.anchors_.left_ = ItemRef::reference(tile, kPropertyLeft);
        maj_min.margins_.set_top(ItemRef::reference(hidden, kUnitSize, 0.13));
        maj_min.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
        maj_min.text_ = TVarRef::constant(TVar(ch_str.c_str()));
        maj_min.elide_ = Qt::ElideNone;
        maj_min.color_ = maj_min.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_chan_));
        maj_min.background_ = maj_min.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

        Text & ch_name = tile.addNew<Text>("ch_name");
        ch_name.font_ = style.font_;
        ch_name.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.28);
        ch_name.anchors_.top_ = ItemRef::reference(maj_min, kPropertyBottom);
        ch_name.anchors_.left_ = ItemRef::reference(maj_min, kPropertyLeft);
        ch_name.margins_.set_top(ItemRef::reference(hidden, kUnitSize, 0.13));
        ch_name.text_ = TVarRef::constant(TVar(channel.name_.c_str()));
        ch_name.elide_ = Qt::ElideNone;
        ch_name.color_ = ch_name.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_chan_));
        ch_name.background_ = ch_name.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

        WatchLive & live_ia = tile.add<WatchLive>
          (new WatchLive("live_ia", view, ch_num));
        live_ia.anchors_.fill(tile);
      }

      ch_tiles[ch_num] = tile_ptr;

      // shortcut:
      Gradient & tile = *tile_ptr;

      yae::shared_ptr<Item> & row_ptr = ch_row_[ch_num];
      if (!row_ptr)
      {
        row_ptr.reset(new Item(("row" + tile.id_).c_str()));
        Item & row = hsv_content.add<Item>(row_ptr);

        // extend EPG to nearest whole hour, both ends:
        row.anchors_.top_ = ItemRef::reference(tile, kPropertyTop);
        row.anchors_.left_ = row.addExpr(new ProgramRowPos(view, ch_num));
        row.width_ = row.addExpr(new ProgramRowLen(view, ch_num));
        row.height_ = ItemRef::reference(tile, kPropertyHeight);
      }

      ch_rows[ch_num] = row_ptr;

      // shortcut:
      Item & row = *row_ptr;
      std::map<uint32_t, yae::shared_ptr<Item> > & progs_v0 = ch_prog_[ch_num];
      std::map<uint32_t, yae::shared_ptr<Item> > & progs_v1 = ch_progs[ch_num];

      for (std::list<yae::mpeg_ts::EPG::Program>::const_iterator
             j = channel.programs_.begin(); j != channel.programs_.end(); ++j)
      {
        const yae::mpeg_ts::EPG::Program & program = *j;
        yae::shared_ptr<Item> & prog_ptr = progs_v0[program.gps_time_];
        if (!prog_ptr)
        {
          std::string prog_ts = yae::to_yyyymmdd_hhmmss(program.tm_);
          prog_ptr.reset(new Item(prog_ts.c_str()));

          Item & prog = row.add<Item>(prog_ptr);
          prog.anchors_.top_ = ItemRef::reference(tile, kPropertyTop);
          prog.height_ = ItemRef::reference(tile, kPropertyHeight);
          prog.anchors_.left_ = prog.
            addExpr(new ProgramTilePos(view, program.gps_time_));
          prog.width_ = prog.
            addExpr(new ProgramTileWidth(view, program.duration_));

          RoundRect & bg = prog.addNew<RoundRect>("bg");
          bg.anchors_.fill(prog);
          bg.margins_.set(ItemRef::reference(hidden, kUnitSize, 0.03));
          bg.radius_ = ItemRef::reference(hidden, kUnitSize, 0.13);
          bg.background_ = bg.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_));
          bg.color_ = bg.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));

          Item & body = bg.addNew<Item>("body");
          body.anchors_.fill(bg);
          body.margins_.set(ItemRef::reference(hidden, kUnitSize, 0.13));

          Text & hhmm = body.addNew<Text>("hhmm");
          hhmm.font_ = style.font_;
          hhmm.font_.setWeight(62);
          hhmm.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
          hhmm.anchors_.top_ = ItemRef::reference(body, kPropertyTop);
          hhmm.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
          hhmm.elide_ = Qt::ElideNone;
          hhmm.color_ = hhmm.
            addExpr(style_color_ref(view, &AppStyle::fg_epg_));
          hhmm.background_ = hhmm.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

          int hour =
            (program.tm_.tm_hour > 12) ? program.tm_.tm_hour % 12 :
            (program.tm_.tm_hour > 00) ? program.tm_.tm_hour : 12;

          std::string hhmm_txt = strfmt("%i:%02i", hour, program.tm_.tm_min);
          hhmm.text_ = TVarRef::constant(TVar(hhmm_txt.c_str()));

          hhmm.visible_ = hhmm.addExpr
            (new DoesItemFit(ItemRef::reference(body, kPropertyLeft),
                             ItemRef::reference(body, kPropertyRight),
                             hhmm));

          RoundRect & rec = body.addNew<RoundRect>("rec");
          rec.width_ = rec.addExpr(new OddRoundUp(bg, kPropertyHeight, 0.25));
          rec.height_ = ItemRef::reference(rec.width_);
          rec.radius_ = ItemRef::reference(rec.width_, 0.5);
          rec.anchors_.bottom_ = ItemRef::reference(body, kPropertyBottom);
          rec.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
          rec.background_ = rec.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
          rec.color_ = rec.
            addExpr(new RecButtonColor(view, ch_num, program.gps_time_));
          rec.visible_ = rec.addExpr
            (new DoesItemFit(ItemRef::reference(body, kPropertyLeft),
                             ItemRef::reference(body, kPropertyRight),
                             rec));

          Text & title = body.addNew<Text>("title");
          title.font_ = style.font_;
          title.font_.setWeight(62);
          title.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
          title.anchors_.vcenter_ = ItemRef::reference(rec, kPropertyVCenter);
          title.anchors_.left_ = ItemRef::reference(rec, kPropertyRight);
          title.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
          title.margins_.
            set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
          title.elide_ = Qt::ElideRight;
          title.color_ = title.
            addExpr(style_color_ref(view, &AppStyle::fg_epg_));
          title.background_ = title.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
          // FIXME: this should be an expression:
          title.text_ = TVarRef::constant(TVar(program.title_.c_str()));

          ShowProgramDetails & details_ia = body.add<ShowProgramDetails>
            (new ShowProgramDetails("details_ia",
                                    view,
                                    ch_num,
                                    program.gps_time_));
          details_ia.anchors_.fill(prog);
          details_ia.margins_.set(ItemRef::scale(hidden, kUnitSize, 0.03));

          ToggleRecording & toggle = body.add<ToggleRecording>
            (new ToggleRecording("toggle", view, ch_num, program.gps_time_));
          toggle.anchors_.fill(rec);
        }

        progs_v1[program.gps_time_] = prog_ptr;
      }
    }

    // update tickmarks:
    uint32_t i0 = gps_time_round_dn(gps_t0);
    uint32_t i1 = gps_time_round_up(gps_t1);
    for (uint32_t gps_time = i0; gps_time < i1; gps_time += 3600)
    {
      ItemPtr & item_ptr = tickmark_[gps_time];
      if (!item_ptr)
      {
        int64_t ts = unix_epoch_gps_offset + gps_time;
        item_ptr.reset(new Item(unix_epoch_time_to_localtime_str(ts).c_str()));

        Item & item = tc.add(item_ptr);
        item.anchors_.top_ = ItemRef::reference(tc, kPropertyTop);
        item.height_ = ItemRef::reference(tc, kPropertyHeight);
        item.anchors_.left_ = item.
          addExpr(new ProgramTilePos(view, gps_time));
        item.width_ = item.
          addExpr(new ProgramTileWidth(view, 3600));

        Rectangle & tickmark = item.addNew<Rectangle>("tickmark");
        tickmark.anchors_.left_ = ItemRef::reference(item, kPropertyLeft);
        tickmark.anchors_.bottom_ = ItemRef::reference(item, kPropertyBottom);
        tickmark.height_ = ItemRef::reference(item, kPropertyHeight);
        tickmark.width_ = ItemRef::constant(1.0);
        tickmark.color_ = tickmark.
            addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.1));

        Text & label = item.addNew<Text>("time");
        label.font_ = style.font_;
        label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
        label.anchors_.vcenter_ = ItemRef::reference(item, kPropertyVCenter);
        label.anchors_.left_ = ItemRef::reference(tickmark, kPropertyRight);
        label.margins_.set_top(ItemRef::reference(hidden, kUnitSize, 0.03));
        label.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
        label.elide_ = Qt::ElideNone;
        label.color_ = label.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.5));
        label.background_ = label.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

        std::string t_str =
          yae::unix_epoch_time_to_localdate(ts, true, true, false);
        label.text_ = TVarRef::constant(TVar(t_str.c_str()));
      }

      tickmarks[gps_time] = item_ptr;
    }

    // highlight timespan of the recordings on the timeline:
    std::map<uint32_t, uint32_t> rec_times;
    for (std::map<uint32_t, TScheduledRecordings>::const_iterator
           i = schedule_.begin(); i != schedule_.end(); ++i)
    {
      const TScheduledRecordings & recordings = i->second;
      for (TScheduledRecordings::const_iterator
             j = recordings.begin(); j != recordings.end(); ++j)
      {
        const TRecPtr rec_ptr = j->second->get_rec();
        const Recording::Rec & rec = *rec_ptr;
        if (!rec.cancelled_)
        {
          uint32_t gps_t1 = yae::get(rec_times, rec.gps_t0_, rec.gps_t1_);
          rec_times[rec.gps_t0_] = std::max(rec.gps_t1_, gps_t1);
        }
      }
    }

    // combine overlapping timespans:
    while (rec_times.size() > 1)
    {
      std::map<uint32_t, uint32_t>::iterator i = rec_times.begin();
      bool combined = false;

      std::map<uint32_t, uint32_t>::iterator prev = i;
      ++i;

      while (i != rec_times.end())
      {
        uint32_t & prev_t1 = prev->second;
        uint32_t t0 = i->first;
        uint32_t t1 = i->second;

        if (t0 <= prev_t1)
        {
          prev_t1 = std::max(prev_t1, t1);
          rec_times.erase(i);
          combined = true;
          break;
        }

        prev = i;
        ++i;
      }

      if (!combined)
      {
        break;
      }
    }

    for (std::map<uint32_t, uint32_t>::const_iterator
           i = rec_times.begin(); i != rec_times.end(); ++i)
    {
      uint32_t gps_t0 = i->first;

      yae::shared_ptr<Rectangle, Item> & item_ptr = rec_highlight_[gps_t0];
      if (!item_ptr)
      {
        uint32_t gps_t1 = i->second;
        int64_t ts = unix_epoch_gps_offset + gps_t0;
        std::string ts_str = unix_epoch_time_to_localtime_str(ts);
        item_ptr.reset(new Rectangle(("rec " + ts_str).c_str()));

        Rectangle & highlight = tc.add<Rectangle>(item_ptr);
        highlight.anchors_.top_ = ItemRef::reference(tc, kPropertyTop);
        highlight.height_ = ItemRef::reference(tc, kPropertyHeight);
        highlight.anchors_.left_ = highlight.
          addExpr(new ProgramTilePos(view, gps_t0));
        highlight.width_ = highlight.
          addExpr(new ProgramTileWidth(view, gps_t1 - gps_t0));
        highlight.color_ = highlight.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_rec_, 0.1));
      }

      rec_highlights[gps_t0] = item_ptr;
    }

    // old channel tiles also must be removed from ch_list:
    for (std::map<uint32_t, yae::shared_ptr<Gradient, Item> >::const_iterator
           i = ch_tile_.begin(); i != ch_tile_.end(); ++i)
    {
      uint32_t ch_num = i->first;
      if (!yae::has(ch_tiles, ch_num))
      {
        yae::shared_ptr<Gradient, Item> tile_ptr = i->second;
        YAE_ASSERT(ch_list.remove(tile_ptr));
      }
    }

    // old channel rows also must be removed from hsv_content:
    for (std::map<uint32_t, yae::shared_ptr<Item> >::const_iterator
           i = ch_row_.begin(); i != ch_row_.end(); ++i)
    {
      uint32_t ch_num = i->first;
      yae::shared_ptr<Item> row_ptr = i->second;

      if (!yae::has(ch_rows, ch_num))
      {
        YAE_ASSERT(hsv_content.remove(row_ptr));
        continue;
      }

      Item & row = *row_ptr;
      std::map<uint32_t, yae::shared_ptr<Item> > & progs_v0 = ch_prog_[ch_num];
      std::map<uint32_t, yae::shared_ptr<Item> > & progs_v1 = ch_progs[ch_num];

      for (std::map<uint32_t, yae::shared_ptr<Item> >::const_iterator
             j = progs_v0.begin(); j != progs_v0.end(); ++j)
      {
        uint32_t gps_time = j->first;
        if (!yae::has(progs_v1, gps_time))
        {
          yae::shared_ptr<Item> prog = j->second;
          YAE_ASSERT(row.remove(prog));
        }
      }
    }

    // old tickmarks also must be removed from tc:
    for (std::map<uint32_t, yae::shared_ptr<Item> >::const_iterator
           i = tickmark_.begin(); i != tickmark_.end(); ++i)
    {
      uint32_t gps_time = i->first;
      if (!yae::has(tickmarks, gps_time))
      {
        yae::shared_ptr<Item> item_ptr = i->second;
        YAE_ASSERT(tc.remove(item_ptr));
      }
    }

    // old highlights also must be removed from tc:
    for (std::map<uint32_t, yae::shared_ptr<Rectangle, Item> >::const_iterator
           i = rec_highlight_.begin(); i != rec_highlight_.end(); ++i)
    {
      uint32_t gps_time = i->first;
      if (!yae::has(rec_highlights, gps_time))
      {
        yae::shared_ptr<Item> item_ptr = i->second;
        YAE_ASSERT(tc.remove(item_ptr));
      }
    }

    // prune old items:
    ch_tile_.swap(ch_tiles);
    ch_row_.swap(ch_rows);
    ch_prog_.swap(ch_progs);
    tickmark_.swap(tickmarks);
    rec_highlight_.swap(rec_highlights);

    hsv_content.uncache();
    dataChanged();
  }

  //----------------------------------------------------------------
  // AppView::sync_ui_channels
  //
  void
  AppView::sync_ui_channels()
  {
    // shortcuts:
    AppView & view = *this;
    AppStyle & style = *style_;
    Item & root = *root_;
    Item & hidden = root.get<Item>("hidden");
    Item & mainview = *mainview_;

    Item & panel = *(ch_layout_.item_);
    Scrollview & sv = get_scrollview(panel);
    Item & table = *(sv.content_);

    ch_layout_.names_.clear();
    ch_layout_.index_.clear();
    std::map<std::string, yae::shared_ptr<Layout> > groups;

    for (std::map<std::string, TChannels>::const_iterator
           i = channels_.begin(); i != channels_.end(); ++i)
    {
      const std::string & frequency = i->first;
      const TChannels & ch_majors = i->second;

      if (ch_majors.empty())
      {
        continue;
      }

      uint64_t freq_hz = boost::lexical_cast<uint64_t>(frequency);

      ch_layout_.index_[frequency] = ch_layout_.names_.size();
      ch_layout_.names_.push_back(frequency);

      yae::shared_ptr<Layout> & layout_ptr = ch_layout_.items_[frequency];
      if (!layout_ptr)
      {
        layout_ptr.reset(new Layout());
        Layout & layout = *layout_ptr;

        std::string title = strfmt("%" PRIu64 " MHz", freq_hz / 1000000);
        Item & group = add_collapsible_list(view,
                                            style,
                                            table,
                                            frequency.c_str(),
                                            title.c_str());
        group.anchors_.top_ = group.
          addExpr(new LayoutItemTop(ch_layout_, frequency));

        layout.item_ = group.self_.lock();

        Item & body = group.get<Item>("body");

        layout.names_.clear();
        layout.index_.clear();
        std::map<std::string, yae::shared_ptr<Layout> > rows;

        for (TChannels::const_iterator
               k = ch_majors.begin(); k != ch_majors.end(); ++k)
        {
          uint16_t ch_major = k->first;
          const TChannelNames & ch_names = k->second;

          for (TChannelNames::const_iterator
                 j = ch_names.begin(); j != ch_names.end(); ++j)
          {
            uint16_t ch_minor = j->first;
            const std::string & ch_name = j->second;

            std::string ch_num = strfmt("%i-%i", ch_major, ch_minor);
            layout.index_[ch_num] = layout.names_.size();
            layout.names_.push_back(ch_num);

            yae::shared_ptr<Layout> & rowlayout_ptr = layout.items_[ch_num];
            if (!rowlayout_ptr)
            {
              rowlayout_ptr.reset(new Layout());
              Layout & rowlayout = *rowlayout_ptr;

              rowlayout.item_.reset(new Item(ch_num.c_str()));
              Item & row = body.add<Item>(rowlayout.item_);

              row.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
              row.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
              row.anchors_.top_ = row.addExpr(new ListItemTop(view,
                                                              body,
                                                              layout.index_,
                                                              row.id_,
                                                              0.6));
              row.height_ = ItemRef::reference(hidden, kUnitSize, 0.6);
#if 1
              Rectangle & bg = row.addNew<Rectangle>("bg");
              bg.anchors_.fill(row);
              bg.color_ = bg.
                addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
              bg.visible_ = bg.
                addInverse(new IsOddRow(layout.index_, row.id_));
#endif

              CheckboxItem & cbox = row.add(new CheckboxItem("cbox", view));
              cbox.anchors_.left_ = ItemRef::reference(row, kPropertyLeft);
              cbox.anchors_.vcenter_ =
                ItemRef::reference(row, kPropertyVCenter);
              cbox.margins_.set_left(ItemRef::reference(row.height_, 0.33));
              cbox.height_ = ItemRef::reference(row.height_, 0.75);
              cbox.width_ = cbox.height_;
              cbox.checked_ = cbox.
                addInverse(new IsBlacklisted(view, ch_major, ch_minor));
              cbox.on_toggle_.
                reset(new OnToggleBlacklist(view, ch_major, ch_minor));

              Item & maj_min = row.addNew<Item>("maj_min");
              maj_min.anchors_.fill(row);
              maj_min.anchors_.left_ = ItemRef::reference(cbox, kPropertyRight);
              maj_min.anchors_.right_.reset();
              maj_min.width_ = ItemRef::reference(hidden, kUnitSize, 2.0);
              maj_min.margins_.set_left(ItemRef::reference(row.height_, 0.33));

              Text & ch_text = row.addNew<Text>("ch_text");
              ch_text.anchors_.vcenter(maj_min);
              ch_text.font_ = style.font_;
              ch_text.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
              ch_text.elide_ = Qt::ElideRight;
              ch_text.color_ = ch_text.
                addExpr(style_color_ref(view, &AppStyle::fg_epg_, 1.0));
              ch_text.background_ = ch_text.
                addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));
              ch_text.text_ = TVarRef::constant(TVar(ch_num));

              Text & name = row.addNew<Text>("ch_name");
              name.anchors_.vcenter(row);
              name.anchors_.left_ = ItemRef::reference(maj_min, kPropertyRight);
              name.font_ = style.font_;
              name.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
              name.elide_ = Qt::ElideRight;
              name.color_ = name.
                addExpr(style_color_ref(view, &AppStyle::fg_epg_, 1.0));
              name.background_ = name.
                addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));
              name.text_ = TVarRef::constant(TVar(ch_name));
            }

            rows[ch_num] = rowlayout_ptr;
          }
        }

        // unreferenced rows must be removed:
        for (std::map<std::string, yae::shared_ptr<Layout> >::const_iterator
               j = layout.items_.begin(); j != layout.items_.end(); ++j)
        {
          const std::string & name = j->first;
          if (!yae::has(rows, name))
          {
            yae::shared_ptr<Item> row_ptr = j->second->item_;
            YAE_ASSERT(body.remove(row_ptr));
          }
        }

        layout.items_.swap(rows);
        layout.item_->uncache();
      }

      groups[frequency] = layout_ptr;
    }

    // unreferenced groups must be removed:
    for (std::map<std::string, yae::shared_ptr<Layout> >::const_iterator
           i = ch_layout_.items_.begin(); i != ch_layout_.items_.end(); ++i)
    {
      const std::string & name = i->first;
      if (!yae::has(groups, name))
      {
        yae::shared_ptr<Item> group_ptr = i->second->item_;
        YAE_ASSERT(table.remove(group_ptr));
      }
    }

    ch_layout_.items_.swap(groups);
    ch_layout_.item_->uncache();
  }

  //----------------------------------------------------------------
  // AppView::sync_ui_schedule
  //
  void
  AppView::sync_ui_schedule()
  {
    // shortcuts:
    AppView & view = *this;
    AppStyle & style = *style_;
    Item & root = *root_;
    Item & hidden = root.get<Item>("hidden");
    Item & mainview = *mainview_;

    Item & panel = *(sch_layout_.item_);
    Item & header = panel.get<Item>("header");
    Item & container = panel.get<Item>("container");
    Scrollview & sv = get_scrollview(container);
    Item & table = *(sv.content_);
    Item & h1 = header.get<Item>("h1");
    Item & h2 = header.get<Item>("h2");
    Item & h3 = header.get<Item>("h3");

    TRecordings schedule;
    for (std::map<uint32_t, TScheduledRecordings>::const_iterator
           i = schedule_.begin(); i != schedule_.end(); ++i)
    {
      const TScheduledRecordings & ch_recs = i->second;
      for (TScheduledRecordings::const_iterator
             j = ch_recs.begin(); j != ch_recs.end(); ++j)
      {
        const TRecordingPtr & rec_ptr = j->second;
        const Recording & rec = *rec_ptr;
        std::string basename = rec.get_basename();
        schedule[basename] = rec_ptr;
      }
    }

    sch_layout_.names_.clear();
    sch_layout_.index_.clear();
    std::map<std::string, yae::shared_ptr<Layout> > rows;

    for (TRecordings::const_iterator
           i = schedule.begin(); i != schedule.end(); ++i)
    {
      const std::string & rec_id = i->first;
      const TRecPtr rec_ptr = i->second->get_rec();
      const Recording::Rec & rec = *rec_ptr;
      const uint32_t ch_num = yae::mpeg_ts::channel_number(rec.channel_major_,
                                                           rec.channel_minor_);

      sch_layout_.index_[rec_id] = sch_layout_.names_.size();
      sch_layout_.names_.push_back(rec_id);

      yae::shared_ptr<Layout> & layout_ptr = sch_layout_.items_[rec_id];
      if (!layout_ptr)
      {
        layout_ptr.reset(new Layout());
        Layout & layout = *layout_ptr;

        layout.item_.reset(new Item(rec_id.c_str()));
        Item & row = table.add<Item>(layout.item_);

        row.anchors_.left_ = ItemRef::reference(table, kPropertyLeft);
        row.anchors_.right_ = ItemRef::reference(table, kPropertyRight);
        row.anchors_.top_ = row.addExpr(new ListItemTop(view,
                                                        table,
                                                        sch_layout_.index_,
                                                        row.id_,
                                                        1.78));
        row.height_ = ItemRef::reference(hidden, kUnitSize, 1.78);

        Rectangle & bg = row.addNew<Rectangle>("bg");
        bg.anchors_.fill(row);
        bg.color_ = bg.addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
        bg.visible_ = bg.addExpr(new IsOddRow(sch_layout_.index_, row.id_));

        Item & c1 = row.addNew<Item>("c1");
        Item & c2 = row.addNew<Item>("c2");
        Item & c3 = row.addNew<Item>("c3");

        c1.anchors_.top_ = ItemRef::reference(row, kPropertyTop);
        c1.anchors_.left_ = ItemRef::reference(row, kPropertyLeft);
        c1.anchors_.bottom_ = ItemRef::reference(row, kPropertyBottom);
        c1.width_ = ItemRef::reference(h1, kPropertyWidth);

        c2.anchors_.top_ = ItemRef::reference(row, kPropertyTop);
        c2.anchors_.left_ = ItemRef::reference(c1, kPropertyRight);
        c2.anchors_.bottom_ = ItemRef::reference(row, kPropertyBottom);
        c2.width_ = ItemRef::reference(h2, kPropertyWidth);

        c3.anchors_.top_ = ItemRef::reference(row, kPropertyTop);
        c3.anchors_.left_ = ItemRef::reference(c2, kPropertyRight);
        c3.anchors_.bottom_ = ItemRef::reference(row, kPropertyBottom);
        c3.width_ = ItemRef::reference(h3, kPropertyWidth);

        // checkbox, channel, title:
        Item & row1 = row.addNew<Item>("row1");
        row1.anchors_.fill(row);
        row1.anchors_.bottom_.reset();
        row1.height_ = ItemRef::reference(hidden, kUnitSize, 0.6);
        row1.margins_.set(ItemRef::reference(hidden, kUnitSize, 0.13));

        CheckboxItem & cbox = row1.add(new CheckboxItem("cbox", view));
        cbox.anchors_.vcenter(row1);
        cbox.anchors_.right_.reset();
        cbox.margins_.set_left(ItemRef::reference(row1.height_, 0.33));
        cbox.height_ = ItemRef::reference(row1.height_, 0.75);
        cbox.width_ = cbox.height_;
        cbox.checked_ = cbox.
          addInverse(new IsCancelled(view, ch_num, rec.gps_t0_));
        cbox.on_toggle_.
          reset(new OnToggleSchedule(view, ch_num, rec.gps_t0_));
        Item & maj_min = row1.addNew<Item>("maj_min");
        maj_min.anchors_.fill(row1);
        maj_min.anchors_.left_ = ItemRef::reference(cbox, kPropertyRight);
        maj_min.anchors_.right_.reset();
        maj_min.width_ = ItemRef::reference(hidden, kUnitSize, 2.0);
        maj_min.margins_.set_left(ItemRef::reference(row1.height_, 0.33));

        Text & ch_text = row1.addNew<Text>("ch_text");
        ch_text.anchors_.vcenter(maj_min);
        ch_text.font_ = style.font_;
        ch_text.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
        ch_text.elide_ = Qt::ElideRight;
        ch_text.color_ = ch_text.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_, 1.0));
        ch_text.background_ = ch_text.
          addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));
        std::string ch_txt = strfmt("%i-%i",
                                    rec.channel_major_,
                                    rec.channel_minor_);
        ch_text.text_ = TVarRef::constant(TVar(ch_txt));

        Text & title = row1.addNew<Text>("title");
        title.anchors_.vcenter(row1);
        title.anchors_.left_ = ItemRef::reference(maj_min, kPropertyRight);
        title.anchors_.right_ = ItemRef::reference(c1, kPropertyRight);
        title.font_ = style.font_;
        title.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
        title.elide_ = Qt::ElideRight;
        title.color_ = title.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_, 1.0));
        title.background_ = title.
          addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));
        title.text_ = TVarRef::constant(TVar(rec.title_));

        // duration:
        Text & length = row1.addNew<Text>("length");
        length.anchors_.vcenter(row1);
        length.anchors_.left_ = ItemRef::reference(c2, kPropertyLeft);
        length.anchors_.right_ = ItemRef::reference(c2, kPropertyRight);
        length.margins_.set(ItemRef::reference(hidden, kUnitSize, 0.13));
        length.font_ = style.font_;
        length.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
        length.font_.setWeight(62);
        length.elide_ = Qt::ElideRight;
        length.color_ = length.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_));
        length.background_ = length.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
        TTime dt(rec.gps_t1_ - rec.gps_t0_, 1);
        length.text_ = TVarRef::constant(TVar(dt.to_hhmmss()));

        // when recorded:
        Text & date = row1.addNew<Text>("date");
        date.anchors_.vcenter(row1);
        date.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
        date.anchors_.right_ = ItemRef::reference(c3, kPropertyRight);
        date.margins_.set(ItemRef::reference(hidden, kUnitSize, 0.13));
        date.font_ = style.font_;
        date.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
        date.font_.setWeight(62);
        date.elide_ = Qt::ElideRight;
        date.color_ = date.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_));
        date.background_ = date.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
        std::string ts = unix_epoch_time_to_localtime_str(rec.utc_t0_);
        date.text_ = TVarRef::constant(TVar(ts));

        // description:
        Item & row2 = row.addNew<Item>("row2");
        row2.anchors_.fill(row);
        row2.anchors_.top_ = ItemRef::reference(row1, kPropertyBottom);
        row2.anchors_.left_ = ItemRef::reference(title, kPropertyLeft);
        row2.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.6));

        Text & desc = row2.addNew<Text>("desc");
        desc.anchors_.fill(row2);
        desc.font_ = style.font_;
        desc.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
        desc.elide_ = Qt::ElideNone;
        desc.color_ = desc.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_, 1.0));
        desc.background_ = desc.
          addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));
        desc.text_ = TVarRef::constant(TVar(rec.description_));
      }

      rows[rec_id] = layout_ptr;
    }

    // unreferenced rows must be removed:
    for (std::map<std::string, yae::shared_ptr<Layout> >::const_iterator
           i = sch_layout_.items_.begin(); i != sch_layout_.items_.end(); ++i)
    {
      const std::string & name = i->first;
      if (!yae::has(rows, name))
      {
        yae::shared_ptr<Item> row_ptr = i->second->item_;
        YAE_ASSERT(table.remove(row_ptr));
      }
    }

    sch_layout_.items_.swap(rows);
    sch_layout_.item_->uncache();
  }

  //----------------------------------------------------------------
  // AppView::sync_ui_wishlist
  //
  void
  AppView::sync_ui_wishlist()
  {
    // shortcuts:
    AppView & view = *this;
    AppStyle & style = *style_;
    Item & root = *root_;
    Item & hidden = root.get<Item>("hidden");

    Item & panel = *sideview_;
    Scrollview & sv = panel.get<Scrollview>("sideview.scrollview");
    Item & sidebar = *(sv.content_);
    Item & group = sidebar.get<Item>("wishlist_group");
    Item & body = group.get<Item>("body");

    std::map<std::string, yae::shared_ptr<Item> > rows;
    wl_index_.clear();
    std::size_t num_rows = 0;

    for (std::map<std::string, Wishlist::Item>::const_iterator
           i = wishlist_.begin(); i != wishlist_.end(); ++i)
    {
      const std::string & wi_key = i->first;
      const Wishlist::Item & wi = i->second;

      std::string row_id = "wl: " + wi_key;
      wl_index_[row_id] = num_rows;
      num_rows++;

      yae::shared_ptr<Item> & row_ptr = wl_sidebar_[row_id];
      if (!row_ptr)
      {
        row_ptr.reset(new EditWishlistItem(row_id.c_str(),
                                           view,
                                           view.sidebar_sel_));

        Item & row = body.add<Item>(row_ptr);
        row.height_ = ItemRef::reference(hidden, kUnitSize, 0.6);
        row.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
        row.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
        row.anchors_.top_ = row.
          addExpr(new ListItemTop(view, body, wl_index_, row.id_));

        Rectangle & bg = row.addNew<Rectangle>("bg");
        bg.anchors_.fill(row);
        bg.anchors_.left_ = ItemRef::reference(sidebar, kPropertyLeft);
        bg.anchors_.right_ = ItemRef::reference(sidebar, kPropertyRight);
        bg.color_ = bg.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_scrollbar_, 0.33));
        bg.visible_ = bg.addExpr(new IsSelected(view.sidebar_sel_, row.id_));

        RoundRect & icon = row.addNew<RoundRect>("icon");
        icon.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
        icon.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
        icon.width_ = ItemRef::reference(hidden, kUnitSize, 0.9);
        icon.height_ = ItemRef::reference(hidden, kUnitSize, 0.5);
        icon.radius_ = ItemRef::reference(hidden, kUnitSize, 0.13);
        icon.background_ = icon.
           addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));
        icon.color_ = icon.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_scrollbar_, 1.0));

        double fade = wi.is_disabled() ? 0.5 : 1.0;
        Text & chan = row.addNew<Text>("chan");
        chan.font_ = style.font_;
        chan.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
        chan.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
        chan.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
        chan.width_ = ItemRef::reference(hidden, kUnitSize, 0.9);
        chan.height_ = ItemRef::reference(hidden, kUnitSize, 0.5);
        chan.alignment_ = Qt::AlignHCenter;
        chan.elide_ = Qt::ElideRight;
        chan.text_ = TVarRef::constant(TVar(wi.ch_txt()));
        chan.color_ = chan.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
        chan.background_ = chan.
          addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));

        Text & desc = row.addNew<Text>("desc");
        desc.font_ = style.font_;
        desc.font_.setWeight(62);
        desc.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
        desc.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
        desc.anchors_.left_ = ItemRef::reference(icon, kPropertyRight);
        desc.anchors_.right_ = ItemRef::reference(row, kPropertyRight);
        desc.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
        desc.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.13));
        desc.elide_ = Qt::ElideRight;
        desc.text_ = TVarRef::constant(TVar(wi.to_txt()));
        desc.color_ = desc.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_, 1.0 * fade));
        desc.background_ = desc.
          addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));
      }

      rows[row_id] = row_ptr;
    }

    // unreferenced wishlist items must be removed:
    std::string sidebar_sel = view.sidebar_sel_;
    std::string prev_wishlist;

    for (std::map<std::string, yae::shared_ptr<Item> >::const_iterator
           i = wl_sidebar_.begin(); i != wl_sidebar_.end(); ++i)
    {
      const std::string & row_id = i->first;
      if (!yae::has(rows, row_id))
      {
        yae::shared_ptr<Item> row_ptr = i->second;
        YAE_ASSERT(body.remove(row_ptr));

        if (view.sidebar_sel_ == row_id)
        {
          sidebar_sel = prev_wishlist;
        }
      }
      else
      {
        prev_wishlist = row_id;
      }
    }

    if (sidebar_sel.empty())
    {
      sidebar_sel = "view_mode_program_guide";
    }

    view.sidebar_sel_ = sidebar_sel;
    wl_sidebar_.swap(rows);
    dataChanged();
  }

  //----------------------------------------------------------------
  // AppView::sync_ui_playlists
  //
  void
  AppView::sync_ui_playlists()
  {
    // shortcuts:
    AppView & view = *this;
    AppStyle & style = *style_;
    Item & root = *root_;
    Item & hidden = root.get<Item>("hidden");
    Item & panel = *sideview_;
    Scrollview & sv = panel.get<Scrollview>("sideview.scrollview");
    Item & sidebar = *(sv.content_);
    Item & group = sidebar.get<Item>("playlist_group");
    Item & body = group.get<Item>("body");

    std::map<std::string, yae::shared_ptr<Item> > rows;
    std::size_t num_playlists = 0;
    pl_index_.clear();

    for (std::map<std::string, TRecs>::const_iterator
           i = playlists_.begin(); i != playlists_.end(); ++i)
    {
      const std::string & name = i->first;
      const TRecs & recs = i->second;
      if (recs.empty())
      {
        continue;
      }

      const Recording::Rec & rec = *(recs.begin()->second);
      pl_index_[name] = num_playlists;
      num_playlists++;

      yae::shared_ptr<Item> & row_ptr = pl_sidebar_[name];
      if (!row_ptr)
      {
        row_ptr.reset(new Select(name.c_str(), view, view.sidebar_sel_));

        Item & row = body.add<Item>(row_ptr);
        row.height_ = ItemRef::reference(hidden, kUnitSize, 0.6);
        row.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
        row.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
        row.anchors_.top_ = row.
          addExpr(new ListItemTop(view, body, pl_index_, row.id_));

        Rectangle & bg = row.addNew<Rectangle>("bg");
        bg.anchors_.fill(row);
        bg.anchors_.left_ = ItemRef::reference(sidebar, kPropertyLeft);
        bg.anchors_.right_ = ItemRef::reference(sidebar, kPropertyRight);
        bg.color_ = bg.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_scrollbar_, 0.33));
        bg.visible_ = bg.addExpr(new IsSelected(view.sidebar_sel_, row.id_));

        RoundRect & chbg = row.addNew<RoundRect>("chbg");
        chbg.radius_ = ItemRef::reference(hidden, kUnitSize, 0.13);
        chbg.background_ = chbg.
          addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));
        chbg.color_ = chbg.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_scrollbar_, 1.0));

        Text & chan = row.addNew<Text>("chan");
        chan.font_ = style.font_;
        chan.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
        chan.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
        chan.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
        // chan.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.3));
        chan.width_ = ItemRef::reference(hidden, kUnitSize, 0.9);
        chan.height_ = ItemRef::reference(hidden, kUnitSize, 0.5);
        // chan.height_ = ItemRef::reference(chan, kPropertyFontHeight);
        chan.alignment_ = Qt::AlignHCenter;
        chan.elide_ = Qt::ElideRight;
        chan.color_ = chan.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
        chan.background_ = chan.
          addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));

        std::string chan_txt = strfmt("%i-%i",
                                      rec.channel_major_,
                                      rec.channel_minor_);
        chan.text_ = TVarRef::constant
          (TVar(QString::fromUtf8(chan_txt.c_str())));

        chbg.anchors_.fill(chan);

        Text & title = row.addNew<Text>("title");
        RoundRect & count_bg = row.addNew<RoundRect>("count_bg");
        Text & count = row.addNew<Text>("count");

        title.font_ = style.font_;
        title.font_.setWeight(62);
        title.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
        title.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
        title.anchors_.left_ = ItemRef::reference(chan, kPropertyRight);
        title.anchors_.right_ = ItemRef::reference(count_bg, kPropertyLeft);
        title.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
        title.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.13));
        title.elide_ = Qt::ElideRight;
        title.color_ = title.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_, 1.0));
        title.background_ = title.
          addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));
        title.text_ = TVarRef::constant
          (TVar(QString::fromUtf8(rec.title_.c_str())));

        count.font_ = style.font_;
        count.font_.setWeight(62);
        count.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
        count.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
        count.anchors_.right_ = ItemRef::reference(row, kPropertyRight);
        count.margins_.set(ItemRef::reference(hidden, kUnitSize, 0.13));
        count.elide_ = Qt::ElideRight;
        count.color_ = count.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_scrollbar_, 1.0));
        count.background_ = count.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_scrollbar_, 0.0));
        count.text_ = count.addExpr(new GetPlaylistSize(view, name));
        count.text_.disableCaching();

        count_bg.anchors_.center(count, -1.0, -1.0);
        count_bg.width_ = count_bg.
          addExpr(new OddRoundUp(count, kPropertyWidth, 1.0, 7.0));
        count_bg.height_ = count_bg.
          addExpr(new OddRoundUp(count, kPropertyHeight));
        count_bg.radius_ = ItemRef::scale(count, kPropertyHeight, 0.5);
        count_bg.color_ = count_bg.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_scrollbar_, 1.0));
        count_bg.background_ = count_bg.
          addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));
      }

      rows[name] = row_ptr;

      sync_ui_playlist(name, recs);
    }

    sync_ui_playlist(std::string("view_mode_recordings"), recordings_);

    // unreferenced playlist items must be removed:
    std::string sidebar_sel = view.sidebar_sel_;
    std::string prev_playlist;

    for (std::map<std::string, yae::shared_ptr<Item> >::const_iterator
           i = pl_sidebar_.begin(); i != pl_sidebar_.end(); ++i)
    {
      const std::string & name = i->first;
      if (!yae::has(rows, name))
      {
        yae::shared_ptr<Item> row_ptr = i->second;
        YAE_ASSERT(body.remove(row_ptr));

        yae::shared_ptr<Layout> layout_ptr = pl_layout_[name];
        pl_layout_.erase(name);

        // must remove the playlist layout item from mainview to avoid
        // a dangling reference to Layout::index_ in ListItemTop:
        YAE_ASSERT(mainview_->remove(layout_ptr->item_));

        if (view.sidebar_sel_ == name)
        {
          sidebar_sel = prev_playlist;
        }
      }
      else
      {
        prev_playlist = name;
      }
    }

    if (sidebar_sel.empty())
    {
      sidebar_sel = "view_mode_recordings";
    }

    view.sidebar_sel_ = sidebar_sel;
    pl_sidebar_.swap(rows);
    dataChanged();
  }

  //----------------------------------------------------------------
  // AppView::sync_ui_playlist
  //
  void
  AppView::sync_ui_playlist(const std::string & playlist_name,
                            const TRecs & playlist_recs)
  {
    // shortcuts:
    AppView & view = *this;
    AppStyle & style = *style_;
    Item & root = *root_;
    Item & hidden = root.get<Item>("hidden");
    Item & mainview = *mainview_;

    yae::shared_ptr<Layout> & layout_ptr = pl_layout_[playlist_name];
    if (!layout_ptr)
    {
      layout_ptr.reset(new Layout());
      Layout & layout = *layout_ptr;

      layout.item_.reset(new Item(playlist_name.c_str()));
      Item & panel = mainview.add<Item>(layout.item_);
      panel.anchors_.fill(mainview);
      panel.visible_ = panel.
        addExpr(new IsSelected(sidebar_sel_, playlist_name));

      Item & header = panel.addNew<Item>("header");
      header.anchors_.fill(panel);
      header.anchors_.bottom_.reset();
      header.height_ = ItemRef::reference(hidden, kUnitSize, 0.4);

      Rectangle & bg = header.addNew<Rectangle>("bg");
      bg.anchors_.fill(header);
      bg.color_ = bg.addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));

      // add table columns:
      Item & h1 = header.addNew<Item>("h1");
      Rectangle & l1 = header.addNew<Rectangle>("l1");
      Item & h2 = header.addNew<Item>("h2");
      Rectangle & l2 = header.addNew<Rectangle>("l2");
      Item & h3 = header.addNew<Item>("h3");
      Rectangle & l3 = header.addNew<Rectangle>("l3");
      Item & h4 = header.addNew<Item>("h4");

      h1.anchors_.top_ = ItemRef::reference(header, kPropertyTop);
      h1.anchors_.bottom_ = ItemRef::reference(header, kPropertyBottom);
      h1.anchors_.left_ = ItemRef::reference(header, kPropertyLeft);
      h1.anchors_.right_ = ItemRef::reference(l1, kPropertyLeft);

      Text & t1 = h1.addNew<Text>("t1");
      t1.anchors_.vcenter(h1);
      t1.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
      t1.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.13));
      t1.font_ = style.font_;
      t1.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      t1.elide_ = Qt::ElideRight;
      t1.color_ = t1.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
      t1.background_ = t1.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
      t1.text_ = TVarRef::constant(TVar("Title and Description"));

      l1.anchors_.top_ = ItemRef::offset(header, kPropertyTop, 1);
      l1.anchors_.bottom_ = ItemRef::offset(header, kPropertyBottom, -2);
      l1.anchors_.right_ = ItemRef::reference(h2, kPropertyLeft);
      l1.width_ = ItemRef::constant(1);
      l1.color_ = l1.addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.3));

      h2.anchors_.top_ = ItemRef::reference(header, kPropertyTop);
      h2.anchors_.bottom_ = ItemRef::reference(header, kPropertyBottom);
      h2.anchors_.right_ = ItemRef::reference(l2, kPropertyLeft);
      h2.width_ = ItemRef::reference(hidden, kUnitSize, 2.0);

      Text & t2 = h2.addNew<Text>("t2");
      t2.anchors_.vcenter(h2);
      t2.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
      t2.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.13));
      t2.font_ = style.font_;
      t2.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      t2.elide_ = Qt::ElideRight;
      t2.color_ = t2.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
      t2.background_ = t2.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
      t2.text_ = TVarRef::constant(TVar("Duration"));

      l2.anchors_.top_ = ItemRef::offset(header, kPropertyTop, 1);
      l2.anchors_.bottom_ = ItemRef::offset(header, kPropertyBottom, -2);
      l2.anchors_.right_ = ItemRef::reference(h3, kPropertyLeft);
      l2.width_ = ItemRef::constant(1);
      l2.color_ = l2.addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.3));

      h3.anchors_.top_ = ItemRef::reference(header, kPropertyTop);
      h3.anchors_.bottom_ = ItemRef::reference(header, kPropertyBottom);
      h3.anchors_.right_ = ItemRef::reference(l3, kPropertyLeft);
      h3.width_ = ItemRef::reference(hidden, kUnitSize, 3.5);

      Text & t3 = h3.addNew<Text>("t3");
      t3.anchors_.vcenter(h3);
      t3.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
      t3.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.13));
      t3.font_ = style.font_;
      t3.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      t3.elide_ = Qt::ElideRight;
      t3.color_ = t3.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
      t3.background_ = t3.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
      t3.text_ = TVarRef::constant(TVar("Date"));

      l3.anchors_.top_ = ItemRef::offset(header, kPropertyTop, 1);
      l3.anchors_.bottom_ = ItemRef::offset(header, kPropertyBottom, -2);
      l3.anchors_.right_ = ItemRef::reference(h4, kPropertyLeft);
      l3.width_ = ItemRef::constant(1);
      l3.color_ = l3.addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.3));

      h4.anchors_.top_ = ItemRef::reference(header, kPropertyTop);
      h4.anchors_.bottom_ = ItemRef::reference(header, kPropertyBottom);
      h4.anchors_.right_ = ItemRef::reference(header, kPropertyRight);
      h4.width_ = ItemRef::reference(hidden, kUnitSize, 3.0);

      Text & t4 = h4.addNew<Text>("t4");
      t4.anchors_.vcenter(h4);
      t4.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
      t4.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.13));
      t4.font_ = style.font_;
      t4.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      t4.elide_ = Qt::ElideRight;
      t4.color_ = t4.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
      t4.background_ = t4.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
      t4.text_ = TVarRef::constant(TVar("Size"));

      // layout the table body:
      Item & body = panel.addNew<Item>("body");
      body.anchors_.fill(panel);
      body.anchors_.top_ = ItemRef::reference(header, kPropertyBottom);

      layout_scrollview(kScrollbarVertical, view, style, body,
                        ItemRef::reference(hidden, kUnitSize, 0.33));
    }

    // shortcuts:
    Layout & layout = *layout_ptr;
    Item & panel = *(layout.item_);
    Item & header = panel.get<Item>("header");
    Item & body = panel.get<Item>("body");
    Scrollview & sv = get_scrollview(body);
    Item & table = *(sv.content_);
    Item & h1 = header.get<Item>("h1");
    Item & h2 = header.get<Item>("h2");
    Item & h3 = header.get<Item>("h3");
    Item & h4 = header.get<Item>("h4");

    layout.names_.clear();
    layout.index_.clear();
    std::map<std::string, yae::shared_ptr<Layout> > rows;

    for (TRecs::const_iterator
           i = playlist_recs.begin(); i != playlist_recs.end(); ++i)
    {
      const std::string & name = i->first;
      const TRecPtr & rec_ptr = i->second;
      const Recording::Rec & rec = *rec_ptr;
      std::string basepath = rec.get_filepath(dvr_->basedir_, "");

      layout.index_[name] = layout.names_.size();
      layout.names_.push_back(name);

      yae::shared_ptr<Layout> & rowlayout_ptr = layout.items_[name];
      if (!rowlayout_ptr)
      {
        rowlayout_ptr.reset(new Layout());
        Layout & rowlayout = *rowlayout_ptr;

        rowlayout.item_.reset(new Item(name.c_str()));
        Item & row = table.add<Item>(rowlayout.item_);

        row.anchors_.left_ = ItemRef::reference(table, kPropertyLeft);
        row.anchors_.right_ = ItemRef::reference(table, kPropertyRight);
        row.anchors_.top_ = row.addExpr(new ListItemTop(view,
                                                        table,
                                                        layout.index_,
                                                        row.id_,
                                                        2.56));
        row.height_ = ItemRef::reference(hidden, kUnitSize, 2.56);

        PlaybackRecording & playback_ia =
          row.add(new PlaybackRecording("playback_ia", view, name));
        playback_ia.anchors_.fill(row);

        Rectangle & bg = row.addNew<Rectangle>("bg");
        bg.anchors_.fill(row);
        bg.color_ = bg.addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
        bg.visible_ = bg.addExpr(new IsOddRow(layout.index_, row.id_));

        // thumbnail, title, description:
        Item & inner = row.addNew<Item>("inner");
        inner.anchors_.fill(row);
        inner.margins_.set(ItemRef::reference(hidden, kUnitSize, 0.26));

        Item & row1 = row.addNew<Item>("row1");
        row1.anchors_.fill(row);
        row1.anchors_.bottom_.reset();
        row1.height_ = ItemRef::reference(hidden, kUnitSize, 0.6);
        row1.margins_.set_top(ItemRef::reference(hidden, kUnitSize, 0.13));

        Item & c1 = row.addNew<Item>("c1");
        Item & c2 = row.addNew<Item>("c2");
        Item & c3 = row.addNew<Item>("c3");
        Item & c4 = row.addNew<Item>("c4");

        c1.anchors_.top_ = ItemRef::reference(row1, kPropertyTop);
        c1.anchors_.left_ = ItemRef::reference(row1, kPropertyLeft);
        c1.anchors_.bottom_ = ItemRef::reference(row1, kPropertyBottom);
        c1.width_ = ItemRef::reference(h1, kPropertyWidth);

        c2.anchors_.top_ = ItemRef::reference(row1, kPropertyTop);
        c2.anchors_.left_ = ItemRef::reference(c1, kPropertyRight);
        c2.anchors_.bottom_ = ItemRef::reference(row1, kPropertyBottom);
        c2.width_ = ItemRef::reference(h2, kPropertyWidth);

        c3.anchors_.top_ = ItemRef::reference(row1, kPropertyTop);
        c3.anchors_.left_ = ItemRef::reference(c2, kPropertyRight);
        c3.anchors_.bottom_ = ItemRef::reference(row1, kPropertyBottom);
        c3.width_ = ItemRef::reference(h3, kPropertyWidth);

        c4.anchors_.top_ = ItemRef::reference(row1, kPropertyTop);
        c4.anchors_.left_ = ItemRef::reference(c3, kPropertyRight);
        c4.anchors_.bottom_ = ItemRef::reference(row1, kPropertyBottom);
        c4.width_ = ItemRef::reference(h4, kPropertyWidth);

        Image & thumbnail = inner.addNew<Image>("thumbnail");
        thumbnail.setContext(view);
        thumbnail.anchors_.fill(inner);
        thumbnail.anchors_.right_.reset();
        thumbnail.width_ = ItemRef::reference(inner, kPropertyHeight,
                                              16.0 / 9.0);

        std::string path = rec.get_filepath(dvr_->basedir_, ".mpg");
        std::string url = "image://thumbnails/" + path;
        thumbnail.url_ = TVarRef::constant(QString::fromUtf8(url.c_str()));

        Rectangle & badge_bg = inner.addNew<Rectangle>("badge_bg");
        Text & badge = inner.addNew<Text>("badge");
        badge.anchors_.bottom_ = ItemRef::offset(inner, kPropertyBottom);
        badge.anchors_.left_ = ItemRef::offset(inner, kPropertyLeft);
        badge.margins_.set_top(ItemRef::reference(hidden, kUnitSize, 0.1));
        badge.margins_.set_bottom(ItemRef::reference(hidden, kUnitSize, -0.1));
        badge.font_ = style.font_large_;
        badge.font_.setWeight(62);
        badge.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
        badge.elide_ = Qt::ElideRight;
        badge.color_ = badge.
          addExpr(style_color_ref(view, &AppStyle::cursor_fg_));
        badge.background_ = badge.
          addExpr(style_color_ref(view, &AppStyle::cursor_, 0.0));
        badge.text_ = badge.addExpr(new GetBadgeText(view, basepath));
        badge.visible_ = badge.addExpr(new ShowBadge(view, basepath));

        badge_bg.anchors_.fill(badge);
        badge_bg.margins_.
          set_top(ItemRef::reference(badge, kPropertyFontDescent, -1.0));
        badge_bg.margins_.
          set_left(ItemRef::reference(hidden, kUnitSize, -0.1));
        badge_bg.margins_.
          set_right(ItemRef::reference(hidden, kUnitSize, -0.1));
        badge_bg.visible_ = BoolRef::reference(badge, kPropertyVisible);
        badge_bg.color_ = badge_bg.
          addExpr(style_color_ref(view, &AppStyle::cursor_));

        Text & title = c1.addNew<Text>("title");
        title.anchors_.vcenter(c1);
        title.anchors_.left_ = ItemRef::reference(thumbnail, kPropertyRight);
        title.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.26));
        title.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.26));
        title.font_ = style.font_;
        title.font_.setWeight(62);
        title.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
        title.elide_ = Qt::ElideRight;
        title.color_ = title.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_));
        title.background_ = title.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
        title.text_ =
          TVarRef::constant(TVar(QString::fromUtf8(rec.title_.c_str())));

        Text & desc = c1.addNew<Text>("desc");
        desc.anchors_.top_ = ItemRef::reference(title, kPropertyBottom);
        desc.anchors_.left_ = ItemRef::reference(thumbnail, kPropertyRight);
        desc.anchors_.right_ = ItemRef::reference(inner, kPropertyRight);
        desc.anchors_.bottom_ = ItemRef::reference(row, kPropertyBottom);
        desc.margins_.set_top(ItemRef::reference(hidden, kUnitSize, 0.13));
        desc.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.26));
        desc.font_ = style.font_;
        desc.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
        desc.elide_ = Qt::ElideNone;
        desc.color_ = desc.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_));
        desc.background_ = desc.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
        desc.text_ =
          TVarRef::constant(TVar(QString::fromUtf8(rec.description_.c_str())));
        desc.setAttr("linewrap", true);

        // duration:
        Text & length = c2.addNew<Text>("length");
        length.anchors_.vcenter(c2);
        length.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
        length.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.13));
        length.font_ = style.font_;
        length.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
        length.font_.setWeight(62);
        length.elide_ = Qt::ElideRight;
        length.color_ = length.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_));
        length.background_ = length.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
        TTime dt(rec.gps_t1_ - rec.gps_t0_, 1);
        length.text_ = TVarRef::constant(TVar(dt.to_hhmmss()));

        // when recorded:
        Text & date = c3.addNew<Text>("date");
        date.anchors_.vcenter(c3);
        date.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
        date.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.13));
        date.font_ = style.font_;
        date.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
        date.font_.setWeight(62);
        date.elide_ = Qt::ElideRight;
        date.color_ = date.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_));
        date.background_ = date.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
        std::string ts = unix_epoch_time_to_localtime_str(rec.utc_t0_);
        date.text_ = TVarRef::constant(TVar(ts));

        // file size:
        Text & nbytes = c4.addNew<Text>("nbytes");
        nbytes.anchors_.vcenter(c4);
        nbytes.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
        nbytes.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.26));
        nbytes.font_ = style.font_;
        nbytes.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
        nbytes.font_.setWeight(62);
        nbytes.elide_ = Qt::ElideRight;
        nbytes.color_ = nbytes.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_));
        nbytes.background_ = nbytes.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
        nbytes.text_ = nbytes.addExpr(new GetFileSize(path));
        nbytes.text_.disableCaching();

        // trashcan:
        RoundRect & trashcan_bg = c4.addNew<RoundRect>("trashcan_bg");
        TexturedRect & trashcan = c4.addNew<TexturedRect>("trashcan");
        trashcan.anchors_.top_ =
          ItemRef::reference(nbytes, kPropertyTop);
        trashcan.anchors_.bottom_ =
          ItemRef::reference(nbytes, kPropertyBottom);
        trashcan.anchors_.right_ =
          ItemRef::reference(inner, kPropertyRight);
        trashcan.margins_.
          set_bottom(ItemRef::reference(nbytes, kPropertyFontDescent));
        trashcan.width_ = ItemRef::reference(trashcan, kPropertyHeight);
        trashcan.texture_ = trashcan.
          addExpr(new GetTexTrashcan(view, trashcan));

        trashcan_bg.anchors_.center(trashcan);
        trashcan_bg.width_ = ItemRef::scale(trashcan, kPropertyHeight, 2);
        trashcan_bg.height_ = ItemRef::reference(trashcan_bg, kPropertyWidth);
        trashcan_bg.radius_ = ItemRef::scale(trashcan_bg, kPropertyWidth, 0.5);
        trashcan_bg.color_ = trashcan_bg.
          addExpr(style_color_ref(view, &AppStyle::cursor_, 1.0));
        trashcan_bg.background_ = trashcan_bg.
          addExpr(style_color_ref(view, &AppStyle::cursor_, 0.0));

        DeleteRecording & trashcan_ia =
          c4.add(new DeleteRecording("trashcan_ia", view, name));
        trashcan_ia.anchors_.fill(trashcan_bg);
      }

      rows[name] = rowlayout_ptr;
    }

    // unreferenced playlist items must be removed:
    for (std::map<std::string, yae::shared_ptr<Layout> >::const_iterator
           i = layout.items_.begin(); i != layout.items_.end(); ++i)
    {
      const std::string & name = i->first;
      if (!yae::has(rows, name))
      {
        yae::shared_ptr<Item> row_ptr = i->second->item_;
        YAE_ASSERT(table.remove(row_ptr));
      }
    }

    layout.items_.swap(rows);
    table.uncache();
  }

  //----------------------------------------------------------------
  // AppView::on_watch_live
  //
  void
  AppView::on_watch_live(uint32_t ch_num)
  {
    TTime seek_pos = TTime::now();
    yae::queue_call(*this, &AppView::emit_watch_live, ch_num, seek_pos);
  }

  //----------------------------------------------------------------
  // AppView::show_program_details
  //
  void
  AppView::show_program_details(uint32_t ch_num, uint32_t gps_time)
  {
    const yae::mpeg_ts::EPG::Channel * channel = NULL;
    const yae::mpeg_ts::EPG::Program * program = NULL;

    if (!epg_.find(ch_num, gps_time, channel, program))
    {
      return;
    }

    program_sel_.reset(new DVR::ChanTime(ch_num, gps_time));

    // shortcuts:
    Item & panel = *(pd_layout_.item_);

    Scrollview & scrollview = panel.
      get<Scrollview>((panel.id_ + ".scrollview").c_str());
    Item & content = *(scrollview.content_);

    Item & body = content.Item::get<Item>("body");
    RoundRect & head_bg = body.Item::get<RoundRect>("head_bg");
    Item & ch_item = head_bg.Item::get<Item>("ch_item");

    Text & maj_min = ch_item.Item::get<Text>("maj_min");
    maj_min.text_ = TVarRef::constant
      (TVar(yae::strfmt("%i-%i", channel->major_, channel->minor_)));

    Text & ch_name = ch_item.Item::get<Text>("ch_name");
    ch_name.text_ = TVarRef::constant(TVar(channel->name_));

    Text & title = head_bg.Item::get<Text>("title");
    title.text_ = TVarRef::constant(TVar(program->title_));

    Text & rating = body.Item::get<Text>("rating");
    rating.text_ = TVarRef::constant(TVar("Rating: " + program->rating_));

    Text & desc = body.Item::get<Text>("desc");
    desc.text_ = TVarRef::constant(TVar(program->description_));

    // round to closest minute:
    Text & timing = head_bg.Item::get<Text>("timing");
    int64_t ts = yae::gps_time_to_unix_epoch_time(program->gps_time_);
    ts = 60 * ((ts + 29) / 60);

    struct tm t;
    unix_epoch_time_to_localtime(ts, t);

    const char * am_pm = (t.tm_hour < 12) ? "AM" : "PM";
    int hour =
      (t.tm_hour > 12) ? t.tm_hour % 12 :
      (t.tm_hour > 00) ? t.tm_hour : 12;

    std::string duration =
      yae::short_hours_minutes(program->duration_, "hr", "min");

    timing.text_ = TVarRef::constant
      (TVar(yae::strfmt("%s %s %i %i, %i:%02i %s, %s",
                        yae::kWeekdays[t.tm_wday],
                        yae::kMonths[t.tm_mon],
                        t.tm_mday,
                        t.tm_year + 1900,
                        hour,
                        t.tm_min,
                        am_pm,
                        duration.c_str())));

    requestUncache(&panel);
    requestRepaint();
  }

  //----------------------------------------------------------------
  // AppView::toggle_recording
  //
  void
  AppView::toggle_recording(uint32_t ch_num, uint32_t gps_time)
  {
    dvr_->toggle_recording(ch_num, gps_time);
    schedule_.clear();

    Item & epg_view = *epg_view_;
    Scrollview & vsv = epg_view.get<Scrollview>("vsv");
    Item & vsv_content = *(vsv.content_);
    Scrollview & hsv = vsv_content.get<Scrollview>("hsv");
    Item & hsv_content = *(hsv.content_);
    hsv_content.uncache();

    sync_ui_epg();
    requestUncache();
    requestRepaint();
  }

  //----------------------------------------------------------------
  // AppView::delete_recording
  //
  void
  AppView::delete_recording(const std::string & name)
  {
    TRecs::iterator found = recordings_.find(name);
    YAE_ASSERT(found != recordings_.end());
    if (found == recordings_.end())
    {
      return;
    }

    // ask for confirmation:
    TRecPtr rec_ptr = found->second;
    yae::queue_call(*this, &AppView::emit_confirm_delete, rec_ptr);
  }

  //----------------------------------------------------------------
  // AppView::playback_recording
  //
  void
  AppView::playback_recording(const std::string & name)
  {
    TRecs::iterator found = recordings_.find(name);
    YAE_ASSERT(found != recordings_.end());
    if (found == recordings_.end())
    {
      return;
    }

    const TRecPtr & rec_ptr = found->second;
    const Recording::Rec & rec = *rec_ptr;
    std::string basepath = rec.get_filepath(dvr_->basedir_, "");
    now_playing_.reset(new DVR::Playback(sidebar_sel_, name, basepath));

    yae::queue_call(*this, &AppView::emit_playback, rec_ptr);
    dataChanged();
    resetMouseState();
  }

  //----------------------------------------------------------------
  // AppView::watch_now
  //
  void
  AppView::watch_now(yae::shared_ptr<DVR::Playback> playback_ptr,
                     TRecPtr rec_ptr)
  {
    if (!(playback_ptr && rec_ptr))
    {
      return;
    }

    now_playing_ = playback_ptr;
    yae::queue_call(*this, &AppView::emit_playback, rec_ptr);
    dataChanged();
    resetMouseState();
  }

  //----------------------------------------------------------------
  // AppView::add_wishlist_item
  //
  void
  AppView::add_wishlist_item()
  {
    // shortcuts:
    AppView & view = *this;
    AppStyle & style = *style_;

    sidebar_sel_ = "wl: add";
    wi_edit_.reset(new std::pair<std::string, Wishlist::Item>
                   (std::string(), Wishlist::Item()));

    wishlist_ui_->uncache();
    requestRepaint();
  }

  //----------------------------------------------------------------
  // AppView::add_wishlist_item
  //
  void
  AppView::add_wishlist_item(const yae::shared_ptr<DVR::ChanTime> & prog_sel)
  {
    // shortcuts:
    AppView & view = *this;
    AppStyle & style = *style_;

    sidebar_sel_ = "wl: add";

    Wishlist::Item wi;
    if (prog_sel)
    {
      uint32_t ch_num = prog_sel->ch_num_;
      uint32_t gps_time = prog_sel->gps_time_;

      const yae::mpeg_ts::EPG::Channel * channel = NULL;
      const yae::mpeg_ts::EPG::Program * program = NULL;

      if (epg_.find(ch_num, gps_time, channel, program))
      {
        TTime t0((program->tm_.tm_hour * 60 +
                  program->tm_.tm_min) * 60 +
                 program->tm_.tm_sec, 1);

        wi.title_ = program->title_;
        wi.weekday_mask_ = (1 << program->tm_.tm_wday);
        wi.when_ = Timespan(t0, t0 + TTime(program->duration_, 1));
        wi.channel_ = std::make_pair(channel->major_, channel->minor_);
      }
    }

    wi_edit_.reset(new std::pair<std::string, Wishlist::Item>
                   (std::string(), wi));

    wishlist_ui_->uncache();
    requestRepaint();
  }

  //----------------------------------------------------------------
  // AppView::edit_wishlist_item
  //
  void
  AppView::edit_wishlist_item(const std::string & row_id)
  {
    // shortcuts:
    AppView & view = *this;
    AppStyle & style = *style_;

    std::string wi_key = row_id.substr(4);
    std::map<std::string, Wishlist::Item>::const_iterator found =
      wishlist_.find(wi_key);

    if (found != wishlist_.end())
    {
      wi_edit_.reset(new std::pair<std::string, Wishlist::Item>
                     (wi_key, found->second));
    }
    else
    {
      wi_edit_.reset(new std::pair<std::string, Wishlist::Item>
                     (std::string(), Wishlist::Item()));
    }

    wishlist_ui_->uncache();
    requestRepaint();
  }

  //----------------------------------------------------------------
  // AppView::remove_wishlist_item
  //
  void
  AppView::remove_wishlist_item(const std::string & wi_key)
  {
    dvr_->wishlist_remove(wi_key);
    dataChanged();
  }

  //----------------------------------------------------------------
  // AppView::save_wishlist_item
  //
  void
  AppView::save_wishlist_item()
  {
    if (wi_edit_)
    {
      const std::string & wi_key = wi_edit_->first;
      const Wishlist::Item & wi = wi_edit_->second;
      dvr_->wishlist_update(wi_key, wi);
      dataChanged();
    }
  }

  //----------------------------------------------------------------
  // AppView::update_wi_channel
  //
  void
  AppView::update_wi_channel(const QString & qstr)
  {
    if (wi_edit_)
    {
      Wishlist::Item & wi = wi_edit_->second;
      std::string txt(qstr.toUtf8().constData());
      const char * src = txt.empty() ? "" : &(txt[0]);

      // hh mm
      std::vector<std::string> tokens;
      std::size_t n = yae::split(tokens, ":;,/.- ", src);

      if (n == 2)
      {
        uint16_t major = boost::lexical_cast<uint16_t>(tokens[0]);
        uint16_t minor = boost::lexical_cast<uint16_t>(tokens[1]);
        wi.channel_ = std::make_pair(major, minor);
      }
      else
      {
        wi.channel_.reset();
      }

      requestUncache(wishlist_ui_.get());
      requestRepaint();
    }
  }

  //----------------------------------------------------------------
  // AppView::update_wi_title
  //
  void
  AppView::update_wi_title(const QString & qstr)
  {
    if (wi_edit_)
    {
      Wishlist::Item & wi = wi_edit_->second;
      std::string txt(qstr.toUtf8().constData());
      wi.set_title(txt);
      requestUncache(wishlist_ui_.get());
      requestRepaint();
    }
  }

  //----------------------------------------------------------------
  // AppView::update_wi_desc
  //
  void
  AppView::update_wi_desc(const QString & qstr)
  {
    if (wi_edit_)
    {
      Wishlist::Item & wi = wi_edit_->second;
      std::string txt(qstr.toUtf8().constData());
      wi.set_description(txt);
      requestUncache(wishlist_ui_.get());
      requestRepaint();
    }
  }

  //----------------------------------------------------------------
  // AppView::update_wi_time_start
  //
  void
  AppView::update_wi_time_start(const QString & qstr)
  {
    if (wi_edit_)
    {
      Wishlist::Item & wi = wi_edit_->second;
      std::string txt(qstr.toUtf8().constData());
      const char * src = txt.empty() ? "" : &(txt[0]);

      // hh mm
      std::vector<std::string> tokens;
      std::size_t n = yae::split(tokens, ":;,/.- ", src);

      if (n > 1)
      {
        int hh = boost::lexical_cast<int>(tokens[0]);
        int mm = boost::lexical_cast<int>(tokens[1]);
        TTime t(60 * (mm + 60 * hh), 1);

        Timespan when = wi.when_ ? *wi.when_ : Timespan(t, t);
        when.t0_ = t;
        wi.when_ = when;
      }
      else
      {
        wi.when_.reset();
      }

      requestUncache(wishlist_ui_.get());
      requestRepaint();
    }
  }

  //----------------------------------------------------------------
  // AppView::update_wi_time_end
  //
  void
  AppView::update_wi_time_end(const QString & qstr)
  {
    if (wi_edit_)
    {
      Wishlist::Item & wi = wi_edit_->second;
      std::string txt(qstr.toUtf8().constData());
      const char * src = txt.empty() ? "" : &(txt[0]);

      // hh mm
      std::vector<std::string> tokens;
      std::size_t n = yae::split(tokens, ":;,/.- ", src);

      if (n > 1)
      {
        int hh = boost::lexical_cast<int>(tokens[0]);
        int mm = boost::lexical_cast<int>(tokens[1]);
        TTime t(60 * (mm + 60 * hh), 1);

        Timespan when = wi.when_ ? *wi.when_ : Timespan(t, t);
        when.t1_ = t;
        wi.when_ = when;
      }
      else
      {
        wi.when_.reset();
      }

      requestUncache(wishlist_ui_.get());
      requestRepaint();
    }
  }

  //----------------------------------------------------------------
  // AppView::update_wi_date
  //
  void
  AppView::update_wi_date(const QString & qstr)
  {
    if (wi_edit_)
    {
      Wishlist::Item & wi = wi_edit_->second;
      std::string txt(qstr.toUtf8().constData());
      const char * src = txt.empty() ? "" : &(txt[0]);

      // yyyy mm dd
      std::vector<std::string> tokens;
      std::size_t n = yae::split(tokens, "/.- ", src);

      if (n == 3)
      {
        int yyyy = boost::lexical_cast<int>(tokens[0]);
        int mm = boost::lexical_cast<int>(tokens[1]);
        int dd = boost::lexical_cast<int>(tokens[2]);

        struct tm tm;
        memset(&tm, 0, sizeof(tm));

        tm.tm_year = yyyy < 100 ? yyyy + 100 : yyyy - 1900;
        tm.tm_mon = mm - 1;
        tm.tm_mday = dd;

        wi.date_ = tm;
      }
      else
      {
        wi.date_.reset();
      }

      requestUncache(wishlist_ui_.get());
      requestRepaint();
    }
  }

  //----------------------------------------------------------------
  // AppView::update_wi_min_minutes
  //
  void
  AppView::update_wi_min_minutes(const QString & qstr)
  {
    if (wi_edit_)
    {
      Wishlist::Item & wi = wi_edit_->second;
      std::string txt(qstr.toUtf8().constData());
      uint16_t n = boost::lexical_cast<uint16_t>(txt);

      if (n)
      {
        wi.min_minutes_ = n;
      }
      else
      {
        wi.min_minutes_.reset();
      }

      requestUncache(wishlist_ui_.get());
      requestRepaint();
    }
  }

  //----------------------------------------------------------------
  // AppView::update_wi_max_minutes
  //
  void
  AppView::update_wi_max_minutes(const QString & qstr)
  {
    if (wi_edit_)
    {
      Wishlist::Item & wi = wi_edit_->second;
      std::string txt(qstr.toUtf8().constData());
      uint16_t n = boost::lexical_cast<uint16_t>(txt);

      if (n)
      {
        wi.max_minutes_ = n;
      }
      else
      {
        wi.max_minutes_.reset();
      }

      requestUncache(wishlist_ui_.get());
      requestRepaint();
    }
  }

  //----------------------------------------------------------------
  // AppView::update_wi_max
  //
  void
  AppView::update_wi_max(const QString & qstr)
  {
    if (wi_edit_)
    {
      Wishlist::Item & wi = wi_edit_->second;
      std::string txt(qstr.toUtf8().constData());
      uint16_t n = boost::lexical_cast<uint16_t>(txt);

      if (n)
      {
        wi.max_recordings_ = n;
      }
      else
      {
        wi.max_recordings_.reset();
      }

      requestUncache(wishlist_ui_.get());
      requestRepaint();
    }
  }

  //----------------------------------------------------------------
  // AppView::layout
  //
  void
  AppView::layout(AppView & view, AppStyle & style, Item & root)
  {
    Item & hidden = root.addHidden(new Item("hidden"));
    hidden.width_ = hidden.
      addExpr(style_item_ref(view, &AppStyle::unit_size_));

    Rectangle & bg = root.addNew<Rectangle>("background");
    bg.color_ = bg.addExpr(style_color_ref(view, &AppStyle::bg_epg_));
    bg.anchors_.fill(root);

    Item & overview = root.addNew<Item>("overview");
    overview.anchors_.fill(bg);

    sideview_.reset(new Item("sideview"));
    Item & sideview = overview.add<Item>(sideview_);

    mainview_.reset(new Item("mainview"));
    Item & mainview = overview.add<Item>(mainview_);

    Rectangle & sep = overview.addNew<Rectangle>("separator");
    sep.color_ = sep.addExpr(style_color_ref(view, &AppStyle::bg_splitter_));

    Splitter & splitter = overview.
      add(new Splitter("splitter",
                       // lower bound:
                       hidden.addExpr
                       (new SplitterPos(view, overview, SplitterPos::kLeft)),
                       // upper bound:
                       hidden.addExpr
                       (new SplitterPos(view, overview, SplitterPos::kRight)),
                       // unit size:
                       ItemRef::reference(style.unit_size_),
                       // orientation:
                       Splitter::kHorizontal,
                       // initial position:
                       0.331));
    splitter.anchors_.inset(sep, -5.0, 0.0);

    sep.anchors_.top_ = ItemRef::reference(overview, kPropertyTop);
    sep.anchors_.bottom_ = ItemRef::reference(overview, kPropertyBottom);
    sep.anchors_.left_ = ItemRef::reference(splitter.pos_);
    sep.width_ = ItemRef::constant(1.0);

    sideview.anchors_.fill(overview);
    sideview.anchors_.right_ = ItemRef::reference(sep, kPropertyLeft);
    layout_sidebar(view, style, sideview);

    mainview.anchors_.fill(overview);
    mainview.anchors_.left_ = ItemRef::reference(sep, kPropertyRight);

    // layout Program Guide panel:
    layout_epg(view, style, mainview);

    // layout Channel List panel:
    layout_channels(view, style, mainview);

    // layout Scheduled Recordings panel:
    layout_schedule(view, style, mainview);

    // activated by double-clicking on an EPG item
    // useful for very short programs where neither
    // the program title doesn't fit in the EPG tile:
    layout_program_details(view, style, mainview);

    // layout Wishlist::Item editor panel:
    layout_wishlist(view, style, mainview);
  }

  //----------------------------------------------------------------
  // add_viewmode_row
  //
  static Select &
  add_viewmode_row(AppView & view,
                   AppStyle & style,
                   Item & sidebar,
                   Item & group,
                   Item * prev,
                   const char * id,
                   const char * text)
  {
    Item & root = *(view.root());
    Item & hidden = root.get<Item>("hidden");
    Item & body = group.get<Item>("body");

    Select & row = body.add(new Select(id, view, view.sidebar_sel_));

    row.anchors_.top_ = prev ?
      ItemRef::reference(*prev, kPropertyBottom) :
      ItemRef::reference(body, kPropertyTop);

    row.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    row.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
    row.height_ = ItemRef::reference(hidden, kUnitSize, 0.6);

    Rectangle & bg = row.addNew<Rectangle>("bg");
    bg.anchors_.fill(row);
    bg.anchors_.left_ = ItemRef::reference(sidebar, kPropertyLeft);
    bg.anchors_.right_ = ItemRef::reference(sidebar, kPropertyRight);
    bg.color_ = bg.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_scrollbar_, 0.33));
    bg.visible_ = bg.addExpr(new IsSelected(view.sidebar_sel_, row.id_));

    Text & label = row.addNew<Text>("label");
    label.font_ = style.font_;
    label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
    label.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
    label.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    label.elide_ = Qt::ElideRight;
    label.color_ = label.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 1.0));
    label.background_ = label.
      addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));
    label.text_ = TVarRef::constant(TVar(text));

    return row;
  }

  //----------------------------------------------------------------
  // AppView::layout_sidebar
  //
  void
  AppView::layout_sidebar(AppView & view, AppStyle & style, Item & panel)
  {
    Item & root = *(view.root_);
    Item & hidden = root.get<Item>("hidden");

    Rectangle & bg = panel.addNew<Rectangle>("bg_sidebar");
    bg.color_ = bg.addExpr(style_color_ref(view, &AppStyle::bg_sidebar_));
    bg.anchors_.fill(panel);

    Scrollview & sv =
      layout_scrollview(kScrollbarVertical, view, style, panel,
                        ItemRef::reference(hidden, kUnitSize, 0.33));
    bg.anchors_.right_ = ItemRef::reference(sv, kPropertyRight);

    Item & sidebar = *(sv.content_);
    Item & vscrollbar = panel.get<Item>("scrollbar");

    Item & spacer = sidebar.addNew<Item>("spacer");
    spacer.anchors_.top_ = ItemRef::reference(sidebar, kPropertyTop);
    spacer.anchors_.left_ = ItemRef::reference(sidebar, kPropertyLeft);
    spacer.anchors_.right_ = ItemRef::reference(sidebar, kPropertyRight);
    spacer.height_ = ItemRef::reference(hidden, kUnitSize, 0.14);

    // Digital Video Recorder:
    Item & top_group = add_collapsible_list(view,
                                            style,
                                            sidebar,
                                            "top_group",
                                            "Digital Video Recorder");
    top_group.anchors_.top_ =
      ItemRef::offset(spacer, kPropertyBottom);

    // Program Guide
    Select & view_epg =
      add_viewmode_row(view,
                       style,
                       sidebar,
                       top_group,
                       NULL, // prev
                       "view_mode_program_guide",
                       "Program Guide");

    // Channels
    Select & edit_channels =
      add_viewmode_row(view,
                       style,
                       sidebar,
                       top_group,
                       &view_epg, // prev
                       "view_mode_channel_list",
                       "Channels");

    // Schedule
    Select & edit_schedule =
      add_viewmode_row(view,
                       style,
                       sidebar,
                       top_group,
                       &edit_channels, // prev
                       "view_mode_schedule",
                       "Schedule");

    // Recordings
    Select & view_recordings =
      add_viewmode_row(view,
                       style,
                       sidebar,
                       top_group,
                       &edit_schedule, // prev
                       "view_mode_recordings",
                       "Recordings");

    // Playlist
    Item & playlist_group = add_collapsible_list(view,
                                                 style,
                                                 sidebar,
                                                 "playlist_group",
                                                 "Playlist");
    playlist_group.anchors_.top_ =
      ItemRef::offset(top_group, kPropertyBottom);

    // Wishlist
    Item & wishlist_group = add_collapsible_list(view,
                                                 style,
                                                 sidebar,
                                                 "wishlist_group",
                                                 "Wishlist");
    // add ( + ) button for adding wishlist items:
    {
      wishlist_group.anchors_.top_ =
        ItemRef::offset(playlist_group, kPropertyBottom);

      // shortcut:
      Item & body = wishlist_group.get<Item>("body");

      Item & row = wishlist_group.addNew<Item>("wishlist_add_item");
      row.anchors_.top_ = ItemRef::reference(body, kPropertyBottom);
      row.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
      row.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
      row.visible_ = BoolRef::reference(body, kPropertyVisible);
      row.height_ = row.addExpr(new InvisibleItemZeroHeight(row));

      Item & container = row.addNew<Item>("container");
      container.anchors_.top_ = ItemRef::reference(row, kPropertyTop);
      container.anchors_.left_ = ItemRef::reference(row, kPropertyLeft);
      container.anchors_.right_ = ItemRef::reference(row, kPropertyRight);
      container.margins_.set_top(ItemRef::reference(hidden, kUnitSize, 0.1));
      container.height_ = ItemRef::reference(hidden, kUnitSize, 0.6);

      RoundRect & btn = container.addNew<RoundRect>("btn");
      btn.radius_ = ItemRef::reference(hidden, kUnitSize, 0.13);
      btn.border_ = ItemRef::reference(hidden, kUnitSize, 0.01, 0.5);
      btn.anchors_.vcenter_ = ItemRef::reference(container, kPropertyVCenter);
      btn.anchors_.left_ = ItemRef::reference(container, kPropertyLeft);
      btn.height_ = ItemRef::reference(container, kPropertyHeight);
      btn.width_ = ItemRef::reference(hidden, kUnitSize, 0.9);
      btn.background_ = btn.
        addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));
      btn.color_ = btn.
        addExpr(style_color_ref(view, &AppStyle::bg_sidebar_, 0.0));
      btn.colorBorder_ = btn.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.5));

      Text & text = btn.addNew<Text>("text");
      text.anchors_.center(btn);
      text.font_ = style.font_;
      text.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.36);
      text.setAttr("oneline", true);
      text.elide_ = Qt::ElideNone;
      text.text_ = TVarRef::constant(TVar("+"));
      text.color_ = text.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.5));
      text.background_ = text.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_scrollbar_, 0.0));

      AddWishlistItem & ia = container.add(new AddWishlistItem("ia", view));
      ia.anchors_.fill(btn);
    }
  }

  //----------------------------------------------------------------
  // AppView::layout_epg
  //
  void
  AppView::layout_epg(AppView & view, AppStyle & style, Item & mainview)
  {
    Item & root = *(view.root_);
    Item & hidden = root.get<Item>("hidden");

    epg_view_.reset(new Item("epg_view"));
    Item & panel = mainview.add<Item>(epg_view_);
    panel.anchors_.fill(mainview);
    panel.visible_ = panel.
      addExpr(new IsSelected(sidebar_sel_, "view_mode_program_guide"));

    Gradient & ch_header = panel.addNew<Gradient>("ch_header");
    ch_header.anchors_.fill(panel);
    ch_header.anchors_.right_.reset();
    ch_header.width_ = ItemRef::reference(hidden, kUnitSize, 2.6);
    ch_header.anchors_.bottom_.reset();
    ch_header.height_ = ItemRef::reference(hidden, kUnitSize, 0.42);
    ch_header.orientation_ = Gradient::kVertical;
    ch_header.color_ = style.bg_epg_header_;

    Gradient & epg_header = panel.addNew<Gradient>("epg_header");
    epg_header.anchors_.fill(panel);
    epg_header.anchors_.left_ =
      ItemRef::reference(ch_header, kPropertyRight);
    epg_header.anchors_.bottom_ =
      ItemRef::reference(ch_header, kPropertyBottom);
    epg_header.orientation_ = Gradient::kVertical;
    epg_header.color_ = style.bg_epg_header_;

    FlickableArea & flickable =
      panel.add(new FlickableArea("flickable", view));

    Scrollview & vsv = panel.addNew<Scrollview>("vsv");
    vsv.clipContent_ = true;
    vsv.anchors_.top_ = ItemRef::reference(ch_header, kPropertyBottom);
    vsv.anchors_.left_ = ItemRef::reference(panel, kPropertyLeft);

    Rectangle & now_marker = panel.addNew<Rectangle>("now_marker");
    Item & chan_bar = panel.addNew<Item>("chan_bar");
    chan_bar.anchors_.fill(panel);
    chan_bar.anchors_.top_ = ItemRef::reference(ch_header, kPropertyBottom);
    chan_bar.anchors_.right_.reset();
    chan_bar.width_ = ItemRef::reference(ch_header, kPropertyWidth);

    Gradient & chan_bar_shadow = panel.addNew<Gradient>("chan_bar_shadow");
    chan_bar_shadow.anchors_.top_ =
      ItemRef::reference(chan_bar, kPropertyTop);
    chan_bar_shadow.anchors_.bottom_ =
      ItemRef::reference(chan_bar, kPropertyBottom);
    chan_bar_shadow.anchors_.left_ =
      ItemRef::reference(chan_bar, kPropertyRight);
    chan_bar_shadow.anchors_.right_.reset();
    chan_bar_shadow.width_ =
      ItemRef::reference(chan_bar, kPropertyWidth, 0.03, 1);
    chan_bar_shadow.orientation_ = Gradient::kHorizontal;
    chan_bar_shadow.color_ = style.bg_epg_shadow_;

    Item & vsv_content = *(vsv.content_);
    Scrollview & hsv = vsv_content.addNew<Scrollview>("hsv");
    Item & ch_list = vsv_content.addNew<Item>("ch_list");
    ch_list.anchors_.top_ = ItemRef::constant(0.0);
    ch_list.anchors_.left_ = ItemRef::constant(0.0);
    ch_list.width_ = ItemRef::reference(chan_bar, kPropertyWidth);

    hsv.uncacheContent_ = false;
    hsv.anchors_.top_ = ItemRef::reference(ch_list, kPropertyTop);
    hsv.anchors_.left_ = ItemRef::reference(ch_list, kPropertyRight);
    hsv.height_ = ItemRef::reference(ch_list, kPropertyHeight);

    Item & hsv_content = *(hsv.content_);
    Rectangle & hscrollbar = panel.addNew<Rectangle>("hscrollbar");
    Rectangle & vscrollbar = panel.addNew<Rectangle>("vscrollbar");

    hscrollbar.setAttr("vertical", false);
    hscrollbar.anchors_.left_ = ItemRef::reference(panel, kPropertyLeft);
    hscrollbar.anchors_.right_ = ItemRef::reference(vscrollbar, kPropertyLeft);
    hscrollbar.anchors_.bottom_ = ItemRef::reference(panel, kPropertyBottom);
    hscrollbar.height_ = ItemRef::reference(hidden, kUnitSize, 0.33);
    hscrollbar.color_ = hscrollbar.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_scrollbar_));

    vscrollbar.anchors_.top_ = ItemRef::reference(ch_header, kPropertyBottom);
    vscrollbar.anchors_.bottom_ = ItemRef::reference(hscrollbar, kPropertyTop);
    vscrollbar.anchors_.right_ = ItemRef::reference(panel, kPropertyRight);
    vscrollbar.width_ = ItemRef::reference(hidden, kUnitSize, 0.33);
    vscrollbar.color_ = hscrollbar.color_;

    chan_bar.anchors_.bottom_ = ItemRef::reference(hscrollbar, kPropertyTop);
    vsv.anchors_.right_ = ItemRef::reference(vscrollbar, kPropertyLeft);
    vsv.anchors_.bottom_ = ItemRef::reference(hscrollbar, kPropertyTop);
    hsv.width_ = hsv.addExpr(new CalcDistanceLeftRight(chan_bar, vscrollbar));

    // configure vscrollbar slider:
    InputArea & vscrollbar_ia = vscrollbar.addNew<InputArea>("ia");
    vscrollbar_ia.anchors_.fill(vscrollbar);

    RoundRect & vslider = vscrollbar.addNew<RoundRect>("vslider");
    vslider.anchors_.top_ = vslider.
      addExpr(new CalcSliderTop(vsv, vscrollbar, vslider));
    vslider.anchors_.left_ =
      ItemRef::offset(vscrollbar, kPropertyLeft, 2.5);
    vslider.anchors_.right_ =
      ItemRef::offset(vscrollbar, kPropertyRight, -2.5);
    vslider.height_ = vslider.
      addExpr(new CalcSliderHeight(vsv, vscrollbar, vslider));
    vslider.radius_ =
      ItemRef::scale(vslider, kPropertyWidth, 0.5);
    vslider.color_ = vslider.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_scrollbar_));
    vslider.background_ = hscrollbar.color_;

    SliderDrag & vslider_ia =
      vslider.add(new SliderDrag("ia", view, vsv, vscrollbar));
    vslider_ia.anchors_.fill(vslider);

    // configure horizontal scrollbar slider:
    InputArea & hscrollbar_ia = hscrollbar.addNew<InputArea>("ia");
    hscrollbar_ia.anchors_.fill(hscrollbar);

    RoundRect & hslider = hscrollbar.addNew<RoundRect>("hslider");
    hslider.anchors_.top_ =
      ItemRef::offset(hscrollbar, kPropertyTop, 2.5);
    hslider.anchors_.bottom_ =
      ItemRef::offset(hscrollbar, kPropertyBottom, -2.5);
    hslider.anchors_.left_ = hslider.
      addExpr(new CalcSliderLeft(hsv, hscrollbar, hslider));
    hslider.width_ = hslider.
      addExpr(new CalcSliderWidth(hsv, hscrollbar, hslider));
    hslider.radius_ =
      ItemRef::scale(hslider, kPropertyHeight, 0.5);
    hslider.background_ = hscrollbar.color_;
    hslider.color_ = vslider.color_;

    SliderDrag & hslider_ia =
      hslider.add(new SliderDrag("ia", view, hsv, hscrollbar));
    hslider_ia.anchors_.fill(hslider);

    // enable flicking the scrollviews:
    flickable.setVerSlider(&vslider_ia);
    flickable.setHorSlider(&hslider_ia);
    flickable.anchors_.fill(vsv);

    // setup tickmarks scrollview:
    Scrollview & timeline = epg_header.addNew<Scrollview>("timeline");
    {
      timeline.clipContent_ = true;
      timeline.anchors_.fill(epg_header);
      timeline.position_y_ = ItemRef::constant(0.0);
      timeline.position_x_ = timeline.
        addExpr(new CalcViewPositionX(hsv, timeline));
      timeline.position_x_.disableCaching();

      Item & t = *(timeline.content_);
      t.anchors_.top_ = ItemRef::constant(0.0);
      t.anchors_.left_ = ItemRef::reference(hsv_content, kPropertyLeft);
      t.anchors_.right_ = ItemRef::reference(hsv_content, kPropertyRight);
      t.height_ = ItemRef::reference(epg_header, kPropertyHeight);
    }

    // setup "now" time marker:
    now_marker.color_ = now_marker.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.2));
    now_marker.width_ = ItemRef::constant(1.0);
    now_marker.anchors_.top_ = ItemRef::reference(panel, kPropertyTop);
    now_marker.anchors_.bottom_ = ItemRef::reference(hscrollbar, kPropertyTop);
    now_marker.anchors_.left_ = now_marker.
      addExpr(new NowMarkerPos(view, epg_header, hsv));
    now_marker.anchors_.left_.disableCaching();
    now_marker.xExtentDisableCaching();
  }

  //----------------------------------------------------------------
  // AppView::layout_program_details
  //
  void
  AppView::layout_program_details(AppView & view,
                                  AppStyle & style,
                                  Item & mainview)
  {
    // shortcuts:
    Item & root = *root_;
    Item & hidden = root.get<Item>("hidden");
    Layout & layout = pd_layout_;

    layout.item_.reset(new Item("program_details_view"));
    Item & panel = mainview.add<Item>(layout.item_);

    panel.anchors_.fill(mainview);
    panel.visible_ = panel.addExpr(new ProgramDetailsVisible(view));

    // setup a mouse trap to prevent unintended click-through:
    MouseTrap & mouse_trap = panel.addNew<MouseTrap>("mouse_trap");
    mouse_trap.anchors_.fill(panel);

    Rectangle & bg = panel.addNew<Rectangle>("bg");
    bg.anchors_.fill(panel);
    bg.color_ = bg.addExpr(style_color_ref(view, &AppStyle::bg_epg_));

    Scrollview & scrollview =
      layout_scrollview(kScrollbarVertical, view, style, panel,
                        ItemRef::reference(hidden, kUnitSize, 0.33));
    Item & content = *(scrollview.content_);

    Item & body = content.addNew<Item>("body");
    body.anchors_.top_ = ItemRef::reference(hidden, kUnitSize, 1);
    body.anchors_.left_ = ItemRef::reference(hidden, kUnitSize, 1);
    body.width_ = body.addExpr(new WidthMinusMargin(view, panel, 2));


    Item & r1 = body.addNew<Item>("r1");
    r1.anchors_.fill(body);
    r1.anchors_.bottom_.reset();
    r1.height_ = ItemRef::reference(hidden, kUnitSize, 1.6);

    Item & r2 = body.addNew<Item>("r2");
    r2.anchors_.fill(body);
    r2.anchors_.top_ = ItemRef::reference(r1, kPropertyBottom);
    r2.anchors_.bottom_.reset();
    r2.height_ = ItemRef::reference(hidden, kUnitSize, 0.8);

    Item & r3 = body.addNew<Item>("r3");
    r3.anchors_.fill(body);
    r3.anchors_.top_ = ItemRef::reference(r2, kPropertyBottom);
    r3.anchors_.bottom_.reset();
    r3.height_ = ItemRef::reference(hidden, kUnitSize, 0.4125);

    RoundRect & head_bg = body.addNew<RoundRect>("head_bg");
    head_bg.anchors_.top_ = ItemRef::reference(r1, kPropertyTop);
    head_bg.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    head_bg.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
    head_bg.anchors_.bottom_ = ItemRef::reference(r3, kPropertyBottom);
    head_bg.radius_ = ItemRef::reference(hidden, kUnitSize, 0.13);
    head_bg.background_ = head_bg.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_));
    head_bg.color_ = head_bg.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));

    Item & ch_item = head_bg.addNew<Item>("ch_item");
    ch_item.anchors_.left_ = ItemRef::reference(head_bg, kPropertyLeft);

    Text & maj_min = ch_item.addNew<Text>("maj_min");
    maj_min.font_ = style.font_;
    maj_min.fontSize_ = ItemRef::reference(hidden, kUnitSize, 1.0);
    maj_min.anchors_.left_ = ItemRef::reference(ch_item, kPropertyLeft);
    maj_min.anchors_.bottom_ = ItemRef::reference(r1, kPropertyBottom);
    maj_min.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.5));
    maj_min.alignment_ = Qt::AlignLeft;
    maj_min.elide_ = Qt::ElideRight;
    maj_min.color_ = maj_min.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
    maj_min.background_ = maj_min.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    maj_min.text_ = TVarRef::constant(TVar(QString::fromUtf8("X-Y")));

    Item & sp1 = ch_item.addNew<Item>("sp1");
    sp1.anchors_.left_ = ItemRef::reference(maj_min, kPropertyRight);
    sp1.anchors_.bottom_ = ItemRef::reference(maj_min, kPropertyBottom);
    sp1.height_ = ItemRef::reference(maj_min, kPropertyHeight);
    sp1.width_ = ItemRef::reference(hidden, kUnitSize, 0.5);

    Text & ch_name = ch_item.addNew<Text>("ch_name");
    ch_name.font_ = style.font_;
    ch_name.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.5);
    ch_name.anchors_.left_ = ItemRef::reference(ch_item, kPropertyLeft);
    ch_name.anchors_.bottom_ = ItemRef::reference(r2, kPropertyBottom);
    ch_name.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.5));
    ch_name.alignment_ = Qt::AlignLeft;
    ch_name.elide_ = Qt::ElideRight;
    ch_name.color_ = ch_name.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
    ch_name.background_ = ch_name.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    ch_name.text_ = TVarRef::constant(TVar(QString::fromUtf8("YAE TV")));

    Item & sp2 = ch_item.addNew<Item>("sp2");
    sp2.anchors_.left_ = ItemRef::reference(ch_name, kPropertyRight);
    sp2.anchors_.bottom_ = ItemRef::reference(ch_name, kPropertyBottom);
    sp2.height_ = ItemRef::reference(ch_name, kPropertyHeight);
    sp2.width_ = ItemRef::reference(hidden, kUnitSize, 0.5);

    Rectangle & ch_sep = head_bg.addNew<Rectangle>("ch_sep");
    ch_sep.anchors_.left_ = ItemRef::reference(ch_item, kPropertyRight);
    ch_sep.anchors_.top_ = ItemRef::reference(ch_item, kPropertyTop);
    ch_sep.anchors_.bottom_ = ItemRef::reference(ch_item, kPropertyBottom);
    ch_sep.width_ = ItemRef::reference(hidden, kUnitSize, 0.03, 1);
    ch_sep.color_ = ch_sep.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));

    Text & title = head_bg.addNew<Text>("title");
    title.font_ = style.font_;
    title.fontSize_ = ItemRef::reference(hidden, kUnitSize, 1.0);
    title.anchors_.left_ = ItemRef::reference(ch_sep, kPropertyRight);
    title.anchors_.right_ = ItemRef::reference(head_bg, kPropertyRight);
    title.anchors_.bottom_ = ItemRef::reference(r1, kPropertyBottom);
    title.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.5));
    title.alignment_ = Qt::AlignLeft;
    title.elide_ = Qt::ElideRight;
    title.color_ = title.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
    title.background_ = title.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    title.text_ = TVarRef::constant(TVar(QString::fromUtf8("Program Title")));

    Text & timing = head_bg.addNew<Text>("timing");
    timing.font_ = style.font_;
    timing.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.5);
    timing.anchors_.left_ = ItemRef::reference(title, kPropertyLeft);
    timing.anchors_.right_ = ItemRef::reference(head_bg, kPropertyRight);
    timing.anchors_.bottom_ = ItemRef::reference(r2, kPropertyBottom);
    timing.alignment_ = Qt::AlignLeft;
    timing.elide_ = Qt::ElideRight;
    timing.color_ = timing.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
    timing.background_ = timing.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    timing.text_ = TVarRef::constant
      (TVar(QString::fromUtf8("Fri Feb 28 2020, 10:00 PM, 2hr 30min")));

    Item & paragraphs = body.addNew<Item>("paragraphs");
    paragraphs.anchors_.fill(body);
    paragraphs.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.5));
    paragraphs.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.5));
    paragraphs.anchors_.top_ = ItemRef::reference(head_bg, kPropertyBottom);
    paragraphs.anchors_.bottom_.reset();
    paragraphs.height_ = ItemRef::constant(0.0);

    Item & r4 = body.addNew<Item>("r4");
    r4.anchors_.fill(paragraphs);
    r4.anchors_.bottom_.reset();
    r4.height_ = ItemRef::reference(hidden, kUnitSize, 1.2);

    Text & rating = body.addNew<Text>("rating");
    rating.font_ = style.font_;
    rating.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.5);
    rating.anchors_.left_ = ItemRef::reference(paragraphs, kPropertyLeft);
    rating.anchors_.bottom_ = ItemRef::reference(r4, kPropertyBottom);
    rating.alignment_ = Qt::AlignLeft;
    rating.elide_ = Qt::ElideRight;
    rating.color_ = rating.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
    rating.background_ = rating.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    rating.text_ = TVarRef::constant(TVar(QString::fromUtf8("Rating: PG-13")));

    Text & desc = body.addNew<Text>("desc");
    desc.font_ = style.font_;
    desc.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.5);
    desc.anchors_.left_ = ItemRef::reference(paragraphs, kPropertyLeft);
    desc.anchors_.right_ = ItemRef::reference(paragraphs, kPropertyRight);
    desc.anchors_.top_ = ItemRef::reference(r4, kPropertyBottom);
    desc.margins_.set_top(ItemRef::reference(hidden, kUnitSize, 0.3));
    desc.alignment_ = Qt::AlignLeft;
    desc.elide_ = Qt::ElideNone;
    desc.color_ = desc.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
    desc.background_ = desc.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    const char * description = "Once upon a time in a galaxy far away "
      "a fox jumped over a box ... or something like that, idk... "
      "I just need some filler text to fill out the description "
      "for evaluation purposes.  This ought to be enough";
    desc.text_ = TVarRef::constant(TVar(QString::fromUtf8(description)));

    Item & r5 = body.addNew<Item>("r5");
    r5.anchors_.fill(paragraphs);
    r5.anchors_.top_ = ItemRef::reference(desc, kPropertyBottom);
    r5.anchors_.bottom_.reset();
    r5.height_ = ItemRef::reference(hidden, kUnitSize, 1.2);

    Text & status = body.addNew<Text>("status");
    status.font_ = style.font_;
    status.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.5);
    status.anchors_.left_ = ItemRef::reference(paragraphs, kPropertyLeft);
    status.anchors_.right_ = ItemRef::reference(paragraphs, kPropertyRight);
    status.anchors_.bottom_ = ItemRef::reference(r5, kPropertyBottom);
    status.alignment_ = Qt::AlignLeft;
    status.elide_ = Qt::ElideRight;
    status.color_ = status.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
    status.background_ = status.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    status.text_ = status.addExpr(new GetProgramDetailsStatus(view));

    Item & r6 = body.addNew<Item>("r6");
    r6.anchors_.fill(paragraphs);
    r6.anchors_.top_ = ItemRef::reference(r5, kPropertyBottom);
    r6.anchors_.bottom_.reset();
    r6.margins_.set_top(ItemRef::reference(hidden, kUnitSize, 0.4));
    r6.height_ = ItemRef::reference(hidden, kUnitSize, 1.2);

    RoundRect & bg_toggle = body.addNew<RoundRect>("bg_toggle");
    RoundRect & bg_close = body.addNew<RoundRect>("bg_close");
    RoundRect & bg_add_wi = body.addNew<RoundRect>("bg_add_wi");

    Text & tx_toggle = body.addNew<Text>("tx_toggle");
    Text & tx_close = body.addNew<Text>("tx_close");
    Text & tx_add_wi = body.addNew<Text>("tx_add_wi");

    tx_toggle.anchors_.bottom_ = ItemRef::reference(r6, kPropertyBottom);
    tx_toggle.anchors_.left_ = ItemRef::reference(paragraphs, kPropertyLeft);
    tx_toggle.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.5));
    tx_toggle.color_ = tx_toggle.
      addExpr(style_color_ref(view, &AppStyle::cursor_fg_));
    tx_toggle.background_ = tx_toggle.
      addExpr(style_color_ref(view, &AppStyle::cursor_, 0.0));
    tx_toggle.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.5);
    tx_toggle.elide_ = Qt::ElideNone;
    tx_toggle.setAttr("oneline", true);
    tx_toggle.text_ = tx_toggle.addExpr(new GetProgramDetailsToggleText(view));

    bg_toggle.anchors_.fill(tx_toggle);
    bg_toggle.margins_.set_left(ItemRef::reference(hidden, kUnitSize, -0.5));
    bg_toggle.margins_.set_right(ItemRef::reference(hidden, kUnitSize, -0.5));
    bg_toggle.margins_.set_top(ItemRef::reference(hidden, kUnitSize, -0.2));
    bg_toggle.margins_.set_bottom(ItemRef::reference(hidden, kUnitSize, -0.2));
    bg_toggle.color_ = bg_toggle.
      addExpr(style_color_ref(view, &AppStyle::cursor_));
    bg_toggle.background_ = bg_toggle.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_, 0.0));
    bg_toggle.radius_ = ItemRef::scale(bg_toggle, kPropertyHeight, 0.1);

    ProgramDetailsToggleRecording & on_toggle = bg_toggle.
      add(new ProgramDetailsToggleRecording("on_toggle", *this));
    on_toggle.anchors_.fill(bg_toggle);

    tx_close.anchors_.bottom_ = ItemRef::reference(r6, kPropertyBottom);
    tx_close.anchors_.left_ = ItemRef::reference(tx_toggle, kPropertyRight);
    tx_close.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 1.6));
    tx_close.text_ = TVarRef::constant(TVar("Close"));
    tx_close.color_ = tx_close.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
    tx_close.background_ = tx_close.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    tx_close.fontSize_ = tx_toggle.fontSize_;
    tx_close.elide_ = Qt::ElideNone;
    tx_close.setAttr("oneline", true);

    bg_close.anchors_.fill(tx_close);
    bg_close.margins_.set_left(ItemRef::reference(hidden, kUnitSize, -0.5));
    bg_close.margins_.set_right(ItemRef::reference(hidden, kUnitSize, -0.5));
    bg_close.margins_.set_top(ItemRef::reference(hidden, kUnitSize, -0.2));
    bg_close.margins_.set_bottom(ItemRef::reference(hidden, kUnitSize, -0.2));
    bg_close.color_ = bg_close.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
    bg_close.background_ = bg_close.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_, 0.0));
    bg_close.radius_ = ItemRef::scale(bg_close, kPropertyHeight, 0.1);

    CloseProgramDetails & on_close = bg_close.
      add(new CloseProgramDetails("on_close", *this));
    on_close.anchors_.fill(bg_close);

    tx_add_wi.anchors_.bottom_ = ItemRef::reference(r6, kPropertyBottom);
    tx_add_wi.anchors_.right_ = ItemRef::reference(paragraphs, kPropertyRight);
    tx_add_wi.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.5));
    tx_add_wi.text_ = TVarRef::constant(TVar("New Wishlist"));
    tx_add_wi.color_ = tx_add_wi.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
    tx_add_wi.background_ = tx_add_wi.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    tx_add_wi.fontSize_ = tx_toggle.fontSize_;
    tx_add_wi.elide_ = Qt::ElideNone;
    tx_add_wi.setAttr("oneline", true);

    bg_add_wi.anchors_.fill(tx_add_wi);
    bg_add_wi.margins_.set_left(ItemRef::reference(hidden, kUnitSize, -0.5));
    bg_add_wi.margins_.set_right(ItemRef::reference(hidden, kUnitSize, -0.5));
    bg_add_wi.margins_.set_top(ItemRef::reference(hidden, kUnitSize, -0.2));
    bg_add_wi.margins_.set_bottom(ItemRef::reference(hidden, kUnitSize, -0.2));
    bg_add_wi.color_ = bg_add_wi.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
    bg_add_wi.background_ = bg_add_wi.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_, 0.0));
    bg_add_wi.radius_ = ItemRef::scale(bg_add_wi, kPropertyHeight, 0.1);

    ProgramDetailsAddWishlist & on_add_wi = bg_add_wi.
      add(new ProgramDetailsAddWishlist("on_add_wi", *this));
    on_add_wi.anchors_.fill(bg_add_wi);

    // spacer:
    Item & r7 = body.addNew<Item>("r7");
    r7.anchors_.fill(paragraphs);
    r7.anchors_.top_ = ItemRef::reference(r6, kPropertyBottom);
    r7.anchors_.bottom_.reset();
    r7.height_ = ItemRef::reference(hidden, kUnitSize, 1);
  }

  //----------------------------------------------------------------
  // AppView::layout_channels
  //
  void
  AppView::layout_channels(AppView & view, AppStyle & style, Item & mainview)
  {
    // shortcuts:
    Item & root = *root_;
    Item & hidden = root.get<Item>("hidden");
    Layout & layout = ch_layout_;

    layout.item_.reset(new Item("channel_view"));
    Item & panel = mainview.add<Item>(layout.item_);
    panel.anchors_.fill(mainview);
    panel.visible_ = panel.
      addExpr(new IsSelected(sidebar_sel_, "view_mode_channel_list"));

    layout_scrollview(kScrollbarVertical, view, style, panel,
                      ItemRef::reference(hidden, kUnitSize, 0.33));
  }

  //----------------------------------------------------------------
  // AppView::layout_schedule
  //
  void
  AppView::layout_schedule(AppView & view, AppStyle & style, Item & mainview)
  {
    // shortcuts:
    Item & root = *root_;
    Item & hidden = root.get<Item>("hidden");
    Layout & layout = sch_layout_;

    layout.item_.reset(new Item("schedule_view"));
    Item & panel = mainview.add<Item>(layout.item_);
    panel.anchors_.fill(mainview);
    panel.visible_ = panel.
      addExpr(new IsSelected(sidebar_sel_, "view_mode_schedule"));

    Item & header = panel.addNew<Item>("header");
    header.anchors_.fill(panel);
    header.anchors_.bottom_.reset();
    header.height_ = ItemRef::reference(hidden, kUnitSize, 0.4);

    Rectangle & bg = header.addNew<Rectangle>("bg");
    bg.anchors_.fill(header);
    bg.color_ = bg.addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));

    // add table columns:
    Item & h1 = header.addNew<Item>("h1");
    Rectangle & l1 = header.addNew<Rectangle>("l1");
    Item & h2 = header.addNew<Item>("h2");
    Rectangle & l2 = header.addNew<Rectangle>("l2");
    Item & h3 = header.addNew<Item>("h3");

    h1.anchors_.top_ = ItemRef::reference(header, kPropertyTop);
    h1.anchors_.left_ = ItemRef::reference(header, kPropertyLeft);
    h1.anchors_.right_ = ItemRef::reference(l1, kPropertyLeft);
    h1.anchors_.bottom_ = ItemRef::reference(header, kPropertyBottom);

    Text & t1 = h1.addNew<Text>("t1");
    t1.anchors_.vcenter(h1);
    t1.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
    t1.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.13));
    t1.font_ = style.font_;
    t1.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
    t1.elide_ = Qt::ElideRight;
    t1.color_ = t1.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
    t1.background_ = t1.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    t1.text_ = TVarRef::constant(TVar("Title and Description"));

    l1.anchors_.top_ = ItemRef::offset(header, kPropertyTop, 1);
    l1.anchors_.right_ = ItemRef::reference(h2, kPropertyLeft);
    l1.anchors_.bottom_ = ItemRef::offset(header, kPropertyBottom, -2);
    l1.width_ = ItemRef::constant(1);
    l1.color_ = l1.addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.3));

    h2.anchors_.top_ = ItemRef::reference(header, kPropertyTop);
    h2.anchors_.right_ = ItemRef::reference(l2, kPropertyLeft);
    h2.anchors_.bottom_ = ItemRef::reference(header, kPropertyBottom);
    h2.width_ = ItemRef::reference(hidden, kUnitSize, 2.0);

    Text & t2 = h2.addNew<Text>("t2");
    t2.anchors_.vcenter(h2);
    t2.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
    t2.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.13));
    t2.font_ = style.font_;
    t2.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
    t2.elide_ = Qt::ElideRight;
    t2.color_ = t2.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
    t2.background_ = t2.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    t2.text_ = TVarRef::constant(TVar("Duration"));

    l2.anchors_.top_ = ItemRef::offset(header, kPropertyTop, 1);
    l2.anchors_.right_ = ItemRef::reference(h3, kPropertyLeft);
    l2.anchors_.bottom_ = ItemRef::offset(header, kPropertyBottom, -2);
    l2.width_ = ItemRef::constant(1);
    l2.color_ = l1.addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.3));

    h3.anchors_.top_ = ItemRef::reference(header, kPropertyTop);
    h3.anchors_.bottom_ = ItemRef::reference(header, kPropertyBottom);
    h3.anchors_.right_ = ItemRef::reference(header, kPropertyRight);
    h3.width_ = ItemRef::reference(hidden, kUnitSize, 3.5);

    Text & t3 = h3.addNew<Text>("t3");
    t3.anchors_.vcenter(h3);
    t3.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
    t3.margins_.set_right(ItemRef::reference(hidden, kUnitSize, 0.13));
    t3.font_ = style.font_;
    t3.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
    t3.elide_ = Qt::ElideRight;
    t3.color_ = t3.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
    t3.background_ = t3.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    t3.text_ = TVarRef::constant(TVar("Date"));

    // layout the table container:
    Item & container = panel.addNew<Item>("container");
    container.anchors_.fill(panel);
    container.anchors_.top_ = ItemRef::reference(header, kPropertyBottom);

    layout_scrollview(kScrollbarVertical, view, style, container,
                      ItemRef::reference(hidden, kUnitSize, 0.33));
  }

  //----------------------------------------------------------------
  // AppView::layout_wishlist
  //
  void
  AppView::layout_wishlist(AppView & view, AppStyle & style, Item & mainview)
  {
    // shortcuts:
    bool ok = true;
    Item & root = *root_;
    Item & hidden = root.get<Item>("hidden");

    wishlist_ui_.reset(new Item("wishlist_ui"));
    Item & panel = mainview.add<Item>(wishlist_ui_);
    panel.anchors_.fill(mainview);
    panel.visible_ = panel.addExpr(new IsWishlistSelected(sidebar_sel_));

    Item & body = panel.addNew<Item>("body");
    body.anchors_.fill(panel);
    body.margins_.set(ItemRef::reference(hidden, kUnitSize, 1.0));

    Item & c1 = body.addNew<Item>("c1");
    Item & c2 = body.addNew<Item>("c2");
    Item & c3 = body.addNew<Item>("c3");

    c1.anchors_.fill(body);
    c1.anchors_.right_.reset();
    c1.width_ = ItemRef::reference(hidden, kUnitSize, 3);

    c2.anchors_.fill(body);
    c2.anchors_.left_ = ItemRef::reference(c1, kPropertyRight);
    c2.anchors_.right_.reset();
    c2.width_ = ItemRef::reference(hidden, kUnitSize, 0.3);

    c3.anchors_.fill(body);
    c3.anchors_.left_ = c3.addExpr(new RoundUp(c2, kPropertyRight));
    c3.anchors_.right_ = c3.addExpr(new RoundUp(body, kPropertyRight));
    c3.width_ = c3.addExpr(new RoundUp(body, kPropertyWidth, 0.75));

    Item & r1 = body.addNew<Item>("r1");
    r1.anchors_.fill(body);
    r1.anchors_.bottom_.reset();
    r1.height_ = ItemRef::reference(hidden, kUnitSize, 1.0);

    // layout the channel number row:
    {
      // shortcut:
      Item & row = r1;

      Text & label = row.addNew<Text>("label");
      label.font_ = style.font_;
      label.font_.setWeight(62);
      label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
      label.anchors_.bottom_ = label.
        addExpr(new RoundUp(row, kPropertyVCenter));
      label.anchors_.right_ = ItemRef::reference(c1, kPropertyRight);
      label.elide_ = Qt::ElideNone;
      label.text_ = TVarRef::constant(TVar("Channel"));
      label.color_ = label.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      label.background_ = label.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

      Rectangle & text_bg = row.
        addNew<Rectangle>("text_bg");

      Text & text = row.
        addNew<Text>("text");

      TextInput & edit = row.
        addNew<TextInput>("edit");

      TextInputProxy & focus = row.
        add(new TextInputProxy("focus_channel", text, edit));

      focus.anchors_.fill(text_bg);
      focus.bgNoFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.5));
      focus.bgOnFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      focus.copyViewToEdit_ = BoolRef::constant(true);
      focus.editingFinishedOnFocusOut_ = BoolRef::constant(true);

      ItemFocus::singleton().
        setFocusable(view, focus, "wishlist_ui", 0);

      text.anchors_.bottom_ = ItemRef::reference(label, kPropertyBottom);
      text.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      text.width_ = ItemRef::reference(hidden, kUnitSize, 3);
      text.visible_ = text.addExpr(new ShowWhenFocused(focus, false));
      text.color_ = text.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      text.background_ = text.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      text.text_ = text.addExpr(new GetWishlistItemChannel(view));
      text.font_ = style.font_;
      text.font_.setWeight(62);
      text.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);

      text_bg.anchors_.offset(text, -3, 3, -3, 1);
      text_bg.margins_.set_top(ItemRef::reference(hidden, kUnitSize, -0.03));
      text_bg.color_ = text_bg.addExpr(new ColorWhenFocused(focus));
      text_bg.color_.disableCaching();

      edit.anchors_.fill(text);
      edit.margins_.set_right(ItemRef::scale(edit,
                                             kPropertyCursorWidth,
                                             -1.0));
      edit.visible_ = edit.
        addExpr(new ShowWhenFocused(focus, true));
      edit.color_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      edit.background_ =
        ColorRef::transparent(focus, kPropertyColorOnFocusBg);
      edit.cursorColor_ = edit.
        addExpr(style_color_ref(view, &AppStyle::cursor_, 1.0));
      edit.font_ = text.font_;
      edit.fontSize_ = text.fontSize_;

      edit.selectionBg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::bg_edit_selected_, 1.0));
      edit.selectionFg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_edit_selected_, 1.0));

      ok = connect(&edit, SIGNAL(editingFinished(const QString &)),
                   &view, SLOT(update_wi_channel(const QString &)));
      YAE_ASSERT(ok);

      Text & note = row.addNew<Text>("note");
      note.font_ = style.font_;
      note.font_.setWeight(62);
      note.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      note.anchors_.top_ = ItemRef::reference(text_bg, kPropertyBottom);
      note.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      note.elide_ = Qt::ElideNone;
      note.text_ = TVarRef::constant(TVar("optional, M-N format"));
      note.color_ = note.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.8));
      note.background_ = note.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    }

    Item & r2 = body.addNew<Item>("r2");
    r2.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    r2.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
    r2.anchors_.top_ = ItemRef::reference(r1, kPropertyBottom);
    r2.height_ = ItemRef::reference(hidden, kUnitSize, 1.0);

    // layout the program title/regex row:
    {
      // shortcut:
      Item & row = r2;

      Text & label = row.addNew<Text>("label");
      label.font_ = style.font_;
      label.font_.setWeight(62);
      label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
      label.anchors_.bottom_ = label.
        addExpr(new RoundUp(row, kPropertyVCenter));
      label.anchors_.right_ = ItemRef::reference(c1, kPropertyRight);
      label.elide_ = Qt::ElideNone;
      label.text_ = TVarRef::constant(TVar("Program Title"));
      label.color_ = label.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      label.background_ = label.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

      Rectangle & text_bg = row.
        addNew<Rectangle>("text_bg");

      Text & text = row.
        addNew<Text>("text");

      TextInput & edit = row.
        addNew<TextInput>("edit");

      TextInputProxy & focus = row.
        add(new TextInputProxy("focus_title_rx", text, edit));

      focus.anchors_.fill(text_bg);
      focus.bgNoFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.5));
      focus.bgOnFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      focus.copyViewToEdit_ = BoolRef::constant(true);
      focus.editingFinishedOnFocusOut_ = BoolRef::constant(true);

      ItemFocus::singleton().
        setFocusable(view, focus, "wishlist_ui", 1);

      text.anchors_.bottom_ = ItemRef::reference(label, kPropertyBottom);
      text.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      text.anchors_.right_ = ItemRef::reference(c3, kPropertyRight);
      text.visible_ = text.addExpr(new ShowWhenFocused(focus, false));
      text.color_ = text.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      text.background_ = text.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      text.text_ = text.addExpr(new GetWishlistItemTitle(view));
      text.font_ = style.font_;
      text.font_.setWeight(62);
      text.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);

      text_bg.anchors_.offset(text, -3, 3, -3, 1);
      text_bg.margins_.set_top(ItemRef::reference(hidden, kUnitSize, -0.03));
      text_bg.color_ = text_bg.addExpr(new ColorWhenFocused(focus));
      text_bg.color_.disableCaching();

      edit.anchors_.fill(text);
      edit.margins_.set_right(ItemRef::scale(edit,
                                             kPropertyCursorWidth,
                                             -1.0));
      edit.visible_ = edit.
        addExpr(new ShowWhenFocused(focus, true));
      edit.color_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      edit.background_ =
        ColorRef::transparent(focus, kPropertyColorOnFocusBg);
      edit.cursorColor_ = edit.
        addExpr(style_color_ref(view, &AppStyle::cursor_, 1.0));
      edit.font_ = text.font_;
      edit.fontSize_ = text.fontSize_;

      edit.selectionBg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::bg_edit_selected_, 1.0));
      edit.selectionFg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_edit_selected_, 1.0));

      ok = connect(&edit, SIGNAL(editingFinished(const QString &)),
                   &view, SLOT(update_wi_title(const QString &)));
      YAE_ASSERT(ok);

      Text & note = row.addNew<Text>("note");
      note.font_ = style.font_;
      note.font_.setWeight(62);
      note.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      note.anchors_.top_ = ItemRef::reference(text_bg, kPropertyBottom);
      note.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      note.elide_ = Qt::ElideNone;
      note.text_ = TVarRef::constant(TVar("optional, exact title or a regex"));
      note.color_ = note.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.8));
      note.background_ = note.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    }

    Item & r3 = body.addNew<Item>("r3");
    r3.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    r3.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
    r3.anchors_.top_ = ItemRef::reference(r2, kPropertyBottom);
    r3.height_ = ItemRef::reference(hidden, kUnitSize, 1.0);

    // layout the program description/regex row:
    {
      // shortcut:
      Item & row = r3;

      Text & label = row.addNew<Text>("label");
      label.font_ = style.font_;
      label.font_.setWeight(62);
      label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
      label.anchors_.bottom_ = label.
        addExpr(new RoundUp(row, kPropertyVCenter));
      label.anchors_.right_ = ItemRef::reference(c1, kPropertyRight);
      label.elide_ = Qt::ElideNone;
      label.text_ = TVarRef::constant(TVar("Program Description"));
      label.color_ = label.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      label.background_ = label.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

      Rectangle & text_bg = row.
        addNew<Rectangle>("text_bg");

      Text & text = row.
        addNew<Text>("text");

      TextInput & edit = row.
        addNew<TextInput>("edit");

      TextInputProxy & focus = row.
        add(new TextInputProxy("focus_desc_rx", text, edit));

      focus.anchors_.fill(text_bg);
      focus.bgNoFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.5));
      focus.bgOnFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      focus.copyViewToEdit_ = BoolRef::constant(true);
      focus.editingFinishedOnFocusOut_ = BoolRef::constant(true);

      ItemFocus::singleton().
        setFocusable(view, focus, "wishlist_ui", 2);

      text.anchors_.bottom_ = ItemRef::reference(label, kPropertyBottom);
      text.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      text.anchors_.right_ = ItemRef::reference(c3, kPropertyRight);
      text.visible_ = text.addExpr(new ShowWhenFocused(focus, false));
      text.color_ = text.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      text.background_ = text.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      text.text_ = text.addExpr(new GetWishlistItemDescription(view));
      text.font_ = style.font_;
      text.font_.setWeight(62);
      text.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);

      text_bg.anchors_.offset(text, -3, 3, -3, 1);
      text_bg.margins_.set_top(ItemRef::reference(hidden, kUnitSize, -0.03));
      text_bg.color_ = text_bg.addExpr(new ColorWhenFocused(focus));
      text_bg.color_.disableCaching();

      edit.anchors_.fill(text);
      edit.margins_.set_right(ItemRef::scale(edit,
                                             kPropertyCursorWidth,
                                             -1.0));
      edit.visible_ = edit.
        addExpr(new ShowWhenFocused(focus, true));
      edit.color_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      edit.background_ =
        ColorRef::transparent(focus, kPropertyColorOnFocusBg);
      edit.cursorColor_ = edit.
        addExpr(style_color_ref(view, &AppStyle::cursor_, 1.0));
      edit.font_ = text.font_;
      edit.fontSize_ = text.fontSize_;

      edit.selectionBg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::bg_edit_selected_, 1.0));
      edit.selectionFg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_edit_selected_, 1.0));

      ok = connect(&edit, SIGNAL(editingFinished(const QString &)),
                   &view, SLOT(update_wi_desc(const QString &)));
      YAE_ASSERT(ok);

      Text & note = row.addNew<Text>("note");
      note.font_ = style.font_;
      note.font_.setWeight(62);
      note.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      note.anchors_.top_ = ItemRef::reference(text_bg, kPropertyBottom);
      note.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      note.elide_ = Qt::ElideNone;
      note.text_ = TVarRef::constant
        (TVar("optional, exact description or a regex"));
      note.color_ = note.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.8));
      note.background_ = note.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    }

    Item & r4 = body.addNew<Item>("r4");
    r4.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    r4.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
    r4.anchors_.top_ = ItemRef::reference(r3, kPropertyBottom);
    r4.height_ = ItemRef::reference(hidden, kUnitSize, 1.0);

    // layout the time span row:
    {
      // shortcut:
      Item & row = r4;

      Text & label = row.addNew<Text>("label");
      label.font_ = style.font_;
      label.font_.setWeight(62);
      label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
      label.anchors_.bottom_ = label.
        addExpr(new RoundUp(row, kPropertyVCenter));
      label.anchors_.right_ = ItemRef::reference(c1, kPropertyRight);
      label.elide_ = Qt::ElideNone;
      label.text_ = TVarRef::constant(TVar("Program(s) Time Span"));
      label.color_ = label.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      label.background_ = label.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

      Rectangle & text_t0_bg = row.
        addNew<Rectangle>("text_t0_bg");

      Text & text_t0 = row.
        addNew<Text>("text_t0");

      TextInput & edit_t0 = row.
        addNew<TextInput>("edit_t0");

      TextInputProxy & focus_t0 = row.
        add(new TextInputProxy("focus_t0", text_t0, edit_t0));

      focus_t0.anchors_.fill(text_t0_bg);
      focus_t0.bgNoFocus_ = focus_t0.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.5));
      focus_t0.bgOnFocus_ = focus_t0.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      focus_t0.copyViewToEdit_ = BoolRef::constant(true);
      focus_t0.editingFinishedOnFocusOut_ = BoolRef::constant(true);

      ItemFocus::singleton().
        setFocusable(view, focus_t0, "wishlist_ui", 3);

      text_t0_bg.anchors_.left_ =
        ItemRef::offset(text_t0, kPropertyLeft, -3);
      text_t0_bg.anchors_.top_ =
        ItemRef::offset(text_t0, kPropertyTop, -3);
      text_t0_bg.anchors_.bottom_ =
        ItemRef::offset(text_t0, kPropertyBottom, 1);
      text_t0_bg.width_ =
        ItemRef::reference(hidden, kUnitSize, 1.475, 3);
      text_t0_bg.margins_.
        set_top(ItemRef::reference(hidden, kUnitSize, -0.03));
      text_t0_bg.color_ = text_t0_bg.addExpr(new ColorWhenFocused(focus_t0));
      text_t0_bg.color_.disableCaching();

      text_t0.anchors_.bottom_ = ItemRef::reference(label, kPropertyBottom);
      text_t0.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      text_t0.width_ = ItemRef::reference(hidden, kUnitSize, 1.475);
      text_t0.visible_ = text_t0.addExpr(new ShowWhenFocused(focus_t0, false));
      text_t0.color_ = text_t0.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      text_t0.background_ = text_t0.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      text_t0.text_ = text_t0.addExpr(new GetWishlistItemStart(view));
      text_t0.font_ = style.font_;
      text_t0.font_.setWeight(62);
      text_t0.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);

      edit_t0.anchors_.fill(text_t0);
      edit_t0.margins_.
        set_right(ItemRef::scale(edit_t0, kPropertyCursorWidth, -1.0));
      edit_t0.visible_ = edit_t0.
        addExpr(new ShowWhenFocused(focus_t0, true));
      edit_t0.color_ = edit_t0.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      edit_t0.background_ =
        ColorRef::transparent(focus_t0, kPropertyColorOnFocusBg);
      edit_t0.cursorColor_ = edit_t0.
        addExpr(style_color_ref(view, &AppStyle::cursor_, 1.0));
      edit_t0.font_ = text_t0.font_;
      edit_t0.fontSize_ = text_t0.fontSize_;

      edit_t0.selectionBg_ = edit_t0.
        addExpr(style_color_ref(view, &AppStyle::bg_edit_selected_, 1.0));
      edit_t0.selectionFg_ = edit_t0.
        addExpr(style_color_ref(view, &AppStyle::fg_edit_selected_, 1.0));

      ok = connect(&edit_t0, SIGNAL(editingFinished(const QString &)),
                   &view, SLOT(update_wi_time_start(const QString &)));
      YAE_ASSERT(ok);

      Rectangle & text_t1_bg = row.
        addNew<Rectangle>("text_t1_bg");

      Text & text_t1 = row.
        addNew<Text>("text_t1");

      TextInput & edit_t1 = row.
        addNew<TextInput>("edit_t1");

      TextInputProxy & focus_t1 = row.
        add(new TextInputProxy("focus_t1", text_t1, edit_t1));

      focus_t1.anchors_.fill(text_t1_bg);
      focus_t1.bgNoFocus_ = focus_t1.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.5));
      focus_t1.bgOnFocus_ = focus_t1.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      focus_t1.copyViewToEdit_ = BoolRef::constant(true);
      focus_t1.editingFinishedOnFocusOut_ = BoolRef::constant(true);

      ItemFocus::singleton().
        setFocusable(view, focus_t1, "wishlist_ui", 4);

      text_t1_bg.anchors_.left_ = text_t1_bg.
        addExpr(new RoundUp(text_t0_bg, kPropertyRight));
      text_t1_bg.anchors_.top_ =
        ItemRef::reference(text_t0_bg, kPropertyTop);
      text_t1_bg.anchors_.bottom_ =
        ItemRef::reference(text_t0_bg, kPropertyBottom);
      text_t1_bg.width_ =
        ItemRef::reference(hidden, kUnitSize, 1.475, 3);
      text_t1_bg.margins_.
        set_left(text_t1_bg.addExpr(new RoundUp(hidden, kUnitSize, 0.05)));
      text_t1_bg.color_ = text_t1_bg.addExpr(new ColorWhenFocused(focus_t1));
      text_t1_bg.color_.disableCaching();

      text_t1.anchors_.bottom_ = ItemRef::reference(label, kPropertyBottom);
      text_t1.anchors_.left_ = ItemRef::offset(text_t1_bg, kPropertyLeft, 3);
      text_t1.width_ = ItemRef::reference(hidden, kUnitSize, 1.475);
      text_t1.visible_ = text_t1.addExpr(new ShowWhenFocused(focus_t1, false));
      text_t1.color_ = text_t1.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      text_t1.background_ = text_t1.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      text_t1.text_ = text_t1.addExpr(new GetWishlistItemEnd(view));
      text_t1.font_ = style.font_;
      text_t1.font_.setWeight(62);
      text_t1.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);

      edit_t1.anchors_.fill(text_t1);
      edit_t1.margins_.
        set_right(ItemRef::scale(edit_t1, kPropertyCursorWidth, -1.0));
      edit_t1.visible_ = edit_t1.
        addExpr(new ShowWhenFocused(focus_t1, true));
      edit_t1.color_ = edit_t1.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      edit_t1.background_ =
        ColorRef::transparent(focus_t1, kPropertyColorOnFocusBg);
      edit_t1.cursorColor_ = edit_t1.
        addExpr(style_color_ref(view, &AppStyle::cursor_, 1.0));
      edit_t1.font_ = text_t1.font_;
      edit_t1.fontSize_ = text_t1.fontSize_;

      edit_t1.selectionBg_ = edit_t1.
        addExpr(style_color_ref(view, &AppStyle::bg_edit_selected_, 1.0));
      edit_t1.selectionFg_ = edit_t1.
        addExpr(style_color_ref(view, &AppStyle::fg_edit_selected_, 1.0));

      ok = connect(&edit_t1, SIGNAL(editingFinished(const QString &)),
                   &view, SLOT(update_wi_time_end(const QString &)));
      YAE_ASSERT(ok);

      Text & note = row.addNew<Text>("note");
      note.font_ = style.font_;
      note.font_.setWeight(62);
      note.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      note.anchors_.top_ = ItemRef::reference(text_t0_bg, kPropertyBottom);
      note.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      note.elide_ = Qt::ElideNone;
      note.text_ = TVarRef::constant
        (TVar("optional, 24h hh:mm - hh:mm, end time can be greater than 24"));
      note.color_ = note.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.8));
      note.background_ = note.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    }

    Item & r5 = body.addNew<Item>("r5");
    r5.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    r5.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
    r5.anchors_.top_ = ItemRef::reference(r4, kPropertyBottom);
    r5.height_ = ItemRef::reference(hidden, kUnitSize, 1.0);

    // layout the weekdays row:
    {
      // shortcut:
      Item & row = r5;

      Text & label = row.addNew<Text>("label");
      label.font_ = style.font_;
      label.font_.setWeight(62);
      label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
      label.anchors_.bottom_ = label.
        addExpr(new RoundUp(row, kPropertyVCenter));
      label.anchors_.right_ = ItemRef::reference(c1, kPropertyRight);
      label.elide_ = Qt::ElideNone;
      label.text_ = TVarRef::constant(TVar("Weekdays"));
      label.color_ = label.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      label.background_ = label.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

      Item * prev = &c2;
      for (uint8_t i = 0; i < 7; i++)
      {
        uint8_t j = (i + 1) % 7;
        uint16_t wday = 1 << j;
        const char * name = kWeekdays[j];

        Rectangle & btn = row.addNew<Rectangle>(name);
        btn.margins_.
          set_top(ItemRef::reference(hidden, kUnitSize, -0.03));
        btn.anchors_.top_ =
          ItemRef::offset(label, kPropertyTop, -3);
        btn.anchors_.bottom_ =
          ItemRef::offset(label, kPropertyBottom, 1);
        btn.anchors_.left_ =
          ItemRef::offset(*prev, kPropertyRight, i ? 0 : -3);
        btn.width_ = ItemRef::reference(hidden, kUnitSize, 0.9667, 1.667);
        btn.color_ = btn.
          addExpr(new WishlistWeekdayBtnColor(view, wday));

        if (i > 0)
        {
          btn.margins_.
            set_left(btn.addExpr(new RoundUp(hidden, kUnitSize, 0.05)));
        }

        Text & text = btn.addNew<Text>("text");
        text.anchors_.bottom_ = ItemRef::reference(label, kPropertyBottom);
        text.anchors_.hcenter_ = ItemRef::reference(btn, kPropertyHCenter);
        text.color_ = text.
          addExpr(new WishlistWeekdayTxtColor(view, wday));
        text.background_ = text.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
        text.text_ = TVarRef::constant(TVar(name));
        text.font_ = style.font_;
        text.font_.setWeight(62);
        text.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);

        WishlistWeekdayToggle & btn_ia =
          btn.add(new WishlistWeekdayToggle("toggle", view, wday));
        btn_ia.anchors_.fill(btn);

        prev = &btn;
      }

      Text & note = row.addNew<Text>("note");
      note.font_ = style.font_;
      note.font_.setWeight(62);
      note.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      note.anchors_.top_ = ItemRef::offset(row, kPropertyVCenter, 1);
      note.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      note.elide_ = Qt::ElideNone;
      note.text_ = TVarRef::constant(TVar("optional"));
      note.color_ = note.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.8));
      note.background_ = note.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    }

    Item & r6 = body.addNew<Item>("r6");
    r6.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    r6.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
    r6.anchors_.top_ = ItemRef::reference(r5, kPropertyBottom);
    r6.height_ = ItemRef::reference(hidden, kUnitSize, 1.0);

    // layout the exact date row:
    {
      // shortcut:
      Item & row = r6;

      Text & label = row.addNew<Text>("label");
      label.font_ = style.font_;
      label.font_.setWeight(62);
      label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
      label.anchors_.bottom_ = label.
        addExpr(new RoundUp(row, kPropertyVCenter));
      label.anchors_.right_ = ItemRef::reference(c1, kPropertyRight);
      label.elide_ = Qt::ElideNone;
      label.text_ = TVarRef::constant(TVar("Exact Date"));
      label.color_ = label.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      label.background_ = label.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

      Rectangle & text_bg = row.
        addNew<Rectangle>("text_bg");

      Text & text = row.
        addNew<Text>("text");

      TextInput & edit = row.
        addNew<TextInput>("edit");

      TextInputProxy & focus = row.
        add(new TextInputProxy("focus_date", text, edit));

      focus.anchors_.fill(text_bg);
      focus.bgNoFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.5));
      focus.bgOnFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      focus.copyViewToEdit_ = BoolRef::constant(true);
      focus.editingFinishedOnFocusOut_ = BoolRef::constant(true);

      ItemFocus::singleton().
        setFocusable(view, focus, "wishlist_ui", 5);

      text.anchors_.bottom_ = ItemRef::reference(label, kPropertyBottom);
      text.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      text.width_ = ItemRef::reference(hidden, kUnitSize, 3);
      text.visible_ = text.addExpr(new ShowWhenFocused(focus, false));
      text.color_ = text.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      text.background_ = text.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      text.text_ = text.addExpr(new GetWishlistItemDate(view));
      text.font_ = style.font_;
      text.font_.setWeight(62);
      text.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);

      text_bg.anchors_.offset(text, -3, 3, -3, 1);
      text_bg.margins_.set_top(ItemRef::reference(hidden, kUnitSize, -0.03));
      text_bg.color_ = text_bg.addExpr(new ColorWhenFocused(focus));
      text_bg.color_.disableCaching();

      edit.anchors_.fill(text);
      edit.margins_.
        set_right(ItemRef::scale(edit, kPropertyCursorWidth, -1.0));
      edit.visible_ = edit.
        addExpr(new ShowWhenFocused(focus, true));
      edit.color_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      edit.background_ =
        ColorRef::transparent(focus, kPropertyColorOnFocusBg);
      edit.cursorColor_ = edit.
        addExpr(style_color_ref(view, &AppStyle::cursor_, 1.0));
      edit.font_ = text.font_;
      edit.fontSize_ = text.fontSize_;

      edit.selectionBg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::bg_edit_selected_, 1.0));
      edit.selectionFg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_edit_selected_, 1.0));

      ok = connect(&edit, SIGNAL(editingFinished(const QString &)),
                   &view, SLOT(update_wi_date(const QString &)));
      YAE_ASSERT(ok);

      Text & note = row.addNew<Text>("note");
      note.font_ = style.font_;
      note.font_.setWeight(62);
      note.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      note.anchors_.top_ = ItemRef::reference(text_bg, kPropertyBottom);
      note.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      note.elide_ = Qt::ElideNone;
      note.text_ = TVarRef::constant(TVar("optional, YYYY/MM/DD format"));
      note.color_ = note.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.8));
      note.background_ = note.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    }

    Item & r7 = body.addNew<Item>("r7");
    r7.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    r7.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
    r7.anchors_.top_ = ItemRef::reference(r6, kPropertyBottom);
    r7.height_ = ItemRef::reference(hidden, kUnitSize, 1.0);

    // layout the min duration row:
    {
      // shortcut:
      Item & row = r7;

      Text & label = row.addNew<Text>("label");
      label.font_ = style.font_;
      label.font_.setWeight(62);
      label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
      label.anchors_.bottom_ = label.
        addExpr(new RoundUp(row, kPropertyVCenter));
      label.anchors_.right_ = ItemRef::reference(c1, kPropertyRight);
      label.elide_ = Qt::ElideNone;
      label.text_ = TVarRef::constant(TVar("Min. Duration"));
      label.color_ = label.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      label.background_ = label.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

      Rectangle & text_bg = row.
        addNew<Rectangle>("text_bg");

      Text & text = row.
        addNew<Text>("text");

      TextInput & edit = row.
        addNew<TextInput>("edit");

      TextInputProxy & focus = row.
        add(new TextInputProxy("focus_min_duration", text, edit));

      focus.anchors_.fill(text_bg);
      focus.bgNoFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.5));
      focus.bgOnFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      focus.copyViewToEdit_ = BoolRef::constant(true);
      focus.editingFinishedOnFocusOut_ = BoolRef::constant(true);

      ItemFocus::singleton().
        setFocusable(view, focus, "wishlist_ui", 6);

      text.anchors_.bottom_ = ItemRef::reference(label, kPropertyBottom);
      text.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      text.width_ = ItemRef::reference(hidden, kUnitSize, 3);
      text.visible_ = text.addExpr(new ShowWhenFocused(focus, false));
      text.color_ = text.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      text.background_ = text.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      text.text_ = text.addExpr(new GetWishlistItemMinDuration(view));
      text.font_ = style.font_;
      text.font_.setWeight(62);
      text.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);

      text_bg.anchors_.offset(text, -3, 3, -3, 1);
      text_bg.margins_.set_top(ItemRef::reference(hidden, kUnitSize, -0.03));
      text_bg.color_ = text_bg.addExpr(new ColorWhenFocused(focus));
      text_bg.color_.disableCaching();

      edit.anchors_.fill(text);
      edit.margins_.
        set_right(ItemRef::scale(edit, kPropertyCursorWidth, -1.0));
      edit.visible_ = edit.
        addExpr(new ShowWhenFocused(focus, true));
      edit.color_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      edit.background_ =
        ColorRef::transparent(focus, kPropertyColorOnFocusBg);
      edit.cursorColor_ = edit.
        addExpr(style_color_ref(view, &AppStyle::cursor_, 1.0));
      edit.font_ = text.font_;
      edit.fontSize_ = text.fontSize_;

      edit.selectionBg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::bg_edit_selected_, 1.0));
      edit.selectionFg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_edit_selected_, 1.0));

      ok = connect(&edit, SIGNAL(editingFinished(const QString &)),
                   &view, SLOT(update_wi_min_minutes(const QString &)));
      YAE_ASSERT(ok);

      Text & note = row.addNew<Text>("note");
      note.font_ = style.font_;
      note.font_.setWeight(62);
      note.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      note.anchors_.top_ = ItemRef::reference(text_bg, kPropertyBottom);
      note.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      note.elide_ = Qt::ElideNone;
      note.text_ = TVarRef::constant
        (TVar("optional, minimum program duration in minutes"));
      note.color_ = note.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.8));
      note.background_ = note.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    }

    Item & r8 = body.addNew<Item>("r8");
    r8.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    r8.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
    r8.anchors_.top_ = ItemRef::reference(r7, kPropertyBottom);
    r8.height_ = ItemRef::reference(hidden, kUnitSize, 1.0);

    // layout the max duration row:
    {
      // shortcut:
      Item & row = r8;

      Text & label = row.addNew<Text>("label");
      label.font_ = style.font_;
      label.font_.setWeight(62);
      label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
      label.anchors_.bottom_ = label.
        addExpr(new RoundUp(row, kPropertyVCenter));
      label.anchors_.right_ = ItemRef::reference(c1, kPropertyRight);
      label.elide_ = Qt::ElideNone;
      label.text_ = TVarRef::constant(TVar("Max. Duration"));
      label.color_ = label.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      label.background_ = label.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

      Rectangle & text_bg = row.
        addNew<Rectangle>("text_bg");

      Text & text = row.
        addNew<Text>("text");

      TextInput & edit = row.
        addNew<TextInput>("edit");

      TextInputProxy & focus = row.
        add(new TextInputProxy("focus_max_duration", text, edit));

      focus.anchors_.fill(text_bg);
      focus.bgNoFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.5));
      focus.bgOnFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      focus.copyViewToEdit_ = BoolRef::constant(true);
      focus.editingFinishedOnFocusOut_ = BoolRef::constant(true);

      ItemFocus::singleton().
        setFocusable(view, focus, "wishlist_ui", 7);

      text.anchors_.bottom_ = ItemRef::reference(label, kPropertyBottom);
      text.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      text.width_ = ItemRef::reference(hidden, kUnitSize, 3);
      text.visible_ = text.addExpr(new ShowWhenFocused(focus, false));
      text.color_ = text.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      text.background_ = text.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      text.text_ = text.addExpr(new GetWishlistItemMaxDuration(view));
      text.font_ = style.font_;
      text.font_.setWeight(62);
      text.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);

      text_bg.anchors_.offset(text, -3, 3, -3, 1);
      text_bg.margins_.set_top(ItemRef::reference(hidden, kUnitSize, -0.03));
      text_bg.color_ = text_bg.addExpr(new ColorWhenFocused(focus));
      text_bg.color_.disableCaching();

      edit.anchors_.fill(text);
      edit.margins_.
        set_right(ItemRef::scale(edit, kPropertyCursorWidth, -1.0));
      edit.visible_ = edit.
        addExpr(new ShowWhenFocused(focus, true));
      edit.color_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      edit.background_ =
        ColorRef::transparent(focus, kPropertyColorOnFocusBg);
      edit.cursorColor_ = edit.
        addExpr(style_color_ref(view, &AppStyle::cursor_, 1.0));
      edit.font_ = text.font_;
      edit.fontSize_ = text.fontSize_;

      edit.selectionBg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::bg_edit_selected_, 1.0));
      edit.selectionFg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_edit_selected_, 1.0));

      ok = connect(&edit, SIGNAL(editingFinished(const QString &)),
                   &view, SLOT(update_wi_max_minutes(const QString &)));
      YAE_ASSERT(ok);

      Text & note = row.addNew<Text>("note");
      note.font_ = style.font_;
      note.font_.setWeight(62);
      note.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      note.anchors_.top_ = ItemRef::reference(text_bg, kPropertyBottom);
      note.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      note.elide_ = Qt::ElideNone;
      note.text_ = TVarRef::constant
        (TVar("optional, maximum program duration in minutes"));
      note.color_ = note.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.8));
      note.background_ = note.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    }

    Item & r9 = body.addNew<Item>("r9");
    r9.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    r9.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
    r9.anchors_.top_ = ItemRef::reference(r8, kPropertyBottom);
    r9.height_ = ItemRef::reference(hidden, kUnitSize, 1.0);

    // layout the max recordings row:
    {
      // shortcut:
      Item & row = r9;

      Text & label = row.addNew<Text>("label");
      label.font_ = style.font_;
      label.font_.setWeight(62);
      label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
      label.anchors_.bottom_ = label.
        addExpr(new RoundUp(row, kPropertyVCenter));
      label.anchors_.right_ = ItemRef::reference(c1, kPropertyRight);
      label.elide_ = Qt::ElideNone;
      label.text_ = TVarRef::constant(TVar("Max. Recordings"));
      label.color_ = label.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      label.background_ = label.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

      Rectangle & text_bg = row.
        addNew<Rectangle>("text_bg");

      Text & text = row.
        addNew<Text>("text");

      TextInput & edit = row.
        addNew<TextInput>("edit");

      TextInputProxy & focus = row.
        add(new TextInputProxy("focus_max_rec", text, edit));

      focus.anchors_.fill(text_bg);
      focus.bgNoFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.5));
      focus.bgOnFocus_ = focus.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      focus.copyViewToEdit_ = BoolRef::constant(true);
      focus.editingFinishedOnFocusOut_ = BoolRef::constant(true);

      ItemFocus::singleton().
        setFocusable(view, focus, "wishlist_ui", 8);

      text.anchors_.bottom_ = ItemRef::reference(label, kPropertyBottom);
      text.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      text.width_ = ItemRef::reference(hidden, kUnitSize, 3);
      text.visible_ = text.addExpr(new ShowWhenFocused(focus, false));
      text.color_ = text.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      text.background_ = text.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      text.text_ = text.addExpr(new GetWishlistItemMax(view));
      text.font_ = style.font_;
      text.font_.setWeight(62);
      text.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);

      text_bg.anchors_.offset(text, -3, 3, -3, 1);
      text_bg.margins_.set_top(ItemRef::reference(hidden, kUnitSize, -0.03));
      text_bg.color_ = text_bg.addExpr(new ColorWhenFocused(focus));
      text_bg.color_.disableCaching();

      edit.anchors_.fill(text);
      edit.margins_.
        set_right(ItemRef::scale(edit, kPropertyCursorWidth, -1.0));
      edit.visible_ = edit.
        addExpr(new ShowWhenFocused(focus, true));
      edit.color_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      edit.background_ =
        ColorRef::transparent(focus, kPropertyColorOnFocusBg);
      edit.cursorColor_ = edit.
        addExpr(style_color_ref(view, &AppStyle::cursor_, 1.0));
      edit.font_ = text.font_;
      edit.fontSize_ = text.fontSize_;

      edit.selectionBg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::bg_edit_selected_, 1.0));
      edit.selectionFg_ = edit.
        addExpr(style_color_ref(view, &AppStyle::fg_edit_selected_, 1.0));

      ok = connect(&edit, SIGNAL(editingFinished(const QString &)),
                   &view, SLOT(update_wi_max(const QString &)));
      YAE_ASSERT(ok);

      Text & note = row.addNew<Text>("note");
      note.font_ = style.font_;
      note.font_.setWeight(62);
      note.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      note.anchors_.top_ = ItemRef::reference(text_bg, kPropertyBottom);
      note.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      note.elide_ = Qt::ElideNone;
      note.text_ = TVarRef::constant(TVar("optional"));
      note.color_ = note.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.8));
      note.background_ = note.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    }

    Item & r10 = body.addNew<Item>("r10");
    r10.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    r10.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
    r10.anchors_.top_ = ItemRef::reference(r9, kPropertyBottom);
    r10.height_ = ItemRef::reference(hidden, kUnitSize, 1.0);

    // layout the skip duplicates row:
    {
      // shortcut:
      Item & row = r10;

      Text & label = row.addNew<Text>("label");
      label.font_ = style.font_;
      label.font_.setWeight(62);
      label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
      label.anchors_.bottom_ = label.
        addExpr(new RoundUp(row, kPropertyVCenter));
      label.anchors_.right_ = ItemRef::reference(c1, kPropertyRight);
      label.elide_ = Qt::ElideNone;
      label.text_ = TVarRef::constant(TVar("Skip Duplicates"));
      label.color_ = label.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      label.background_ = label.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

      CheckboxItem & cbox = row.add(new CheckboxItem("cbox", view));
      cbox.anchors_.bottom_ = ItemRef::reference(label, kPropertyBottom);
      cbox.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      cbox.height_ = cbox.addExpr(new OddRoundUp(label, kPropertyHeight));
      cbox.width_ = cbox.height_;
      cbox.checked_ = cbox.addExpr(new GetWishlistItemSkipDuplicates(view));
      cbox.on_toggle_.reset(new OnToggleSkipDuplicates(view));

      Text & note = row.addNew<Text>("note");
      note.font_ = style.font_;
      note.font_.setWeight(62);
      note.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      note.anchors_.top_ = ItemRef::offset(cbox, kPropertyBottom, 1);
      note.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      note.elide_ = Qt::ElideNone;
      note.text_ = TVarRef::constant(TVar("optional"));
      note.color_ = note.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.8));
      note.background_ = note.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    }

    Item & r11 = body.addNew<Item>("r11");
    r11.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    r11.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
    r11.anchors_.top_ = ItemRef::reference(r10, kPropertyBottom);
    r11.height_ = ItemRef::reference(hidden, kUnitSize, 1.0);

    // layout the Enabled row:
    {
      // shortcut:
      Item & row = r11;

      Text & label = row.addNew<Text>("label");
      label.font_ = style.font_;
      label.font_.setWeight(62);
      label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
      label.anchors_.bottom_ = label.
        addExpr(new RoundUp(row, kPropertyVCenter));
      label.anchors_.right_ = ItemRef::reference(c1, kPropertyRight);
      label.elide_ = Qt::ElideNone;
      label.text_ = TVarRef::constant(TVar("Enabled"));
      label.color_ = label.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
      label.background_ = label.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));

      CheckboxItem & cbox = row.add(new CheckboxItem("cbox", view));
      cbox.anchors_.bottom_ = ItemRef::reference(label, kPropertyBottom);
      cbox.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      cbox.height_ = cbox.addExpr(new OddRoundUp(label, kPropertyHeight));
      cbox.width_ = cbox.height_;
      cbox.checked_ = cbox.addExpr(new GetWishlistItemEnabled(view));
      cbox.on_toggle_.reset(new OnToggleWishlistItemEnabled(view));

      Text & note = row.addNew<Text>("note");
      note.font_ = style.font_;
      note.font_.setWeight(62);
      note.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.23);
      note.anchors_.top_ = ItemRef::offset(cbox, kPropertyBottom, 1);
      note.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      note.elide_ = Qt::ElideNone;
      note.text_ = TVarRef::constant
        (TVar("optional, enable or disable this wishlist item"));
      note.color_ = note.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.8));
      note.background_ = note.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
    }

    Item & r12 = body.addNew<Item>("r12");
    r12.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
    r12.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
    r12.anchors_.top_ = ItemRef::reference(r11, kPropertyBottom);
    r12.height_ = ItemRef::reference(hidden, kUnitSize, 1.0);

    // layout the buttons:
    {
      // shortcut:
      Item & row = r12;

      RoundRect & bg_remove = body.addNew<RoundRect>("bg_remove");
      RoundRect & bg_save = body.addNew<RoundRect>("bg_save");

      Text & tx_remove = body.addNew<Text>("tx_remove");
      Text & tx_save = body.addNew<Text>("tx_save");

      tx_remove.anchors_.bottom_ = ItemRef::reference(row, kPropertyBottom);
      tx_remove.anchors_.left_ = ItemRef::reference(c3, kPropertyLeft);
      tx_remove.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.5));
      tx_remove.color_ = tx_remove.
        addExpr(style_color_ref(view, &AppStyle::cursor_fg_));
      tx_remove.background_ = tx_remove.
        addExpr(style_color_ref(view, &AppStyle::cursor_, 0.0));
      tx_remove.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.5);
      tx_remove.elide_ = Qt::ElideNone;
      tx_remove.setAttr("oneline", true);
      tx_remove.text_ = tx_remove.
        addExpr(new GetWishlistItemRemoveBtnText(view));

      bg_remove.anchors_.fill(tx_remove);
      bg_remove.margins_.
        set_left(ItemRef::reference(hidden, kUnitSize, -0.5));
      bg_remove.margins_.
        set_right(ItemRef::reference(hidden, kUnitSize, -0.5));
      bg_remove.margins_.
        set_top(ItemRef::reference(hidden, kUnitSize, -0.2));
      bg_remove.margins_.
        set_bottom(ItemRef::reference(hidden, kUnitSize, -0.2));
      bg_remove.color_ = bg_remove.
        addExpr(style_color_ref(view, &AppStyle::cursor_));
      bg_remove.background_ = bg_remove.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_, 0.0));
      bg_remove.radius_ = ItemRef::scale(bg_remove, kPropertyHeight, 0.1);

      RemoveWishlistItem & on_remove = bg_remove.
        add(new RemoveWishlistItem("on_remove", *this));
      on_remove.anchors_.fill(bg_remove);

      tx_save.anchors_.bottom_ = ItemRef::reference(row, kPropertyBottom);
      tx_save.anchors_.left_ = ItemRef::reference(tx_remove, kPropertyRight);
      tx_save.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 1.6));
      tx_save.text_ = TVarRef::constant(TVar("Save"));
      tx_save.color_ = tx_save.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.7));
      tx_save.background_ = tx_save.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_, 0.0));
      tx_save.fontSize_ = tx_remove.fontSize_;
      tx_save.elide_ = Qt::ElideNone;
      tx_save.setAttr("oneline", true);

      bg_save.anchors_.fill(tx_save);
      bg_save.margins_.set_left(ItemRef::reference(hidden, kUnitSize, -0.5));
      bg_save.margins_.set_right(ItemRef::reference(hidden, kUnitSize, -0.5));
      bg_save.margins_.set_top(ItemRef::reference(hidden, kUnitSize, -0.2));
      bg_save.margins_.set_bottom(ItemRef::reference(hidden, kUnitSize, -0.2));
      bg_save.color_ = bg_save.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      bg_save.background_ = bg_save.
        addExpr(style_color_ref(view, &AppStyle::bg_epg_, 0.0));
      bg_save.radius_ = ItemRef::scale(bg_save, kPropertyHeight, 0.1);

      SaveWishlistItem & on_save = bg_save.
        add(new SaveWishlistItem("on_save", *this));
      on_save.anchors_.fill(bg_save);
    }
  }

}
