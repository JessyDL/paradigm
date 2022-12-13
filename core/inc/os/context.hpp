#pragma once

#if defined(PLATFORM_ANDROID)
struct android_app;
struct ASensorManager;
struct ASensor;
struct ASensorEventQueue;

namespace core::os
{
class context
{
  public:
	context(android_app* application);

	auto application() noexcept -> android_app&;
	bool tick() noexcept;

  private:
	android_app* m_Application {nullptr};
	bool m_Paused {false};
	ASensorManager* m_SensorManager {nullptr};
	const ASensor* m_AccelerometerSensor {nullptr};
	ASensorEventQueue* m_SensorEventQueue {nullptr};
};
}	 // namespace core::os
#else
namespace core::os
{
class context
{
  public:
	bool tick() noexcept { return true; }
};
}	 // namespace core::os
#endif
