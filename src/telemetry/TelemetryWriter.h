// SPDX-License-Identifier: CC0-1.0
#pragma once

#include <filesystem>
#include <fstream>

#include "hud/HudSnapshot.h"

namespace ll::mod {
class NativeMod;
}

namespace boat_hud::telemetry {

/// Writes a buffered, versioned CSV stream for one driving session.
class TelemetryWriter {
public:
    /// Creates a writer that uses the owning mod's data directory and logger.
    /// @param mod Owning native mod.
    explicit TelemetryWriter(ll::mod::NativeMod& mod);

    TelemetryWriter(TelemetryWriter const&)            = delete;
    TelemetryWriter(TelemetryWriter&&)                 = delete;
    TelemetryWriter& operator=(TelemetryWriter const&) = delete;
    TelemetryWriter& operator=(TelemetryWriter&&)      = delete;

    /// Flushes and closes an active file.
    ~TelemetryWriter();

    /// Opens a timestamped telemetry file and writes its schema header.
    /// @return True when the file is ready for samples.
    /// @throws Nothing; filesystem failures are logged and converted to false.
    bool start() noexcept;

    /// Appends one Tick sample to the active telemetry stream.
    /// @param snapshot Sample to serialize.
    /// @return True when the sample remains buffered successfully.
    /// @throws Nothing; stream failures are logged and converted to false.
    bool write(hud::HudSnapshot const& snapshot) noexcept;

    /// Flushes and closes the current file. Repeated calls are safe.
    /// @throws Nothing.
    void stop() noexcept;

    /// Reports whether a telemetry file is currently open.
    /// @return True while samples can be written.
    [[nodiscard]] bool isActive() const noexcept;

private:
    ll::mod::NativeMod&   mMod;
    std::filesystem::path mCurrentPath;
    std::ofstream         mStream;
};

} // namespace boat_hud::telemetry
