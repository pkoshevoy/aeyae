// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Tue May 19 20:05:05 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SETTINGS_INTERFACE_H_
#define YAE_SETTINGS_INTERFACE_H_

// aeyae:
#include "yae_api.h"

// standard:
#include <cstddef>
#include <limits>


namespace yae
{

  //----------------------------------------------------------------
  // ISetting
  //
  struct YAE_API ISettingBase
  {

    //----------------------------------------------------------------
    // DataType
    //
    enum DataType
    {
      kGroup = 0,
      kBool,
      kEnum,
      kString,
      kInteger,
      kDouble
    };

    //----------------------------------------------------------------
    // HciRepresentation
    //
    enum HciRepresentation
    {
      kUnspecified = 0,
      kHidden,
      kGroupBox,
      kCheckBox,
      kTextLine,
      kTextBox,
      kSlider,
      kSpinBox,
      kComboBox,
      kRadioGroup
    };

    virtual ~ISettingBase() {}

    virtual ISettingBase * clone() const = 0;

    virtual DataType dataType() const = 0;
    virtual HciRepresentation hciHint() const = 0;

    //----------------------------------------------------------------
    // IAttributes
    //
    struct YAE_API IAttributes
    {
      virtual ~IAttributes() {}

      // setting ID, could be a GUID, but could be user friendly too:
      virtual const char * id() const = 0;
      virtual void setId(const char * id) = 0;

      // user friendly setting label, suitable for localization:
      virtual const char * label() const = 0;
      virtual void setLabel(const char * label) = 0;

      // units suffix (km, kg, lb, F, C, pixels, Kbit, etc...)
      virtual const char * units() const = 0;
      virtual void setUnits(const char * units) = 0;

      // tooltip
      virtual const char * tooltip() const = 0;
      virtual void setTooltip(const char * tooltip) = 0;

      // optional settings should be indicated as such in the UI,
      // rendering the default or preferred value, unless the value has been
      // explicitly set by the user (which should also be reflected in the UI)
      virtual bool isOptionalSetting() const = 0;
      virtual void setOptionalSetting(bool optional) = 0;

      virtual bool isOptionalSettingSpecified() const = 0;
      virtual void setOptionalSettingSpecified(bool specified) = 0;

      virtual const char * optionalSettingSummary() const = 0;
      virtual void setOptionalSettingSummary(const char * summary) = 0;
    };

    virtual const IAttributes & attributes() const = 0;
    virtual IAttributes & attributes() = 0;

    // shortcuts to attribute setters:
    inline ISettingBase & setId(const char * id)
    {
      attributes().setId(id);
      return *this;
    }

    inline ISettingBase & setLabel(const char * label)
    {
      attributes().setLabel(label);
      return *this;
    }

    inline ISettingBase & setUnits(const char * units)
    {
      attributes().setUnits(units);
      return *this;
    }

    inline ISettingBase & setTooltip(const char * tooltip)
    {
      attributes().setTooltip(tooltip);
      return *this;
    }

    inline ISettingBase & setOptional(bool optional)
    {
      attributes().setOptionalSetting(optional);
      return *this;
    }

    inline ISettingBase & setSpecified(bool specified)
    {
      attributes().setOptionalSettingSpecified(specified);
      return *this;
    }

    inline ISettingBase & setSummary(const char * summary)
    {
      attributes().setOptionalSettingSummary(summary);
      return *this;
    }

    // shortcuts to attribute accessors:
    inline const char * id() const
    { return attributes().id(); }

    inline const char * label() const
    { return attributes().label(); }

    inline const char * units() const
    { return attributes().units(); }

    inline const char * tooltip() const
    { return attributes().tooltip(); }

    inline bool optional() const
    { return attributes().isOptionalSetting(); }

    inline bool specified() const
    { return attributes().isOptionalSettingSpecified(); }

    inline const char * summary() const
    { return attributes().optionalSettingSummary(); }

    //----------------------------------------------------------------
    // IGroup
    //
    struct YAE_API IGroup
    {
      virtual ~IGroup() {}

      virtual std::size_t size() const = 0;
      virtual ISettingBase * setting(std::size_t index) const = 0;
      virtual void addSetting(ISettingBase * setting) = 0;
    };

    //----------------------------------------------------------------
    // IBool
    //
    struct YAE_API IBool
    {
      virtual ~IBool() {}

      virtual bool value() const = 0;
      virtual void setValue(bool v) = 0;
    };

    //----------------------------------------------------------------
    // IEnum
    //
    struct YAE_API IEnum
    {
      virtual ~IEnum() {}

