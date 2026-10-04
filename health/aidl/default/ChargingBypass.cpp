/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ChargingBypass.h"

namespace aidl::vendor::lineage::health {

ChargingBypass::ChargingBypass(ChargingNode& input, ChargingNode& battery)
    : mInput(input), mBattery(battery) {}

bool ChargingBypass::writeChecked(ChargingNode& node, bool enabled) {
    bool actual;
    if (!node.read(&actual)) return false;
    if (actual == enabled) return true;
    return node.write(enabled) && node.read(&actual) && actual == enabled;
}

bool ChargingBypass::getChargingEnabled(bool* enabled) {
    std::lock_guard lock(mMutex);
    bool batteryEnabled;
    if (!mBattery.read(&batteryEnabled)) return false;
    if (!batteryEnabled && mNormalChargingEnabled.has_value()) {
        *enabled = *mNormalChargingEnabled;
        return true;
    }
    if (!mInput.read(enabled)) return false;
    mNormalChargingEnabled = *enabled;
    return true;
}

bool ChargingBypass::setChargingEnabled(bool enabled) {
    std::lock_guard lock(mMutex);
    bool batteryEnabled;
    if (!mBattery.read(&batteryEnabled)) return false;
    if (!batteryEnabled) {
        if (!writeChecked(mInput, true)) return false;
    } else if (!writeChecked(mInput, enabled)) {
        return false;
    }
    mNormalChargingEnabled = enabled;
    return true;
}

bool ChargingBypass::getBypassEnabled(bool* enabled) {
    std::lock_guard lock(mMutex);
    bool batteryEnabled, inputEnabled;
    if (!mBattery.read(&batteryEnabled)) return false;
    if (!batteryEnabled && (!mInput.read(&inputEnabled) || !inputEnabled)) return false;
    *enabled = !batteryEnabled;
    return true;
}

bool ChargingBypass::setBypassEnabled(bool enabled) {
    std::lock_guard lock(mMutex);
    bool batteryEnabled, inputEnabled;
    if (!mBattery.read(&batteryEnabled) || !mInput.read(&inputEnabled)) return false;
    if (enabled) {
        if (batteryEnabled) mNormalChargingEnabled = inputEnabled;
        // Stop battery charging first, then ensure the external input is usable.
        if (!writeChecked(mBattery, false)) return false;
        if (writeChecked(mInput, true)) return true;
        // Do not claim success when the input could not be enabled.
        if (batteryEnabled) writeChecked(mBattery, true);
        return false;
    }
    if (batteryEnabled) return true;
    // An unknown policy after a HAL restart must be refreshed by the caller.
    if (!mNormalChargingEnabled.has_value()) return false;
    if (!writeChecked(mInput, *mNormalChargingEnabled)) {
        writeChecked(mInput, true);
        return false;
    }
    if (writeChecked(mBattery, true)) return true;
    // Keep input power available if restoring battery charging failed.
    writeChecked(mInput, true);
    return false;
}

}  // namespace aidl::vendor::lineage::health
