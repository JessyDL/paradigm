#include "psl/platform_def.hpp"
#if defined(PLATFORM_ANDROID)
#include "os/surface.hpp"
#include "psl/assertions.hpp"
#include <android/sensor.h>
#include <android_native_app_glue.h>

using namespace core::os;

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
  psl_assert(getInstanceFunc, "Could not find ASensorManager_getInstance");
  dlclose(androidHandle);

  return getInstanceFunc();
}

bool surface::init_surface(struct android_app* app) {
    m_Application = app;
    m_Focused = false;

    m_Application->userData = this;

    m_SensorManager = AcquireASensorManagerInstance(m_Application);
    m_AccelerometerSensor = ASensorManager_getDefaultSensor(m_SensorManager, ASENSOR_TYPE_ACCELEROMETER);
    m_SensorEventQueue = ASensorManager_createEventQueue(m_SensorManager, m_Application->looper, LOOPER_ID_USER, nullptr, nullptr);

    if (m_Application->savedState != nullptr) {
        // We are starting with a previous saved state; restore from it.
        // engine.state = *(struct saved_state*)state->savedState;
    }

    // We will continue to poll until the window is ready
    while(m_Application->window == nullptr)
        update_surface();

    return true; 
}

ANativeWindow* surface::surface_handle() const noexcept
{
    return m_Application->window;
}

void surface::deinit_surface() {}


void surface::focus(bool value) 
{
    if(value == m_Focused)
        return;
    core::os::log->info((value)?"surface focused":"surface moved to background");
    m_Focused = value;
}

void surface::update_surface()
{
    int ident;
    int events;
    android_poll_source* source;

    // If not animating, we will block forever waiting for events.
    // If animating, we loop until all events are read, then continue
    // to draw the next frame of animation.
    while ((ident=ALooper_pollAll(m_Focused ? 0 : -1, nullptr, &events,
                                    (void**)&source)) >= 0) {

        if(ident == LOOPER_ID_MAIN) {
            int8_t cmd = android_app_read_cmd(m_Application);
            android_app_pre_exec_cmd(m_Application, cmd);
            process(cmd);
            android_app_post_exec_cmd(m_Application, cmd);
        }
        psl_assert(ident == LOOPER_ID_MAIN || ident == LOOPER_ID_USER, "Expected ident to be 'LOOPER_ID_MAIN' or 'LOOPER_ID_USER', but got {}", ident);

        // If a sensor has data, process it now.
        if (ident == LOOPER_ID_USER) {
            AInputEvent* event = NULL;
            if (AInputQueue_getEvent(m_Application->inputQueue, &event) >= 0) {
                if (AInputQueue_preDispatchEvent(m_Application->inputQueue, event)) {
                    return;
                }
                int32_t handled = 0;
                // if (m_Application->onInputEvent != NULL) handled = m_Application->onInputEvent(m_Application, event);
                AInputQueue_finishEvent(m_Application->inputQueue, event, handled);
            } 
            else
            {
                psl::unreachable("Failure reading next input event: {}", strerror(errno));
            }
            if (m_AccelerometerSensor != nullptr) {
                ASensorEvent event;
                while (ASensorEventQueue_getEvents(m_SensorEventQueue,
                                                    &event, 1) > 0) {
                }
            }
        }

        // Check if we are exiting.
        if (m_Application->destroyRequested != 0) {
            m_Open = false;
            core::os::log->info("destroy requested");
            return;
        }
    }
}


void surface::resize_surface() {}


void surface::process(int32_t cmd)
{
    core::os::log->info("processing cmd {}", cmd);
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            m_Application->savedState = nullptr;
            m_Application->savedStateSize = 0;
            break;
        case APP_CMD_INIT_WINDOW:
            if (m_Application->window != nullptr) {
                focus(true);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            //engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            focus(true);
            // When our app gains focus, we start monitoring the accelerometer.
            if (m_AccelerometerSensor != nullptr) {
                ASensorEventQueue_enableSensor(m_SensorEventQueue,
                                               m_AccelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(m_SensorEventQueue,
                                               m_AccelerometerSensor,
                                               (1000L/60)*1000);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            focus(false);
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (m_AccelerometerSensor != nullptr) {
                ASensorEventQueue_disableSensor(m_SensorEventQueue,
                                                m_AccelerometerSensor);
            }
            break;
        default:
            break;
    }
}

#endif
