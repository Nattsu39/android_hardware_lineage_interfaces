/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "ChargingBypass.h"

#include <android-base/file.h>
#include <android-base/strings.h>
#include <string>

namespace aidl::vendor::lineage::health {

class SysfsChargingNode : public ChargingNode {
  public:
    SysfsChargingNode(std::string path, std::string enabled, std::string disabled)
        : mPath(std::move(path)), mEnabled(std::move(enabled)), mDisabled(std::move(disabled)) {}

    bool read(bool* enabled) override {
        std::string value;
        if (!android::base::ReadFileToString(mPath, &value, true)) return false;
        value = android::base::Trim(value);
        if (value != mEnabled && value != mDisabled) return false;
        *enabled = value == mEnabled;
        return true;
    }

    bool write(bool enabled) override {
        return android::base::WriteStringToFile(enabled ? mEnabled : mDisabled, mPath, true);
    }

  private:
    const std::string mPath;
    const std::string mEnabled;
    const std::string mDisabled;
};

}  // namespace aidl::vendor::lineage::health
