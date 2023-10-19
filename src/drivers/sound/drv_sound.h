#if !defined(drv_sound_h)
#define drv_sound_h

#ifdef __cplusplus
    extern "C" {
#endif

int DriverSoundInit(void);
int DriverSoundPlay(uint32_t duration_msec);

#ifdef __cplusplus
    }
#endif

#endif