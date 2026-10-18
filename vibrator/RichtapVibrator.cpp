/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RichtapVibrator.h"

#include <log/log.h>

#include "aac_vibra_function.h"

namespace aidl {
namespace vendor {
namespace aac {
namespace hardware {
namespace richtap {
namespace vibrator {

namespace {

// Same internal IDs used by the AOSP perform() path in Vibrator.cpp.
// The framework sends 0x1000 + the AOSP effect id.
uint32_t toAacEffectId(int32_t effectId) {
    int32_t aospId = effectId;
    if (effectId >= 0x1000 && effectId < 0x1100) {
        aospId = effectId - 0x1000;
    }

    switch (aospId) {
        case 0:  // CLICK
            return 0x3008;
        case 1:  // DOUBLE_CLICK
            return 0x1001;
        case 2:  // TICK
        case 3:  // THUD
        case 4:  // POP
            return 0x3003;
        case 5:  // HEAVY_CLICK
            return 0x3007;
        default:
            return static_cast<uint32_t>(effectId);
    }
}

// Framework prebaked strengths are 150/200/250. Match the actuator tuning used by perform().
int32_t toAacStrength(int8_t strength) {
    switch (static_cast<uint8_t>(strength)) {
        case 150:
            return 69;
        case 200:
            return 100;
        case 250:
            return 150;
        default:
            return static_cast<uint8_t>(strength);
    }
}

void notify(const std::shared_ptr<IRichtapCallback>& callback, int32_t status) {
    if (callback != nullptr) {
        callback->onCallback(status);
    }
}

}  // namespace

ndk::ScopedAStatus RichtapVibrator::init(const std::shared_ptr<IRichtapCallback>& callback) {
    // libaacvibrator is already started by the IVibrator service.
    notify(callback, 0);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::setDynamicScale(
        int32_t scale, const std::shared_ptr<IRichtapCallback>& callback) {
    int32_t ret = aac_vibra_dynamic_scale(static_cast<uint8_t>(scale));
    notify(callback, ret);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::setF0(int32_t f0,
                                          const std::shared_ptr<IRichtapCallback>& callback) {
    int32_t ret = aac_vibra_setting_f0(f0);
    notify(callback, ret);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::stop(const std::shared_ptr<IRichtapCallback>& callback) {
    aac_vibra_looper_stopPerformHe();
    int32_t index = 0;
    int32_t ret = aac_vibra_stop(&index);
    if (ret) {
        ret = aac_vibra_off();
    }
    notify(callback, ret);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::setAmplitude(
        int32_t amplitude, const std::shared_ptr<IRichtapCallback>& callback) {
    if (amplitude < 0) {
        amplitude = 0;
    } else if (amplitude > 255) {
        amplitude = 255;
    }

    int32_t ret = aac_vibra_setAmplitude(static_cast<uint8_t>(amplitude));
    notify(callback, ret);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::performHeParam(
        int32_t interval, int32_t amplitude, int32_t freq,
        const std::shared_ptr<IRichtapCallback>& callback) {
    bool ok = aac_vibra_looper_performParam(interval, amplitude, freq);
    notify(callback, ok ? 0 : -1);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::off(const std::shared_ptr<IRichtapCallback>& callback) {
    aac_vibra_looper_stopPerformHe();
    int32_t ret = aac_vibra_off();
    notify(callback, ret);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::on(int32_t timeoutMs,
                                       const std::shared_ptr<IRichtapCallback>& callback) {
    int32_t ret = aac_vibra_looper_on(static_cast<uint32_t>(timeoutMs));
    if (ret < 0) {
        ALOGE("RichTap on failed: %d", ret);
    }
    notify(callback, ret);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::perform(int32_t effectId, int8_t strength,
                                            const std::shared_ptr<IRichtapCallback>& callback,
                                            int32_t* _aidl_return) {
    // A waveform can leave the gain at a very small value. Prebaked strength is only
    // meaningful when the actuator gain is restored first.
    aac_vibra_setAmplitude(0xFF);

    uint32_t aacEffect = toAacEffectId(effectId);
    int32_t aacStrength = toAacStrength(strength);
    int32_t ret = aac_vibra_looper_prebaked_effect(aacEffect, aacStrength);
    if (ret < 0) {
        ALOGE("RichTap perform failed: effect=0x%x strength=%d ret=%d", aacEffect, aacStrength,
              ret);
        *_aidl_return = 0;
    } else {
        *_aidl_return = ret > 0 ? ret : 1;
    }
    notify(callback, ret);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::performEnvelope(
        const std::vector<int32_t>& envInfo, bool fastFlag,
        const std::shared_ptr<IRichtapCallback>& callback) {
    int32_t ret = -1;
    if (!envInfo.empty()) {
        ret = aac_vibra_looper_envelope(envInfo.data(), static_cast<uint32_t>(envInfo.size()),
                                        fastFlag);
    }
    if (ret < 0) {
        ALOGE("RichTap envelope failed: %d", ret);
    }
    notify(callback, ret);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::performRtp(const ndk::ScopedFileDescriptor& hdl,
                                               const std::shared_ptr<IRichtapCallback>& callback) {
    int32_t ret = aac_vibra_looper_rtp(hdl.get());
    if (ret < 0) {
        ALOGE("RichTap rtp failed: %d", ret);
    }
    notify(callback, ret);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::performHe(int32_t looper, int32_t interval, int32_t amplitude,
                                              int32_t freq, const std::vector<int32_t>& he,
                                              const std::shared_ptr<IRichtapCallback>& callback) {
    const int32_t* pattern = he.empty() ? nullptr : he.data();
    int32_t ret = aac_vibra_looper_post(pattern, static_cast<int32_t>(he.size()), interval, looper,
                                        amplitude, freq);
    if (ret < 0) {
        ALOGE("RichTap performHe failed: %d", ret);
    }
    notify(callback, ret);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::setHapticParam(
        const std::vector<int32_t>& data __unused, int32_t length __unused,
        const std::shared_ptr<IRichtapCallback>& callback) {
    // libaacvibrator on Spacewar has no DRC/haptic-param entry point.
    notify(callback, 0);
    return ndk::ScopedAStatus::ok();
}

}  // namespace vibrator
}  // namespace richtap
}  // namespace hardware
}  // namespace aac
}  // namespace vendor
}  // namespace aidl
