/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ChargingBypass.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace aidl::vendor::lineage::health {
namespace {

class FakeNode : public ChargingNode {
  public:
    FakeNode(std::string name, std::vector<std::string>& writes)
        : name(std::move(name)), writes(writes) {}
    bool read(bool* value) override {
        if (failRead) return false;
        *value = enabled;
        return true;
    }
    bool write(bool value) override {
        writes.push_back(name + (value ? "=1" : "=0"));
        if (failWrite) return false;
        if (!ignoreWrite) enabled = value;
        return true;
    }
    bool enabled = true;
    bool failRead = false;
    bool failWrite = false;
    bool ignoreWrite = false;

  private:
    std::string name;
    std::vector<std::string>& writes;
};

class ChargingBypassTest : public testing::Test {
  protected:
    std::vector<std::string> writes;
    FakeNode input{"input", writes};
    FakeNode battery{"battery", writes};
    ChargingBypass control{input, battery};
};

TEST_F(ChargingBypassTest, ConstructionAndQueriesNeverWrite) {
    bool enabled;
    EXPECT_TRUE(control.getChargingEnabled(&enabled));
    EXPECT_TRUE(enabled);
    EXPECT_TRUE(control.getBypassEnabled(&enabled));
    EXPECT_FALSE(enabled);
    EXPECT_TRUE(writes.empty());
}

TEST_F(ChargingBypassTest, BypassTemporarilyOverridesChargeLimit) {
    ASSERT_TRUE(control.setChargingEnabled(false));
    writes.clear();
    ASSERT_TRUE(control.setBypassEnabled(true));
    EXPECT_EQ(writes, (std::vector<std::string>{"battery=0", "input=1"}));
    bool enabled;
    EXPECT_TRUE(control.getChargingEnabled(&enabled));
    EXPECT_FALSE(enabled);
    EXPECT_TRUE(input.enabled);
    EXPECT_FALSE(battery.enabled);
    writes.clear();
    ASSERT_TRUE(control.setBypassEnabled(false));
    EXPECT_EQ(writes, (std::vector<std::string>{"input=0", "battery=1"}));
}

TEST_F(ChargingBypassTest, UsesLatestPolicyWhenSessionEnds) {
    ASSERT_TRUE(control.setBypassEnabled(true));
    writes.clear();
    ASSERT_TRUE(control.setChargingEnabled(false));
    EXPECT_TRUE(writes.empty());
    ASSERT_TRUE(control.setChargingEnabled(true));
    EXPECT_TRUE(writes.empty());
    ASSERT_TRUE(control.setBypassEnabled(false));
    EXPECT_TRUE(input.enabled);
    EXPECT_TRUE(battery.enabled);
}

TEST_F(ChargingBypassTest, FailingInputRollsBackBatteryDisable) {
    input.enabled = false;
    input.failWrite = true;
    EXPECT_FALSE(control.setBypassEnabled(true));
    EXPECT_TRUE(battery.enabled);
    EXPECT_EQ(writes, (std::vector<std::string>{"battery=0", "input=1", "battery=1"}));
}

TEST_F(ChargingBypassTest, FailingBatteryDisableDoesNotEnableInput) {
    input.enabled = false;
    battery.failWrite = true;
    EXPECT_FALSE(control.setBypassEnabled(true));
    EXPECT_FALSE(input.enabled);
    EXPECT_EQ(writes, (std::vector<std::string>{"battery=0"}));
}

TEST_F(ChargingBypassTest, DetectsIgnoredDriverWrite) {
    battery.ignoreWrite = true;
    EXPECT_FALSE(control.setBypassEnabled(true));
    bool enabled;
    ASSERT_TRUE(control.getBypassEnabled(&enabled));
    EXPECT_FALSE(enabled);
}

TEST_F(ChargingBypassTest, FailedExitKeepsInputAvailableAndCanBeRetried) {
    ASSERT_TRUE(control.setBypassEnabled(true));
    ASSERT_TRUE(control.setChargingEnabled(false));
    battery.failWrite = true;
    EXPECT_FALSE(control.setBypassEnabled(false));
    EXPECT_TRUE(input.enabled);
    EXPECT_FALSE(battery.enabled);
    battery.failWrite = false;
    EXPECT_TRUE(control.setBypassEnabled(false));
    EXPECT_FALSE(input.enabled);
    EXPECT_TRUE(battery.enabled);
}

TEST_F(ChargingBypassTest, ReadFailureDoesNotWrite) {
    battery.failRead = true;
    EXPECT_FALSE(control.setBypassEnabled(true));
    EXPECT_FALSE(control.setChargingEnabled(false));
    EXPECT_TRUE(writes.empty());
}

TEST_F(ChargingBypassTest, RestartRequiresPolicyRefreshBeforeRestoringBattery) {
    battery.enabled = false;
    EXPECT_FALSE(control.setBypassEnabled(false));
    EXPECT_TRUE(writes.empty());
    ASSERT_TRUE(control.setChargingEnabled(false));
    ASSERT_TRUE(control.setBypassEnabled(false));
    EXPECT_FALSE(input.enabled);
    EXPECT_TRUE(battery.enabled);
}

TEST_F(ChargingBypassTest, RepeatedRequestsDoNotWriteAgain) {
    ASSERT_TRUE(control.setBypassEnabled(true));
    writes.clear();
    ASSERT_TRUE(control.setBypassEnabled(true));
    EXPECT_TRUE(writes.empty());
    ASSERT_TRUE(control.setBypassEnabled(false));
    writes.clear();
    ASSERT_TRUE(control.setBypassEnabled(false));
    EXPECT_TRUE(writes.empty());
}

TEST_F(ChargingBypassTest, BlockedInputIsNotReportedAsWorkingBypass) {
    ASSERT_TRUE(control.setBypassEnabled(true));
    input.enabled = false;
    bool enabled;
    EXPECT_FALSE(control.getBypassEnabled(&enabled));
    ASSERT_TRUE(control.setChargingEnabled(false));
    ASSERT_TRUE(control.setBypassEnabled(false));
    EXPECT_FALSE(input.enabled);
    EXPECT_TRUE(battery.enabled);
}

}  // namespace
}  // namespace aidl::vendor::lineage::health
