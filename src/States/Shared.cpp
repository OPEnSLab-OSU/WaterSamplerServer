#include <States/Shared.hpp>
#include <Application/App.hpp>

namespace SharedStates {
    void Idle::enter(KPStateMachine & sm) {}

    void Stop::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.pump.off();
        app.shift.writeAllRegistersLow();
        app.intake.off();
        sm.next();
    }

    void Flush::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.intake.on();
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
        app.pump.on();
                this->condition        = nullptr;

        // This condition will be evaluated repeatedly until true then the callback will be executed
        // once
        auto const condition = [&]() {
            if (app.status.pressure >= 20) {
                this->condition = "pressure";
            }

            if (app.status.temperature < 1 && app.sensors.pressure.enabled) {
                this->condition = "temperature";
            }

            return this->condition != nullptr;
        };

        setTimeCondition(secsToMillis(time), [&]() { sm.next(); });
        setCondition(condition, [&]() { sm.next(1); });
    }

    void FlushVolume::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.intake.on();
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
        app.pump.on();

        this->condition        = nullptr;

        // This condition will be evaluated repeatedly until true then the callback will be executed
        // once
        auto const condition = [&]() {
            if (app.status.pressure >= 20) {
                this->condition = "pressure";
            }

            if (app.status.temperature < 1 && app.sensors.pressure.enabled) {
                this->condition = "temperature";
            }

            if (timeSinceLastTransition() >= secsToMillis(time)) {
                this->condition = "time";
            }

            return this->condition != nullptr;
        };

        setCondition(condition, [&]() { sm.next(); });

        auto waterVolumeCondition = [&]() { return app.status.waterVolume >= 500; };
        setCondition(waterVolumeCondition, [&]() { sm.next(1); });
    }

    void AirFlush::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.writeAllRegistersLow();
        app.shift.setPin(TPICDevices::AIR_VALVE, HIGH);
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
        app.pump.on();

                this->condition        = nullptr;

        // This condition will be evaluated repeatedly until true then the callback will be executed
        // once
        auto const condition = [&]() {
            if (app.status.pressure >= 20) {
                this->condition = "pressure";
            }

            if (app.status.temperature < 1 && app.sensors.pressure.enabled) {
                this->condition = "temperature";
            }

            return this->condition != nullptr;
        };

        setTimeCondition(secsToMillis(time), [&]() { sm.next(); });
        setCondition(condition, [&]() { sm.next(1); });
    }

    void ArgonFlush::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.writeAllRegistersLow();
        app.shift.setPin(app.currentValveIdToPin(), HIGH);
        app.shift.write();
        app.pump.on(Direction::reverse);
        this->condition        = nullptr;

        // This condition will be evaluated repeatedly until true then the callback will be executed
        // once
        auto const condition = [&]() {
            if (app.status.pressure >= 20) {
                this->condition = "pressure";
            }

            if (app.status.temperature < 1 && app.sensors.pressure.enabled) {
                this->condition = "temperature";
            }

            return this->condition != nullptr;
        };

        setTimeCondition(secsToMillis(time), [&]() { sm.next(); });
        setCondition(condition, [&]() { sm.next(1); });
    }

    void Sample::enter(KPStateMachine & sm) {
        // We set the latch valve to intake mode, turn on the filter valve, then the pump
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();
        app.intake.on();
        app.shift.setPin(app.currentValveIdToPin(), HIGH);
        app.shift.write();
        app.pump.on();

        app.sensors.flow.resetVolume();
        app.sensors.flow.startMeasurement();

        app.status.maxPressure = 0;
        this->condition        = nullptr;

        // This condition will be evaluated repeatedly until true then the callback will be executed
        // once
        auto const condition = [&]() {
            if (app.sensors.flow.volume >= volume) {
                this->condition = "volume";
            }

            if (app.status.pressure >= pressure) {
                this->condition = "pressure";
            }

            if (timeSinceLastTransition() >= secsToMillis(time)) {
                this->condition = "time";
            }

            return this->condition != nullptr;
        };

        setCondition(condition, [&]() { sm.next(); });
    }


    void OffshootClean::enter(KPStateMachine & sm) {
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setAllRegistersLow();  // Reset shift registers
        app.intake.on();
        app.shift.setPin(app.currentValveIdToPin(), HIGH);
        app.shift.setPin(TPICDevices::FLUSH_VALVE, HIGH);
        app.shift.write();
        app.pump.on(Direction::reverse);
        this->condition        = nullptr;

        // This condition will be evaluated repeatedly until true then the callback will be executed
        // once
        auto const condition = [&]() {
            if (app.status.pressure >= 20) {
                this->condition = "pressure";
            }

            if (app.status.temperature < 1 && app.sensors.pressure.enabled) {
                this->condition = "temperature";
            }

            return this->condition != nullptr;
        };

        setTimeCondition(secsToMillis(time), [&]() { sm.next(); });

        setCondition(condition, [&]() { sm.next(1); });
    };

    void OffshootPreload::enter(KPStateMachine & sm) {
        // Intake valve is opened and the motor is runnning ...
        // Turnoff only the flush valve
        auto & app = *static_cast<App *>(sm.controller);
        app.shift.setPin(TPICDevices::FLUSH_VALVE, LOW);
        app.intake.on();

        // Reserving space ahead of time for performance
        reserve(app.vm.numberOfValvesInUse + 1);
        println("Begin preloading procedure for ", app.vm.numberOfValvesInUse, " valves...");

        int counter      = 0;
        int prevValvePin = 0;
        for (const decltype(auto) valve : app.vm.valves) {
            if (valve.status == ValveStatus::unavailable) {
                continue;
            }

            // Skip the first register
            auto valvePin = valve.id + app.shift.capacityPerRegister;
            setTimeCondition(counter * preloadTime, [&app, prevValvePin, valvePin]() {
                if (prevValvePin) {
                    // Turn off the previous valve
                    app.shift.setPin(prevValvePin, LOW);
                    println("done");
                }

                app.shift.setPin(valvePin, HIGH);
                app.shift.write();
                print("Flushing offshoot ", valvePin - app.shift.capacityPerRegister, "...");
            });

            prevValvePin = valvePin;
            counter++;
        }

        // Transition to the next state after the last valve
        setTimeCondition(counter * preloadTime, [&]() {
            println("done");
            sm.next();
        });
    };

    void BagPrefill::enter(KPStateMachine & sm) {
        // Intake valve is opened and the motor is runnning ...
        // Turnoff only the flush valve
        auto & app = *static_cast<App *>(sm.controller);
        app.intake.on();

        // Reserving space ahead of time for performance
        reserve(app.vm.numberOfValvesInUse + 1);
        println("Begin prefilling procedure for ", app.vm.numberOfValvesInUse, " valves...");

        int counter      = 0;
        int prevValvePin = 0;
        for (const decltype(auto) valve : app.vm.valves) {
            if (valve.status == ValveStatus::unavailable) {
                continue;
            }

            // Skip the first register
            auto valvePin = valve.id + app.shift.capacityPerRegister;
            setTimeCondition(counter * prefillTime, [&app, prevValvePin, valvePin]() {
                if (prevValvePin) {
                    // Turn off the previous valve
                    app.shift.setPin(prevValvePin, LOW);
                    println("done");
                }

                app.shift.setPin(valvePin, HIGH);
                app.shift.write();
                print("Flushing offshoot ", valvePin - app.shift.capacityPerRegister, "...");
            });

            prevValvePin = valvePin;
            counter++;
        }

        // Transition to the next state after the last valve
        setTimeCondition(counter * prefillTime, [&]() {
            println("done");
            sm.next();
        });
    };

}  // namespace SharedStates