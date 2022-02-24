#define PLATFORM_ANDROID
#include <android/sensor.h>
#include <android_native_app_glue.h>
#include "os/context.hpp"
#include <cassert>

static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            app->savedState = nullptr;
            app->savedStateSize = 0;
            break;
        case APP_CMD_INIT_WINDOW:
            if (app->window != nullptr) {
                //engine_init_display(engine);
                //engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            //engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            // if (engine->accelerometerSensor != nullptr) {
            //     ASensorEventQueue_enableSensor(engine->sensorEventQueue,
            //                                    engine->accelerometerSensor);
            //     // We'd like to get 60 events per second (in us).
            //     ASensorEventQueue_setEventRate(engine->sensorEventQueue,
            //                                    engine->accelerometerSensor,
            //                                    (1000L/60)*1000);
            // }
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            // if (engine->accelerometerSensor != nullptr) {
            //     ASensorEventQueue_disableSensor(engine->sensorEventQueue,
            //                                     engine->accelerometerSensor);
            // }
            // Also stop animating.
            //engine->animating = 0;
            //engine_draw_frame(engine);
            break;
        default:
            break;
    }
}

/*
 * AcquireASensorManagerInstance(void)
 *    Workaround ASensorManager_getInstance() deprecation false alarm
 *    for Android-N and before, when compiling with NDK-r15
 */
#include <dlfcn.h>
ASensorManager* AcquireASensorManagerInstance(android_app* app) {

  if(!app)
    return nullptr;

  typedef ASensorManager *(*PF_GETINSTANCEFORPACKAGE)(const char *name);
  void* androidHandle = dlopen("libandroid.so", RTLD_NOW);
  auto getInstanceForPackageFunc = (PF_GETINSTANCEFORPACKAGE)
      dlsym(androidHandle, "ASensorManager_getInstanceForPackage");
  if (getInstanceForPackageFunc) {
    JNIEnv* env = nullptr;
    app->activity->vm->AttachCurrentThread(&env, nullptr);

    jclass android_content_Context = env->GetObjectClass(app->activity->clazz);
    jmethodID midGetPackageName = env->GetMethodID(android_content_Context,
                                                   "getPackageName",
                                                   "()Ljava/lang/String;");
    auto packageName= (jstring)env->CallObjectMethod(app->activity->clazz,
                                                        midGetPackageName);

    const char *nativePackageName = env->GetStringUTFChars(packageName, nullptr);
    ASensorManager* mgr = getInstanceForPackageFunc(nativePackageName);
    env->ReleaseStringUTFChars(packageName, nativePackageName);
    app->activity->vm->DetachCurrentThread();
    if (mgr) {
      dlclose(androidHandle);
      return mgr;
    }
  }

  typedef ASensorManager *(*PF_GETINSTANCE)();
  auto getInstanceFunc = (PF_GETINSTANCE)
      dlsym(androidHandle, "ASensorManager_getInstance");
  // by all means at this point, ASensorManager_getInstance should be available
  assert(getInstanceFunc);
  dlclose(androidHandle);

  return getInstanceFunc();
}


core::os::context::context(android_app* application) : m_Application(application) 
{
    m_Application->userData = this;
    m_Application->onAppCmd = engine_handle_cmd;

    m_SensorManager = AcquireASensorManagerInstance(m_Application);
    m_AccelerometerSensor = ASensorManager_getDefaultSensor(m_SensorManager, ASENSOR_TYPE_ACCELEROMETER);
    m_SensorEventQueue = ASensorManager_createEventQueue(m_SensorManager, m_Application->looper, LOOPER_ID_USER, nullptr, nullptr);

    if (m_Application->savedState != nullptr) {
        // We are starting with a previous saved state; restore from it.
        // engine.state = *(struct saved_state*)state->savedState;
    }
}

auto core::os::context::application() noexcept -> android_app&
{
    return *m_Application;
}


bool core::os::context::tick() noexcept
{
    int ident;
    int events;
    android_poll_source* source;

    // If not animating, we will block forever waiting for events.
    // If animating, we loop until all events are read, then continue
    // to draw the next frame of animation.
    while ((ident=ALooper_pollAll(m_Paused ? 0 : -1, nullptr, &events,
                                    (void**)&source)) >= 0) {

        // Process this event.
        if (source != nullptr) {
            source->process(m_Application, source);
        }

        // If a sensor has data, process it now.
        if (ident == LOOPER_ID_USER) {
            if (m_AccelerometerSensor != nullptr) {
                ASensorEvent event;
                while (ASensorEventQueue_getEvents(m_SensorEventQueue,
                                                    &event, 1) > 0) {
                }
            }
        }

        // Check if we are exiting.
        if (m_Application->destroyRequested != 0) {
            return false;
        }
    }

    return true;
}