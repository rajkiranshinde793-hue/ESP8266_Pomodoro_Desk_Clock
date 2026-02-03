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