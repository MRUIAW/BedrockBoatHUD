# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-09-30

### Added

- First stable client release for Minecraft Bedrock 1.26.51.01 and LeviLamina 26.51.x on Windows x64, tested with LeviLamina 26.51.5.
- Race (default), Classic, and Compact boat HUD layouts with speed, slip angle, acceleration, ping, and HUD render FPS.
- BoatHUD-style packed, mixed, and blue speed bars with colour sections and over-range flashing.
- Acceleration indicator, steering/throttle history, optional direction arrows, and forward/reverse input icons with keyboard and controller support.
- Smooth Hibiii-style camera assistance in first and third person, disabled by default.
- Native-cursor settings screen, persistent versioned JSON configuration, and optional vanilla HUD hiding, disabled by default.
- English and Simplified Chinese language files loaded through LeviLamina I18n.
- Per-session CSV telemetry and manually configured checkpoint comparisons, including circular tracks.
- English and Chinese READMEs, telemetry instructions, an agent maintenance guide, validated release packaging, and lip client package metadata.

### Changed

- Rebuilt and republished on 2026-10-01 as `v1.0.0-mc26.5x`; retain the `v1.0.0` tag and runtime version.
- Use the BoatHUD package display name/icon and publish only one freshly rebuilt `BedrockBoatHUD-client-windows-x64.zip`, keeping the existing 1.0.0 package URL; align preserved config/data paths with the actual install directory.
- Standardize future release titles as `v<mod-version>-mc<game-series>`, configured in `release-config.json`.
- Promote the accepted 0.2.22 development build to 1.0.0 without changing its driving behaviour.
- Use the package manifest as the release-version source and publish only the Windows x64 client variant.
- Keep the configuration schema at version 3; older progressive/custom speed-bar settings migrate to packed.

### Fixed

- Restore the native settings cursor without drift or game-view mouse capture.
- Correct camera yaw axis, angular units, boat heading offset, and frame-rate-independent smoothing.
- Centre the acceleration indicator and reproduce BoatHUD Extended's nested-box appearance.
- Keep speed bars inside their borders and restore mixed-mode blue-ice colour sections.
- Format Race acceleration values independently with two decimal places below magnitude 10 and one at or above 10.
- Prevent conflicting digital and analog input representations from lighting both arrows when only one direction is pressed.

[Unreleased]: https://github.com/MRUIAW/BedrockBoatHUD/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/MRUIAW/BedrockBoatHUD/releases/tag/v1.0.0
