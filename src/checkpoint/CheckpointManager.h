// SPDX-License-Identifier: CC0-1.0
#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "hud/HudSnapshot.h"

namespace ll::mod {
class NativeMod;
}

namespace boat_hud::checkpoint {

/// Describes one successfully crossed checkpoint relative to its reference sample.
struct CheckpointResult {
    /// One-based index of the crossed checkpoint.
    std::size_t checkpointIndex = 0;

    /// Total number of checkpoints in the loaded course.
    std::size_t checkpointCount = 0;

    /// Signed time difference from the reference in seconds.
    double timeDelta = 0.0;

    /// Signed speed difference from the reference in metres per second.
    double speedDelta = 0.0;
};

/// Loads checkpoint planes and detects forward crossings using Tick-to-Tick interpolation.
class CheckpointManager {
public:
    /// Creates a manager rooted in the owning mod's configuration directory.
    /// @param mod Owning native mod used for paths and diagnostics.
    explicit CheckpointManager(ll::mod::NativeMod& mod);

    /// Loads and validates a five-column checkpoint CSV file.
    /// @param configuredPath Absolute path or path relative to the mod configuration directory.
    /// @return True when at least one checkpoint was loaded.
    /// @throws Nothing; parse and filesystem errors are logged and converted to false.
    bool load(std::string const& configuredPath) noexcept;

    /// Clears the active course and all crossing state.
    /// @throws Nothing.
    void reset() noexcept;

    /// Evaluates the latest boat sample against the next checkpoint plane.
    /// @param snapshot Current Tick-derived position and speed.
    /// @param circularTrack True to continue at checkpoint one after the final checkpoint.
    /// @return Most recent crossing in this Tick, or no value when nothing was crossed.
    /// @throws Nothing.
    [[nodiscard]] std::optional<CheckpointResult> update(hud::HudSnapshot const& snapshot, bool circularTrack) noexcept;

private:
    struct Checkpoint {
        double referenceTime  = 0.0;
        double referenceSpeed = 0.0;
        double normalX        = 0.0;
        double normalZ        = 0.0;
        double planeOffset    = 0.0;
    };

    /// Calculates the signed distance expression for a checkpoint plane.
    /// @param checkpoint Plane to evaluate.
    /// @param x World X coordinate.
    /// @param z World Z coordinate.
    /// @return Negative before the checkpoint and positive after it.
    [[nodiscard]] static double planeValue(Checkpoint const& checkpoint, double x, double z) noexcept;

    ll::mod::NativeMod&     mMod;
    std::vector<Checkpoint> mCheckpoints;
    std::size_t             mNextCheckpoint    = 0;
    bool                    mHasPreviousSample = false;
    double                  mPreviousX         = 0.0;
    double                  mPreviousZ         = 0.0;
    double                  mPreviousSpeed     = 0.0;
    double                  mPreviousTime      = 0.0;
    std::optional<double>   mLapStartTime;
};

} // namespace boat_hud::checkpoint
