#pragma once

#include "ALVR-common/packet_types.h"
#include "TrackedDevice.h"
#include "openvr_driver_wrap.h"
#include <map>

class Controller : public TrackedDevice {
public:
    Controller(uint64_t deviceID, vr::EVRSkeletalTrackingLevel skeletonLevel);
    virtual ~Controller() {};
    void RegisterButton(uint64_t id);
    void SetButton(uint64_t id, FfiButtonValue value);
    bool OnPoseUpdate(uint64_t targetTimestampNs, float predictionS, FfiHandData handData);

private:
    static const int SKELETON_BONE_COUNT = 31;
    static const int ANIMATION_FRAME_COUNT = 15;

    std::map<uint64_t, vr::VRInputComponentHandle_t> m_buttonHandles;

    vr::VRInputComponentHandle_t m_compHaptic;
    vr::VRInputComponentHandle_t m_compSkeleton = vr::k_ulInvalidInputComponentHandle;
    vr::EVRSkeletalTrackingLevel m_skeletonLevel;

    uint64_t m_poseTargetTimestampNs;

    // --- Idle-hold -------------------------------------------------------
    // Timestamp of the last tick where this device both owned its hand role
    // and had a live pose sample. Used to freeze the hand in place for a short
    // window when the pose source drops out, instead of reporting the device
    // as disconnected (which VRChat animates as an arm-drop).
    uint64_t m_lastEnabledTimestampNs = 0;
    bool m_holdingLastPose = false;
    // Whether stale input has already been neutralised for this dropout.
    bool m_inputsReleased = false;
    // How long to keep holding before letting the device report a genuine
    // disconnect.
    //
    // This is generous on purpose: the whole point of the patch is that a
    // controller you set down keeps its hand in place, and a short cap just
    // hands the arm-drop back after a few seconds. It is only safe to be this
    // generous because ReleaseAllInputs() clears input on the way into a hold
    // -- an earlier revision paired a 300 s hold with live stale input, which
    // let a thumbstick keep driving the player. Never raise this without that
    // release in place.
    //
    // A controller that is genuinely powered off still disappears, just after
    // five minutes rather than immediately.
    static constexpr uint64_t IDLE_HOLD_TIMEOUT_NS = 300ULL * 1000000000ULL; // 300 s

    // Tracks which registered components are boolean, so a dropout can reset
    // each one to its correct neutral value.
    std::map<uint64_t, bool> m_buttonIsBinary;

    // These variables are used for controller hand animation
    // todo: move to rust
    float m_thumbTouchAnimationProgress = 0;
    float m_indexTouchAnimationProgress = 0;
    bool m_currentThumbTouch = false;
    bool m_lastThumbTouch = false;
    bool m_currentTriggerTouch = false;
    bool m_lastTriggerTouch = false;
    float m_triggerValue = 0;
    float m_gripValue = 0;

    vr::VRInputComponentHandle_t getHapticComponent();
    void ReleaseAllInputs();
    void GetBoneTransform(bool withController, vr::VRBoneTransform_t outBoneTransform[]);

    // TrackedDevice
    bool activate() final;
    void* get_component([[maybe_unused]] const char* component_name_and_version) final {
        return nullptr;
    }
};
