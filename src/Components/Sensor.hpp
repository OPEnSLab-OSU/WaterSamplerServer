#pragma once
#include <KPFoundation.hpp>
#include <functional>
#include <tuple>

#include <Wire.h>

template <typename Function, typename Tuple, size_t... I>
auto call(Function f, Tuple t, std::index_sequence<I...>) {
	return f(std::get<I>(t)...);
}

template <typename Function, typename Tuple>
auto call(Function f, Tuple t) {
	static constexpr auto size = std::tuple_size<Tuple>::value;
	return call(f, t, std::make_index_sequence<size>{});
}

template <typename _SensorData, typename... Types>
class Sensor {
public:
	struct ErrorCode {
		enum Code { success = 0, tooEarly, invalidChecksum } _code;
		ErrorCode(Code code) : _code(code) {}

		operator Code() const {
			return _code;
		}
	};

private:
	ErrorCode errorCode			 = ErrorCode::success;
	unsigned long updateInterval = 0;

public:
	using SensorData = const _SensorData;

	/**
	 * When this property is set, calling the update function will forward the sensor response to
	 * the callback upon successful reading.
	 *
	 */
	std::function<void(SensorData)> onReceived;

	/**
	 * Subclass should override this method for setting up the sensor for reading/writing
	 */
	virtual void begin() = 0;

	/**
	 * Sublass should override this method to return SensorData
	 *
	 * @return SensorData Object instance of type SensorData provided in the template parameter
	 */
	virtual SensorData read() = 0;

	/**
	 * Set the Error Code object
	 *
	 * @param code Ex. ErrorCode::success, ErrorCode::tooEarly, etc.
	 */
	void setErrorCode(ErrorCode code) {
		errorCode = code;
	}

	/**
	 * Get the Error Code object
	 *
	 * @return ErrorCode x. ErrorCode::success, ErrorCode::tooEarly, etc.
	 */
	ErrorCode getErrorCode() {
		return errorCode;
	}

	/**
	 * Set the Update Freq object
	 *
	 * @param freqHz # per second (1000ms)
	 */
	void setUpdateFreq(double freqHz) {
		if (freqHz <= 0) {
			updateInterval = INT64_MAX;
		} else {
			updateInterval = static_cast<int>(1000.0 / freqHz);
		}
	}

	/**
	 * Calling this method will trigger a call to read() only if time between call is more than the
	 * configured interval setting. Results from read() will then be forwarded to onReceived
	 * callback
	 *
	 * @return ErrorCode
	 */
	virtual ErrorCode update() final {
		static long last_update = millis();
		if ((millis() - last_update) < updateInterval) {
			return ErrorCode::tooEarly;
		}

		setErrorCode(ErrorCode::success);
		const auto response	 = read();
		const auto errorCode = getErrorCode();
		last_update			 = millis();

		if (errorCode == ErrorCode::success && onReceived) {
			onReceived(response);
		}

		return errorCode;
	}
};

/**
 * Convenient template class for tuple type SensorData.
 * Usage: Sensor<int, float, double> -> Sensor<std::tuple<int, float, double>>
 *
 * @tparam First First element type of the tuple
 * @tparam Second Second element type of tuple
 * @tparam Types The rest of the types of tuple elements
 */
template <typename First, typename Second, typename... Types>
struct Sensor<First, Second, Types...> : public Sensor<std::tuple<First, Second, Types...>> {};