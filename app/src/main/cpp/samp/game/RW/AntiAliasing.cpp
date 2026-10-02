// Положить в: app/src/main/cpp/samp/game/RW/AntiAliasing.cpp
// Подключить в CMakeLists.txt не нужно - GLOB_RECURSE подхватит сам.
//
// Что делает:
//   Движок (libGame.so) при старте вызывает eglChooseConfig() без единого
//   бита MSAA (см. GLES3JNIView.java: setEGLConfigChooser(8,8,8,8,16,0) -
//   там просто нет параметра samples, а это неиспользуемый класс; сам же
//   движок создаёт свой EGL-контекст в libGame.so точно так же, без
//   EGL_SAMPLE_BUFFERS/EGL_SAMPLES). Поэтому края объектов, столбы, листва -
//   рваные, как будто сглаживание выключено. Потому что оно и выключено.
//
//   Этот файл перехватывает eglChooseConfig в libEGL.so и на лету
//   дописывает в attrib_list запрос 4x MSAA перед тем как отдать его
//   оригинальной функции. Если конфиг с 4x не находится на устройстве,
//   пробует 2x, затем отдаёт как есть (без AA) - чтобы игра не крашилась
//   на слабых GPU.

#include <EGL/egl.h>
#include <cstdio>
#include "vendor/GlossHook/include/Gloss.h"

typedef EGLBoolean (*eglChooseConfig_t)(EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*);
static eglChooseConfig_t g_eglChooseConfig_orig = nullptr;

// Сколько сэмплов MSAA пробуем запросить. 4 - разумный баланс качества
// и производительности для мобильного железа. Можно поднять до 8,
// но на слабых чипах это ощутимо ударит по FPS.
static constexpr EGLint kDesiredSamples = 4;

static bool AppendSamples(const EGLint* src, EGLint* dst, size_t dstCap, EGLint samples)
{
    // Копируем исходный attrib_list, вставляя EGL_SAMPLE_BUFFERS/EGL_SAMPLES
    // перед терминатором EGL_NONE. Если в списке уже были эти атрибуты -
    // не трогаем его и возвращаем false (пусть уходит как есть).
    size_t n = 0;
    if (src)
    {
        for (size_t i = 0; ; i += 2)
        {
            if (src[i] == EGL_NONE) break;
            if (src[i] == EGL_SAMPLE_BUFFERS || src[i] == EGL_SAMPLES)
                return false; // движок сам просит AA - не мешаем
            if (n + 2 >= dstCap - 5) return false; // на всякий случай, не переполняем буфер
            dst[n++] = src[i];
            dst[n++] = src[i + 1];
        }
    }

    if (samples > 0)
    {
        dst[n++] = EGL_SAMPLE_BUFFERS;
        dst[n++] = 1;
        dst[n++] = EGL_SAMPLES;
        dst[n++] = samples;
    }
    dst[n++] = EGL_NONE;
    return true;
}

static EGLBoolean eglChooseConfig_hook(EGLDisplay dpy, const EGLint* attrib_list,
                                        EGLConfig* configs, EGLint config_size,
                                        EGLint* num_config)
{
    EGLint buf[64];

    // Пробуем 4x, потом 2x, и только если оба не подошли - отдаём
    // оригинальный список без изменений (совсем без AA, как было).
    for (EGLint samples : { kDesiredSamples, (EGLint)2 })
    {
        if (!AppendSamples(attrib_list, buf, 64, samples))
            break; // движок уже сам просил AA - не вмешиваемся вообще

        EGLBoolean ok = g_eglChooseConfig_orig(dpy, buf, configs, config_size, num_config);
        if (ok && num_config && *num_config > 0)
            return ok;
    }

    return g_eglChooseConfig_orig(dpy, attrib_list, configs, config_size, num_config);
}

void InstallAntiAliasingHook()
{
    GHook h = GlossHookByName("libEGL.so", "eglChooseConfig",
                               (void*)&eglChooseConfig_hook,
                               (void**)&g_eglChooseConfig_orig,
                               nullptr);
    if (h)
        printf("[AA] eglChooseConfig hooked, requesting %dx MSAA\n", kDesiredSamples);
    else
        printf("[AA] failed to hook eglChooseConfig - AA not applied\n");
}
