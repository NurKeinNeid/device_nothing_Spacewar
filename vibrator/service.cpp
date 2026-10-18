/*
 * SPDX-FileCopyrightText: 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "RichtapVibrator.h"
#include "Vibrator.h"

using aidl::android::hardware::vibrator::Vibrator;
using aidl::vendor::aac::hardware::richtap::vibrator::RichtapVibrator;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Vibrator> vib = ndk::SharedRefBase::make<Vibrator>();
    ndk::SpAIBinder vibBinder = vib->asBinder();

    std::shared_ptr<RichtapVibrator> richtap = ndk::SharedRefBase::make<RichtapVibrator>();
    CHECK(STATUS_OK == AIBinder_setExtension(vibBinder.get(), richtap->asBinder().get()));

    const std::string instance = std::string() + Vibrator::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(vibBinder.get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
