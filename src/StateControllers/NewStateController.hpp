#pragma once
#include <Components/StateController.hpp>
#include <States/Shared.hpp>
#include <StateControllers/MainStateController.hpp>

namespace New {
    STATE(IDLE);
    STATE(ARGON_FLUSH);
    STATE(FLUSH);
    STATE(SAMPLE);
    STATE(AIR_FLUSH);
    STATE(STOP);

    struct Config {
        decltype(SharedStates::Flush::time) flushTime;
        decltype(SharedStates::Sample::time) sampleTime;
        decltype(SharedStates::Sample::pressure) samplePressure;
        decltype(SharedStates::Sample::volume) sampleVolume;
    };

    class Controller : public StateController, public StateControllerConfig<Config> {
    public:
        Controller() : StateController("new-state-controller") {}

        void setup();
        void configureStates() {
            decltype(auto) flush1 = getState<SharedStates::Flush>(FLUSH);
            flush1.time           = config.flushTime;

            decltype(auto) sample = getState<SharedStates::Sample>(SAMPLE);
            sample.time           = config.sampleTime;
            sample.pressure       = config.samplePressure;
            sample.volume         = config.sampleVolume;

        }

        void begin() override {
            configureStates();
            transitionTo(ARGON_FLUSH);
        }

        void stop() override {
            transitionTo(STOP);
        }

        void idle() override {
            transitionTo(IDLE);
        }

        bool isStop() {
            return getCurrentState()->getName() == STOP;
        }
    };
};  // namespace New

using NewStateController = New::Controller;
