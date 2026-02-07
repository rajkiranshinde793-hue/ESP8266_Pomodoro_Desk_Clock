## [v2.1] - 2026-02-03

### Added
- Google Sheets logging feature for session data tracking
- Real-time clock display on Pomodoro session screen
- Real-time clock display on Break session screen

### Changed
- Removed ESP light sleep mode
- Removed modem sleep mode
- Power management now handled using WiFi duty cycling strategy

### Notes
- Sleep removal was done to avoid instability and timing drift issues
- WiFi duty cycle provides more predictable power behavior

## [v2.2] - 2026-02-05

### Added
- DND (Do Not Disturb) automation integration via MacroDroid
- Phone automatically switches to DND mode when Pomodoro session starts
- Focus mode environment synchronization between device and mobile

### Improved
- Enhanced distraction control during work sessions
- System now extends focus control beyond hardware to user’s smartphone

### Notes
- DND trigger is controlled through WiFi-based event from Pomodoro device
- Requires MacroDroid automation configured on the user’s phone

## [v2.3] - 2026-02-06

### Fixed
- Resolved permanent high current (~80mA) issue caused by incomplete Wi-Fi shutdown sequence
- Eliminated background RF maintenance after DND webhook execution
- Eliminated Wi-Fi persistence after Google Sheets logging

### Root Cause
Multiple modules (DNDControl.cpp, GoogleLogger.cpp) independently enabled Wi-Fi using `WiFi.begin()` but did not guarantee a full radio shutdown sequence.  
As a result, the ESP8266 remained in `WIFI_STA` mode and continued background AP maintenance indefinitely.

### Technical Correction
- Added explicit RF shutdown sequence after HTTP operations:
  - `WiFi.disconnect(true)`
  - `WiFi.mode(WIFI_OFF)`
  - `WiFi.forceSleepBegin()`
  - `delay(1)`
- Ensured all Wi-Fi wake operations have a guaranteed cleanup path
- Corrected incomplete state transitions in DND state machine

### Power Optimization Impact
- Idle current reduced from ~80mA (persistent RF active state) to ~28–30mA
- Eliminated periodic background RF wake spikes (Gratuitous ARP / DTIM listening)
- Improved overall energy efficiency and system stability

### Architectural Improvement
- Established "Post-Flight Rule":
  Every Wi-Fi wake sequence must have a guaranteed shutdown path
- Identified need for centralized Wi-Fi ownership model for future refactoring

### Notes
This update transitions the firmware from opportunistic Wi-Fi usage to deterministic RF lifecycle management.
