/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <mutex>
#include <optional>

namespace aidl::vendor::lineage::health {

// Both production sysfs nodes and test nodes implement this interface.
class ChargingNode {
  public:
    virtual ~ChargingNode() = default;
    virtual bool read(bool* enabled) = 0;
    virtual bool write(bool enabled) = 0;
};

// Serializes the normal charging policy and a temporary bypass session.
// The input remains enabled during bypass; changes to the normal policy are
// remembered and applied before battery charging is restored on exit.
class ChargingBypass {
  public:
    ChargingBypass(ChargingNode& input, ChargingNode& battery);
    bool getChargingEnabled(bool* enabled);
    bool setChargingEnabled(bool enabled);
    bool getBypassEnabled(bool* enabled);
    bool setBypassEnabled(bool enabled);

  private:
    bool writeChecked(ChargingNode& node, bool enabled);
    ChargingNode& mInput;
    ChargingNode& mBattery;
    std::mutex mMutex;
    std::optional<bool> mNormalChargingEnabled;
};

}  // namespace aidl::vendor::lineage::health
