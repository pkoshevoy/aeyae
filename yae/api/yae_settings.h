// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Tue May 19 23:14:09 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SETTINGS_H_
#define YAE_SETTINGS_H_

// standard C++ library:
#include <string>
#include <utility>
#include <vector>

// aeyae:
#include "yae_api.h"
#include "yae_settings_interface.h"


namespace yae
{

  namespace settings
  {

    //----------------------------------------------------------------
    // TAttributes
    //
    struct TAttributes : public ISettingBase::IAttributes
    {
      TAttributes():
        optional_(false),
        specified_(false)
      {}

      virtual const char * id() const
      { return id_.c_str(); }

      virtual void setId(const char * id)
      { id_.assign(id); }

      // user friendly setting label, suitable for localization:
      virtual const char * label() const
      { return label_.c_str(); }

      virtual void setLabel(const char * label)
      { label_.assign(label); }

      // units suffix (km, kg, lb, F, C, pixels, Kbit, etc...)
      virtual const char * units() const
      { return units_.c_str(); }

      virtual void setUnits(const char * units)
      { units_.assign(units); }

      // tooltip
      virtual const char * tooltip() const
      { return tooltip_.c_str(); }

      virtual void setTooltip(const char * tooltip)
      { tooltip_.assign(tooltip); }

      // optional settings should be indicated as such in the UI,
      // rendering the default or preferred value, unless the value has been
      // explicitly set by the user (which should also be reflected in the UI)
      virtual bool isOptionalSetting() const
      { return optional_; }

      virtual void setOptionalSetting(bool optional)
      { optional_ = optional; }

      virtual bool isOptionalSettingSpecified() const
      { return specified_; }

      virtual void setOptionalSettingSpecified(bool specified)
      { specified_ = specified; }

      virtual const char * optionalSettingSummary() const
      { return summary_.c_str(); }

      virtual void setOptionalSettingSummary(const char * summary)
      { summary_.assign(summary); }

    protected:
      std::string id_;
      std::string label_;
      std::string units_;
      std::string tooltip_;
      std::string summary_;
      bool optional_;
      bool specified_;
    };

    //----------------------------------------------------------------
    // TGroup
    //
    struct TGroup : public ISettingBase::IGroup
    {
      virtual std::size_t size() const
      { return settings_.size(); }

      virtual ISettingBase * setting(std::size_t index) const
      { return const_cast<TGroup *>(this)->settings_[index]; }

      virtual void addSetting(ISettingBase * setting)
      { settings_.push_back(setting); }

      std::vector<ISettingBase *> settings_;
    };

    //----------------------------------------------------------------
    // TBool
    //
    struct TBool : public ISettingBase::IBool
    {
      TBool():
        value_(false)
      {}

      virtual bool value() const
      { return value_; }

      virtual void setValue(bool v)
      { value_ = v; }

      bool value_;
    };

    //----------------------------------------------------------------
    // TEnum
    //
    struct TEnum : public ISettingBase::IEnum
    {
      TEnum():
        selected_(0)
      {}

      virtual std::size_t size() const
      { return enums_.size(); }

      virtual void addEnum(int value, const char * label)
      { enums_.push_back(std::make_pair(value, std::string(label))); }

      virtual int value(std::size_t index) const
      { return enums_[index].first; }

      virtual const char * label(std::size_t index) const
      { return enums_[index].second.c_str(); }

      virtual std::size_t selectedIndex() const
      { return selected_; }

      virtual bool setSelectedIndex(std::size_t i)
      {
        if (i >= enums_.size())
        {
          return false;
        }

        selected_ = i;
        return true;
      }

      std::vector<std::pair<int, std::string> > enums_;
      std::size_t selected_;
    };

    //----------------------------------------------------------------
    // TString
    //
    struct TString : public ISettingBase::IString
    {
      virtual const char * value() const
      { return value_.c_str(); }

      virtual void setValue(const char * v)
      { value_.assign(v); }

      std::string value_;
    };

    //----------------------------------------------------------------
    // TScalar
    //
    template <typename TValue>
    struct TScalar : public ISettingBase::IScalar<TValue>
    {
      TScalar(TValue v = TValue(0)):
        value_(v),
        valueMin_(std::numeric_limits<TValue>::min()),
        valueMax_(std::numeric_limits<TValue>::max()),
        valueMinConstrained_(false),
        valueMaxConstrained_(false),
        possibleConstrained_(false)
      {}

      virtual TValue value() const
      { return value_; }

      virtual bool setValue(TValue v)
      {
        if (possibleConstrained_)
        {
          typename std::vector<TValue>::const_iterator found =
            std::find(possible_.begin(), possible_.end(), v);

          if (found == possible_.end())
          {
            return false;
          }
        }

        bool ok = ((!valueMinConstrained_ || valueMin_ <= v) &&
                   (!valueMaxConstrained_ || v <= valueMax_));
        if (ok)
        {
          value_ = v;
        }

        return ok;
      }

      virtual void setMinValue(TValue v)
      {
        valueMin_ = v;
        valueMinConstrained_ = true;
      }

      virtual bool isMinValueConstrained() const
      { return valueMinConstrained_; }

      virtual TValue valueMin() const
      { return valueMin_; }

      virtual void setValueMax(TValue v)
      {
        valueMax_ = v;
        valueMaxConstrained_ = true;
      }

      virtual bool isValueMaxConstrained() const
      { return valueMaxConstrained_; }

      virtual TValue valueMax() const
      { return valueMax_; }

