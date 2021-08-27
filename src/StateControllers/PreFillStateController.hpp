#pragma once
#include <Components/StateController.hpp>
#include <States/Shared.hpp>

namespace PreFill {
    STATE(IDLE);
    STATE(STOP);
    STATE(PREFILL);

    struct Config {
        decltype(SharedStates::BagPrefill::prefillTime) prefillTime;
    };

    class Controller : public StateControllerWithConfig<Config> {
    public:
        Controller() : StateControllerWithConfig("prefill-state-machine") {}

        // FLUSH -> PREFILL -> STOP -> IDLE
        void setup() override {
            registerState(SharedStates::BagPrefill(), PREFILL, STOP);
            registerState(SharedStates::Stop(), STOP, IDLE);
            registerState(SharedStates::Idle(), IDLE);
        }

        void begin() override {

            decltype(auto) preload = getState<SharedStates::BagPrefill>(PREFILL);
            preload.prefillTime    = config.prefillTime;

            transitionTo(PREFILL);
        }

        void stop() override {
            transitionTo(STOP);
        }

        void idle() override {
            transitionTo(IDLE);
        }
    };
}  // namespace HyperFlush

using PreFillStateController = PreFill::Controller;
