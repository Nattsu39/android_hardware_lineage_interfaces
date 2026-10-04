# Independent battery bypass

The manual bypass interface is available only when the device explicitly sets
`lineage_health.charging_bypass_path` and supports the existing charging toggle.
The configured bypass node must accept `1` to permit battery charging and `0` to
inhibit it. The path must differ from `charging_control_charging_path`; do not
enable the feature based only on a guessed node name or the existing `BYPASS`
capability bit.

On NX809J, stock `ZteBatteryService` writes `battery_charging_enabled=0` for
charge separation. The existing Lineage charging policy uses `charging_enabled`.
The HAL coordinates both nodes without changing the stored limit or schedule:

1. Enter: inhibit battery charging, then enable the external input.
2. During the session: acknowledge normal policy changes in memory, keeping
   external input enabled. `getChargingEnabled()` returns that normal policy.
3. Exit: restore the latest normal input policy, then permit battery charging.

Every write is checked by reading the node back. A failed transition returns an
error, attempts to retain usable input power, and never reports an unconfirmed
bypass session as active. Constructors and capability queries do not write.
After a HAL restart, the normal policy must be refreshed before closing a stale
battery gate. The framework reconnects to the HAL and ends the previous session.

The framework owns wired power, minimum battery level, screen timeout and
shutdown handling. It does not persist an enabled session or override thermal
nodes. Driver acknowledgement confirms the control values; it does not measure
the physical power path or guarantee zero battery current.

`LineageChargingBypassTests` uses fake nodes behind the same `ChargingNode`
interface as the sysfs adapter. The tests run on the host and never access a
phone. Build the HAL, the Lineage platform library and Settings together for
deployment; replacing Settings alone does not install the feature.