      virtual const char * valueMinLabel() const
      { return valueMinLabel_.c_str(); }

      virtual void setMinValueLabel(const char * label)
      { valueMinLabel_.assign(label); }

      virtual const char * valueMaxLabel() const
      { return valueMaxLabel_.c_str(); }

      virtual void setValueMaxLabel(const char * label)
      { valueMaxLabel_.assign(label); }

      virtual const TValue * possibleValuesArray() const
      { return possible_.empty() ? NULL : &possible_[0]; }

      virtual std::size_t possibleValuesArraySize() const
      { return possible_.size(); }

      virtual bool possibleValuesAreConstrained() const
      { return possibleConstrained_; }

      virtual void setPossibleValues(const TValue * values,
                                     std::size_t numValues,
                                     bool constrained = false)
      {
        possible_.assign(values, values + numValues);
        possibleConstrained_ = constrained;
      }

      TValue value_;
      TValue valueMin_;
      TValue valueMax_;
      bool valueMinConstrained_;
      bool valueMaxConstrained_;
      bool possibleConstrained_;
      std::string valueMinLabel_;
      std::string valueMaxLabel_;
      std::vector<TValue> possible_;
    };

    //----------------------------------------------------------------
    // TImplement
    //
    template <typename TInterface, typename TTraits>
    struct TImplement : public TInterface
    {
      typedef TImplement<TInterface, TTraits> TSelf;

      TImplement(const char * id = "",
                 const char * label = "",
                 const char * units = "",
                 const char * tooltip = "",
                 bool optional = false,
                 const char * summary = "")
      {
        attrs_.setId(id);
        attrs_.setLabel(label);
        attrs_.setUnits(units);
        attrs_.setTooltip(tooltip);
        attrs_.setOptionalSetting(optional);
        attrs_.setOptionalSettingSummary(summary);
      }

      virtual TSelf * clone() const
      { return new TSelf(*this); }

      virtual const TAttributes & attributes() const
      { return attrs_; }

      virtual TAttributes & attributes()
      { return attrs_; }

      virtual const TTraits & traits() const
      { return traits_; }

      virtual TTraits & traits()
      { return traits_; }

    protected:
      TAttributes attrs_;
      TTraits traits_;
    };

    //----------------------------------------------------------------
    // TImplementScalar
    //
    template <typename TInterface, typename TTraits>
    struct TImplementScalar : public TImplement<TInterface, TTraits>
    {
      typedef TImplement<TInterface, TTraits> TBase;
      typedef TImplementScalar<TInterface, TTraits> TSelf;

      virtual TSelf * clone() const
      { return new TSelf(*this); }

      virtual ISettingBase::HciRepresentation hciHint() const
      {
        return
          TBase::traits_.possibleValuesArraySize() ? ISettingBase::kComboBox :
          TBase::traits_.isValueMinConstrained() &&
          TBase::traits_.isValueMaxConstrained() ?
          ISettingBase::kSlider : ISettingBase::kSpinBox;
      }

      inline TSelf & setValue(typename TTraits::value_type v)
      {
        if (!TBase::attrs_.setValue(v))
        {
          YAE_ASSERT(false);
        }

        return *this;
      }
    };

    //----------------------------------------------------------------
    // TImplementReal
    //
    template <typename TInterface, typename TTraits>
    struct TImplementReal : public TImplementScalar<TInterface, TTraits>
    {
      typedef TImplementScalar<TInterface, TTraits> TBase;
      typedef TImplementReal<TInterface, TTraits> TSelf;

      virtual TSelf * clone() const
      { return new TSelf(*this); }

      TImplementReal(const char * id = "",
                     const char * label = "",
                     const char * units = "",
                     const char * tooltip = "",
                     bool optional = false,
                     const char * summary = ""):
        TBase(id, label, units, tooltip, optional, summary)
      {
        TBase::attrs_.setValueMin
          (-std::numeric_limits<typename TTraits::value_type>::max());
      }
    };
  }


  //----------------------------------------------------------------
  // TSettingGroup
  //
  typedef settings::TImplement<ISettingGroup, settings::TGroup> TSettingGroup;

  //----------------------------------------------------------------
  // TSettingBool
  //
  typedef settings::TImplement<ISettingBool, settings::TBool> TSettingBool;

  //----------------------------------------------------------------
  // TSettingEnum
  //
  typedef settings::TImplement<ISettingEnum, settings::TEnum> TSettingEnum;

  //----------------------------------------------------------------
  // TSettingString
  //
  typedef settings::TImplement<ISettingString,
                               settings::TString> TSettingString;

  //----------------------------------------------------------------
  // TSettingInt32
  //
  typedef settings::TImplementScalar<
    ISettingInt32, settings::TScalar<int> > TSettingInt32;

  //----------------------------------------------------------------
  // TSettingUInt32
  //
  typedef settings::TImplementScalar<
    ISettingUInt32, settings::TScalar<unsigned int> > TSettingUInt32;

  //----------------------------------------------------------------
  // TSettingInt64
  //
  typedef settings::TImplementScalar<
    ISettingInt64, settings::TScalar<int64_t> > TSettingInt64;

  //----------------------------------------------------------------
  // TSettingUInt64
  //
  typedef settings::TImplementScalar<
    ISettingUInt64, settings::TScalar<uint64_t> > TSettingUInt64;

  //----------------------------------------------------------------
  // TSettingDouble
  //
  typedef settings::TImplementReal<
    ISettingDouble, settings::TScalar<double> > TSettingDouble;

}


#endif // YAE_SETTINGS_H_
