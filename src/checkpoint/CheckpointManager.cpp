// SPDX-License-Identifier: CC0-1.0

#include "checkpoint/CheckpointManager.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ll/api/mod/NativeMod.h"

namespace boat_hud::checkpoint {
namespace {

constexpr double DenominatorThreshold = 1.0e-9;

std::vector<std::string> splitCsvLine(std::string const& line) {
    std::vector<std::string> values;
    std::stringstream        stream(line);
    for (std::string value; std::getline(stream, value, ',');) values.emplace_back(std::move(value));
    return values;
}

} // namespace

CheckpointManager::CheckpointManager(ll::mod::NativeMod& mod) : mMod(mod) {}

bool CheckpointManager::load(std::string const& configuredPath) noexcept {
    reset();
    try {
        std::filesystem::path path{configuredPath};
        if (path.is_relative()) path = mMod.getConfigDir() / path;

        std::ifstream stream(path);
        if (!stream.is_open()) {
            mMod.getLogger().error("Failed to open checkpoint file {}", path.string());
            return false;
        }

        std::size_t lineNumber = 0;
        for (std::string line; std::getline(stream, line);) {
            ++lineNumber;
            if (line.empty() || line.starts_with('#') || line.find("time") != std::string::npos) continue;

            auto const values = splitCsvLine(line);
            if (values.size() != 5) {
                mMod.getLogger().warn("Ignoring checkpoint line {}: expected five columns", lineNumber);
                continue;
            }

            Checkpoint checkpoint;
            try {
                checkpoint = {
                    std::stod(values[0]),
                    std::stod(values[1]),
                    std::stod(values[2]),
                    std::stod(values[3]),
                    std::stod(values[4]),
                };
            } catch (std::exception const& exception) {
                mMod.getLogger().warn("Ignoring checkpoint line {}: {}", lineNumber, exception.what());
                continue;
            }
            double const normalLength = std::hypot(checkpoint.normalX, checkpoint.normalZ);
            bool const   finite = std::isfinite(checkpoint.referenceTime) && std::isfinite(checkpoint.referenceSpeed)
                               && std::isfinite(checkpoint.normalX) && std::isfinite(checkpoint.normalZ)
                               && std::isfinite(checkpoint.planeOffset);
            if (!finite || normalLength <= DenominatorThreshold) {
                mMod.getLogger().warn("Ignoring invalid checkpoint line {}", lineNumber);
                continue;
            }

            checkpoint.normalX     /= normalLength;
            checkpoint.normalZ     /= normalLength;
            checkpoint.planeOffset /= normalLength;
            mCheckpoints.emplace_back(checkpoint);
        }

        if (mCheckpoints.empty()) {
            mMod.getLogger().error("Checkpoint file {} did not contain a valid checkpoint", path.string());
            return false;
        }
        mMod.getLogger().info("Loaded {} checkpoints from {}", mCheckpoints.size(), path.string());
        return true;
    } catch (std::exception const& exception) {
        mMod.getLogger().error("Failed to load checkpoints: {}", exception.what());
    } catch (...) {
        mMod.getLogger().error("Failed to load checkpoints because of an unknown error");
    }
    reset();
    return false;
}

void CheckpointManager::reset() noexcept {
    mCheckpoints.clear();
    mNextCheckpoint    = 0;
    mHasPreviousSample = false;
    mPreviousX         = 0.0;
    mPreviousZ         = 0.0;
    mPreviousSpeed     = 0.0;
    mPreviousTime      = 0.0;
    mLapStartTime.reset();
}

std::optional<CheckpointResult>
CheckpointManager::update(hud::HudSnapshot const& snapshot, bool circularTrack) noexcept {
    if (mCheckpoints.empty()) return std::nullopt;

    double const currentTime = static_cast<double>(snapshot.elapsedTicks) / 20.0;
    if (!mHasPreviousSample) {
        mHasPreviousSample = true;
        mPreviousX         = snapshot.positionX;
        mPreviousZ         = snapshot.positionZ;
        mPreviousSpeed     = snapshot.speed;
        mPreviousTime      = currentTime;
        return std::nullopt;
    }

    std::optional<CheckpointResult> result;
    std::size_t                     crossingsRemaining = mCheckpoints.size();
    while (crossingsRemaining-- > 0 && mNextCheckpoint < mCheckpoints.size()) {
        auto const&  checkpoint    = mCheckpoints[mNextCheckpoint];
        double const previousValue = planeValue(checkpoint, mPreviousX, mPreviousZ);
        double const currentValue  = planeValue(checkpoint, snapshot.positionX, snapshot.positionZ);
        if (!(previousValue <= 0.0 && currentValue > 0.0)) break;

        double const denominator = currentValue - previousValue;
        if (std::abs(denominator) <= DenominatorThreshold) break;
        double const progress      = std::clamp(-previousValue / denominator, 0.0, 1.0);
        double const crossingTime  = mPreviousTime + (currentTime - mPreviousTime) * progress;
        double const crossingSpeed = mPreviousSpeed * (1.0 - progress) + snapshot.speed * progress;

        if (mNextCheckpoint == 0) mLapStartTime = crossingTime;
        double const lapStartTime = mLapStartTime.value_or(crossingTime);
        result                    = CheckpointResult{
            mNextCheckpoint + 1,
            mCheckpoints.size(),
            crossingTime - lapStartTime - checkpoint.referenceTime,
            crossingSpeed - checkpoint.referenceSpeed,
        };

        ++mNextCheckpoint;
        if (mNextCheckpoint == mCheckpoints.size()) {
            if (!circularTrack) break;
            mNextCheckpoint = 0;
            mLapStartTime.reset();
        }
    }

    mPreviousX     = snapshot.positionX;
    mPreviousZ     = snapshot.positionZ;
    mPreviousSpeed = snapshot.speed;
    mPreviousTime  = currentTime;
    return result;
}

double CheckpointManager::planeValue(Checkpoint const& checkpoint, double x, double z) noexcept {
    return x * checkpoint.normalX + z * checkpoint.normalZ - checkpoint.planeOffset;
}

} // namespace boat_hud::checkpoint
