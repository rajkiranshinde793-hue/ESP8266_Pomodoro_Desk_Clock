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
