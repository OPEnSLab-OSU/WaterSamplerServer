#include <StateControllers/HyperFlushStateController.hpp>
#include <Application/App.hpp>
void NewStateController::setup() {
    // registerState(SharedStates::Flush(), FLUSH1, [this](int code) {
    // 	switch (code) {
    // 	case 0:
    // 		return transitionTo(OFFSHOOT_CLEAN_1);
    // 	default:
    // 		halt(TRACE, "Unhandled state transition");
    // 	}
    // });
    // ..or alternatively if state only has one input and one output

    registerState(SharedStates::ArgonFlush(), ARGON_FLUSH, [this](int code) {
        auto & app = *static_cast<App *>(controller);
        switch (code) {
        case 1:
            app.logInterruptedSample();
            return transitionTo(STOP);
        default:
            return transitionTo(FLUSH);
        }
    });
    registerState(SharedStates::Flush(), FLUSH, [this](int code) {
        auto & app = *static_cast<App *>(controller);
        switch (code) {
        case 1:
            app.logInterruptedSample();
            return transitionTo(STOP);
        default:
            return transitionTo(SAMPLE);
        }
    });
    registerState(SharedStates::Sample(), SAMPLE, [this](int code) {
        auto & app = *static_cast<App *>(controller);
        app.sensors.flow.stopMeasurement();
        app.logAfterSample();

        switch (code) {
        case 0:
            return transitionTo(AIR_FLUSH);
        default:
            halt(TRACE, "Unhandled state transition: ", code);
        }
    });
    registerState(SharedStates::AirFlush(), AIR_FLUSH, [this](int code) {
        auto & app = *static_cast<App *>(controller);
        switch (code) {
        case 1:
            //May not make sense to have since sampling already occured.
            //app.logInterruptedSample();
            return transitionTo(STOP);
        default:
            return transitionTo(STOP);
        }  
    });

    // Reusing STOP and IDLE states from MainStateController
    registerState(Main::Stop(), STOP, IDLE);
    registerState(Main::Idle(), IDLE);
};