      virtual std::size_t size() const = 0;
      virtual void addEnum(int value, const char * label) = 0;

      virtual int value(std::size_t index) const = 0;
      virtual const char * label(std::size_t index) const = 0;

      virtual std::size_t selectedIndex() const = 0;
      virtual bool setSelectedIndex(std::size_t i) = 0;
    };

    //----------------------------------------------------------------
    // IString
    //
    struct YAE_API IString
    {
      virtual ~IString() {}

      virtual const char * value() const = 0;
      virtual void setValue(const char * v) = 0;
    };

    //----------------------------------------------------------------
    // IScalar
    //
    template <typename TValue>
    struct YAE_API IScalar
    {
      typedef TValue value_type;

      virtual ~IScalar() {}

      virtual TValue value() const = 0;
      virtual bool setValue(TValue v) = 0;

      virtual void setValueMin(TValue v) = 0;
      virtual bool isValueMinConstrained() const = 0;
      virtual TValue valueMin() const = 0;

      virtual void setValueMax(TValue v) = 0;
      virtual bool isValueMaxConstrained() const = 0;
      virtual TValue valueMax() const = 0;

      virtual const char * valueMinLabel() const = 0;
      virtual void setValueMinLabel(const char * label) = 0;

      virtual const char * valueMaxLabel() const = 0;
      virtual void setValueMaxLabel(const char * label) = 0;

      // in case the setting should be presented as a list of values:
      virtual const TValue * possibleValuesArray() const = 0;
      virtual std::size_t possibleValuesArraySize() const = 0;
      virtual bool possibleValuesAreConstrained() const = 0;
      virtual void setPossibleValues(const TValue * values,
                                     std::size_t numValues,
                                     bool constrained = false) = 0;
    };

  };

  //----------------------------------------------------------------
  // ISetting
  //
  template <typename TTraits,
            ISettingBase::DataType settingDataType,
            ISettingBase::HciRepresentation settingHciHint>
  struct ISetting : public ISettingBase
  {
    enum { kDataType = settingDataType };
    enum { kDefaultHciRepresentation = settingHciHint };

    virtual DataType dataType() const
    { return settingDataType; }

    virtual HciRepresentation hciHint() const
    { return settingHciHint; }

    virtual const TTraits & traits() const = 0;
    virtual TTraits & traits() = 0;
  };

  //----------------------------------------------------------------
  // ISettingGroup
  //
  typedef ISetting<ISettingBase::IGroup,
                   ISettingBase::kGroup,
                   ISettingBase::kGroupBox> ISettingGroup;

  //----------------------------------------------------------------
  // ISettingBool
  //
  typedef ISetting<ISettingBase::IBool,
                   ISettingBase::kBool,
                   ISettingBase::kCheckBox> ISettingBool;

  //----------------------------------------------------------------
  // ISettingEnum
  //
  typedef ISetting<ISettingBase::IEnum,
                   ISettingBase::kEnum,
                   ISettingBase::kComboBox> ISettingEnum;

  //----------------------------------------------------------------
  // ISettingString
  //
  typedef ISetting<ISettingBase::IString,
                   ISettingBase::kString,
                   ISettingBase::kTextLine> ISettingString;

  //----------------------------------------------------------------
  // ISettingInt32
  //
  typedef ISetting<ISettingBase::IScalar<int>,
                   ISettingBase::kInteger,
                   ISettingBase::kSpinBox> ISettingInt32;

  //----------------------------------------------------------------
  // ISettingUInt32
  //
  typedef ISetting<ISettingBase::IScalar<unsigned int>,
                   ISettingBase::kInteger,
                   ISettingBase::kSpinBox> ISettingUInt32;

  //----------------------------------------------------------------
  // ISettingInt64
  //
  typedef ISetting<ISettingBase::IScalar<int64_t>,
                   ISettingBase::kInteger,
                   ISettingBase::kSpinBox> ISettingInt64;

  //----------------------------------------------------------------
  // ISettingUInt64
  //
  typedef ISetting<ISettingBase::IScalar<uint64_t>,
                   ISettingBase::kInteger,
                   ISettingBase::kSpinBox> ISettingUInt64;

  //----------------------------------------------------------------
  // ISettingDouble
  //
  typedef ISetting<ISettingBase::IScalar<double>,
                   ISettingBase::kDouble,
                   ISettingBase::kSpinBox> ISettingDouble;

}


#endif // YAE_SETTINGS_INTERFACE_H_
