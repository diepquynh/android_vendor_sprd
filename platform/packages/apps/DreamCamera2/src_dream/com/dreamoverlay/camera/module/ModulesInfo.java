
package com.android.camera.module;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;

import com.android.camera.CaptureModule;
import com.android.camera.PhotoModule;
import com.android.camera.VideoModule;
import com.android.camera.app.AppController;
import com.android.camera.app.ModuleManager;
import com.android.camera.captureintent.CaptureIntentModule;
import com.android.camera.debug.Log;
import com.android.camera.module.ModuleController;
import com.android.camera.one.OneCamera;
import com.android.camera.one.OneCameraException;
import com.android.camera.one.config.OneCameraFeatureConfig;
import com.android.camera.one.config.OneCameraFeatureConfig.HdrPlusSupportLevel;
import com.android.camera.settings.SettingsScopeNamespaces;
import com.android.camera.util.CameraUtil;
import com.android.camera.util.GcamHelper;
import com.android.camera.util.PhotoSphereHelper;
import com.android.camera.util.RefocusHelper;
import com.android.camera2.R;
import com.dream.camera.modules.autophoto.AutoPhotoModule;
import com.dream.camera.modules.autovideo.AutoVideoModule;
import com.dream.camera.modules.continuephoto.ContinuePhotoModule;
import com.dream.camera.modules.filterdream.DreamFilterModule;
import com.dream.camera.modules.gifphoto.GifPhotoModule;
import com.dream.camera.modules.gifvideo.GifVideoModule;
import com.dream.camera.modules.intervalphoto.IntervalPhotoModule;
import com.dream.camera.modules.manualphoto.ManualPhotoModule;
import com.dream.camera.modules.panoramadream.DreamPanoramaModule;
import com.dream.camera.modules.scenephoto.ScenePhotoModule;
import com.dream.camera.modules.scenerydream.DreamSceneryModule;
import com.dream.camera.modules.slowmotionvideo.SlowmotionVideoModule;
import com.dream.camera.modules.timelapsevideo.TimelapseVideoModule;
import com.dream.camera.modules.vgesturephoto.VgesturePhotoModule;
import com.dream.camera.modules.intentcapture.DreamIntentCaptureModule;
import com.dream.camera.modules.intentvideo.DreamIntentVideoModule;
import com.dream.camera.modules.AudioPicture.AudioPictureModule;
import com.dream.camera.modules.qr.QrCodePhotoModule;
import com.dream.camera.util.DreamUtil;
import com.ucamera.ucam.modules.utils.UCamUtill;

public class ModulesInfo {
    private static final Log.Tag TAG = new Log.Tag("ModulesInfo");

    public static void setupModules(Context context, ModuleManager moduleManager,
            OneCameraFeatureConfig config) {
        setupDreamModules(context, moduleManager, config);
        // Resources res = context.getResources();
        // int photoModuleId = context.getResources().getInteger(R.integer.camera_mode_photo);
        // registerPhotoModule(moduleManager, photoModuleId, SettingsScopeNamespaces.PHOTO,
        // config.isUsingCaptureModule());
        // moduleManager.setDefaultModuleIndex(photoModuleId);
        // registerVideoModule(moduleManager, res.getInteger(R.integer.camera_mode_video),
        // SettingsScopeNamespaces.VIDEO);
        //
        // /*SPRD:fix bug474690 add for pano @{ */
        // if (CameraUtil.isWideAngleEnable()) {
        // registerWideAngleModule(moduleManager, context.getResources()
        // .getInteger(R.integer.camera_mode_panorama), SettingsScopeNamespaces.PANORAMA);
        // }
        // /* @} */
        //
        // if (PhotoSphereHelper.hasLightCycleCapture(context)) {
        // registerWideAngleModule(moduleManager, res.getInteger(R.integer.camera_mode_panorama),
        // SettingsScopeNamespaces.PANORAMA);
        // registerPhotoSphereModule(moduleManager,
        // res.getInteger(R.integer.camera_mode_photosphere),
        // SettingsScopeNamespaces.PANORAMA);
        // }
        // if (RefocusHelper.hasRefocusCapture(context)) {
        // registerRefocusModule(moduleManager, res.getInteger(R.integer.camera_mode_refocus),
        // SettingsScopeNamespaces.REFOCUS);
        // }
        // if (GcamHelper.hasGcamAsSeparateModule(config)) {
        // registerGcamModule(moduleManager, res.getInteger(R.integer.camera_mode_gcam),
        // SettingsScopeNamespaces.PHOTO,
        // config.getHdrPlusSupportLevel(OneCamera.Facing.BACK));
        // }
        // if (UCamUtill.isGifEnable()) {
        // registerGifModule(moduleManager, context.getResources()
        // .getInteger(R.integer.camera_mode_gif),
        // SettingsScopeNamespaces.GIF);
        // }
        // int imageCaptureIntentModuleId = res.getInteger(R.integer.camera_mode_capture_intent);
        // registerCaptureIntentModule(moduleManager, imageCaptureIntentModuleId,
        // SettingsScopeNamespaces.PHOTO, config.isUsingCaptureModule());
    }

    private static void registerPhotoModule(ModuleManager moduleManager, final int moduleId,
            final String namespace, final boolean enableCaptureModule) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent() {

            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                // The PhotoModule requests the old app camere, while the new
                // capture module is using OneCamera. At some point we'll
                // refactor all modules to use OneCamera, then the new module
                // doesn't have to manage it itself.
                return !enableCaptureModule;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app, Intent intent) {
                Log.v(TAG, "EnableCaptureModule = " + enableCaptureModule);
                return enableCaptureModule ? new CaptureModule(app) : new PhotoModule(app);
            }
        });
    }

    private static void registerVideoModule(ModuleManager moduleManager, final int moduleId,
            final String namespace) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent() {
            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                return true;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app, Intent intent) {
                return new VideoModule(app);
            }
        });
    }

    private static void registerWideAngleModule(ModuleManager moduleManager, final int moduleId,
            final String namespace) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent() {
            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                return true;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app, Intent intent) {
                return PhotoSphereHelper.createWideAnglePanoramaModule(app);
            }
        });
    }

    private static void registerPhotoSphereModule(ModuleManager moduleManager, final int moduleId,
            final String namespace) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent() {
            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                return true;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app, Intent intent) {
                return PhotoSphereHelper.createPanoramaModule(app);
            }
        });
    }

    private static void registerRefocusModule(ModuleManager moduleManager, final int moduleId,
            final String namespace) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent() {
            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                return true;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app, Intent intent) {
                return RefocusHelper.createRefocusModule(app);
            }
        });
    }

    private static void registerGcamModule(ModuleManager moduleManager, final int moduleId,
            final String namespace, final HdrPlusSupportLevel hdrPlusSupportLevel) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent() {
            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                return false;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app, Intent intent) {
                return GcamHelper.createGcamModule(app, hdrPlusSupportLevel);
            }
        });
    }

    private static void registerCaptureIntentModule(ModuleManager moduleManager,
            final int moduleId,
            final String namespace, final boolean enableCaptureModule) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent() {
            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                return !enableCaptureModule;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app, Intent intent) {
                if (enableCaptureModule) {
                    try {
                        return new CaptureIntentModule(app, intent, namespace);
                    } catch (OneCameraException ignored) {
                    }
                }
                return new PhotoModule(app);
            }
        });
    }

    private static void registerIntentVideoModule(ModuleManager moduleManager,
            final int moduleId, final String namespace) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent() {
            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                return true;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app, Intent intent) {
                return new DreamIntentVideoModule(app);
            }
        });
    }

//    private static void registerGifModule(ModuleManager moduleManager, final int moduleId,
//            final String namespace) {
//        moduleManager.registerModule(new ModuleManager.ModuleAgent() {
//            @Override
//            public int getModuleId() {
//                return moduleId;
//            }
//
//            @Override
//            public boolean requestAppForCamera() {
//                return true;
//            }
//
//            @Override
//            public String getScopeNamespace() {
//                return namespace;
//            }
//
//            @Override
//            public ModuleController createModule(AppController app, Intent intent) {
//                return new GifModule(app);
//            }
//        });
//    }

    // Dream Camera Modules
    public static void setupDreamModules(Context context, ModuleManager moduleManager,
            OneCameraFeatureConfig config) {

        Resources res = context.getResources();

        int photoModuleId = context.getResources().getInteger(R.integer.camera_mode_auto_photo);
        registerAutoPhotoModule(moduleManager, photoModuleId, SettingsScopeNamespaces.AUTO_PHOTO);
        moduleManager.setDefaultModuleIndex(photoModuleId);

        registerManualPhotoModule(moduleManager, res.getInteger(R.integer.camera_mode_manual),
                SettingsScopeNamespaces.MANUAL);

        registerContinuePhotoModule(moduleManager, res.getInteger(R.integer.camera_mode_continue),
                SettingsScopeNamespaces.CONTINUE);

        registerIntervalPhotoModule(moduleManager, res.getInteger(R.integer.camera_mode_interval),
                SettingsScopeNamespaces.INTERVAL);

        registerIntentCaptureModule(moduleManager, res.getInteger(R.integer.camera_mode_capture_intent),
                SettingsScopeNamespaces.INTENTCAPTURE);

        registerIntentVideoModule(moduleManager, res.getInteger(R.integer.camera_mode_video_intent),
                SettingsScopeNamespaces.INTENTVIDEO);

        if (CameraUtil.isWideAngleEnable()) {
            registerDreamPanoramaModule(moduleManager,
                    res.getInteger(R.integer.camera_mode_panorama),
                    SettingsScopeNamespaces.DREAM_PANORAMA);
        }

        if (RefocusHelper.hasRefocusCapture(context)) {
            registerRefocusModule(moduleManager, res.getInteger(R.integer.camera_mode_refocus),
                    SettingsScopeNamespaces.REFOCUS);
        }

        registerScenePhotoModule(moduleManager, res.getInteger(R.integer.camera_mode_scene),
                SettingsScopeNamespaces.SCENE);

        // registerPipModule(moduleManager, res.getInteger(R.integer.camera_mode_scene),
        // SettingsScopeNamespaces.PIP);

        if (GcamHelper.hasGcamAsSeparateModule(config)) {
            registerGcamModule(moduleManager, res.getInteger(R.integer.camera_mode_gcam),
                    SettingsScopeNamespaces.GCAM,
                    config.getHdrPlusSupportLevel(OneCamera.Facing.BACK));
        }

        if (UCamUtill.isGifEnable()) {
            registerGifPhotoModule(moduleManager, res.getInteger(R.integer.camera_mode_gif_photo),
                    SettingsScopeNamespaces.GIF_PHOTO);
        }

        registerAutoVideoModule(moduleManager, res.getInteger(R.integer.camera_mode_auto_video),
                SettingsScopeNamespaces.AUTO_VIDEO);

        // registerVivModule(moduleManager, res.getInteger(R.integer.camera_mode_viv),
        // SettingsScopeNamespaces.VIV);

        registerTimelapseVideoModule(moduleManager,
                res.getInteger(R.integer.camera_mode_timelapse),
                SettingsScopeNamespaces.TIMELAPSE);

        registerSlowmotionVideoModule(moduleManager,
                res.getInteger(R.integer.camera_mode_slowmotion),
                SettingsScopeNamespaces.SLOWMOTION);

        if (UCamUtill.isGifEnable()) {
            registerGifVideoModule(moduleManager, res.getInteger(R.integer.camera_mode_gif_video),
                    SettingsScopeNamespaces.GIF_VIDEO);
        }

        if (UCamUtill.isSceneryEnable()) {
            registerSceneryModule(moduleManager,
                    res.getInteger(R.integer.camera_mode_scenery),
                    SettingsScopeNamespaces.SCENERY);
        }

        if (UCamUtill.isUcamFilterEnable()) {
            registerUcamFilterPhotoModule(moduleManager,
                    res.getInteger(R.integer.camera_mode_filter),
                    SettingsScopeNamespaces.FILTER);
        }

        if (CameraUtil.isVoicePhotoEnable()) {
            registerAudioPictureModule(moduleManager,
                    res.getInteger(R.integer.camera_mode_audio_picture),
                    SettingsScopeNamespaces.AUDIO_PICTURE);
        }

        if (UCamUtill.isVgestureEnnable()) {
            registerVgestureModule(moduleManager,
                    res.getInteger(R.integer.camera_mode_vgesture),
                    SettingsScopeNamespaces.VGESTURE);
        }

        if (CameraUtil.isQrCodeEnabled()) {
            registerQrCodeModule(moduleManager,
                    res.getInteger(R.integer.camera_mode_qrcode),
                    SettingsScopeNamespaces.QR_CODE);
        }
    }

    private static void registerAutoPhotoModule(ModuleManager moduleManager, final int moduleId,
            final String namespace) {
            moduleManager.registerModule(new ModuleManager.ModuleAgent() {

                @Override
                public int getModuleId() {
                    return moduleId;
                }

                @Override
                public boolean requestAppForCamera() {
                    return true;
                }

                @Override
                public String getScopeNamespace() {
                    return namespace;
                }

                @Override
                public ModuleController createModule(AppController app, Intent intent) {
                    Log.i(TAG, "Create Module moduleId=" + moduleId + ",namespace=" + namespace);
                    return new AutoPhotoModule(app);
                }
            });
    }

    private static void registerManualPhotoModule(ModuleManager moduleManager, final int moduleId,
            final String namespace) {
            moduleManager.registerModule(new ModuleManager.ModuleAgent() {

                @Override
                public int getModuleId() {
                    return moduleId;
                }

                @Override
                public boolean requestAppForCamera() {
                    return true;
                }

                @Override
                public String getScopeNamespace() {
                    return namespace;
                }

                @Override
                public ModuleController createModule(AppController app, Intent intent) {
                    Log.i(TAG, "Create Module moduleId=" + moduleId + ",namespace=" + namespace);
                    return new ManualPhotoModule(app);
                }
            });
    }

    private static void registerContinuePhotoModule(ModuleManager moduleManager,
            final int moduleId,
            final String namespace) {
            moduleManager.registerModule(new ModuleManager.ModuleAgent() {

                @Override
                public int getModuleId() {
                    return moduleId;
                }

                @Override
                public boolean requestAppForCamera() {
                    return true;
                }

                @Override
                public String getScopeNamespace() {
                    return namespace;
                }

                @Override
                public ModuleController createModule(AppController app, Intent intent) {
                    Log.i(TAG, "Create Module moduleId=" + moduleId + ",namespace=" + namespace);
                    return new ContinuePhotoModule(app);
                }
            });
    }

    private static void registerIntervalPhotoModule(ModuleManager moduleManager,
            final int moduleId,
            final String namespace) {
            moduleManager.registerModule(new ModuleManager.ModuleAgent() {

                @Override
                public int getModuleId() {
                    return moduleId;
                }

                @Override
                public boolean requestAppForCamera() {
                    return true;
                }

                @Override
                public String getScopeNamespace() {
                    return namespace;
                }

                @Override
                public ModuleController createModule(AppController app, Intent intent) {
                    Log.i(TAG, "Create Module moduleId=" + moduleId + ",namespace=" + namespace);
                    return new IntervalPhotoModule(app);
                }
            });
    }

    private static void registerDreamPanoramaModule(ModuleManager moduleManager,
            final int moduleId,
            final String namespace) {
            moduleManager.registerModule(new ModuleManager.ModuleAgent() {
                @Override
                public int getModuleId() {
                    return moduleId;
                }

                @Override
                public boolean requestAppForCamera() {
                    return true;
                }

                @Override
                public String getScopeNamespace() {
                    return namespace;
                }

                @Override
                public ModuleController createModule(AppController app, Intent intent) {
                    Log.i(TAG, "Create Module moduleId=" + moduleId + ",namespace=" + namespace);
                    return new DreamPanoramaModule(app);
                }
            });
    }

    private static void registerScenePhotoModule(ModuleManager moduleManager, final int moduleId,
            final String namespace) {
            moduleManager.registerModule(new ModuleManager.ModuleAgent() {

                @Override
                public int getModuleId() {
                    return moduleId;
                }

                @Override
                public boolean requestAppForCamera() {
                    return true;
                }

                @Override
                public String getScopeNamespace() {
                    return namespace;
                }

                @Override
                public ModuleController createModule(AppController app, Intent intent) {
                    Log.i(TAG, "Create Module moduleId=" + moduleId + ",namespace=" + namespace);
                    return new ScenePhotoModule(app);
                }
            });
    }

    private static void registerGifPhotoModule(ModuleManager moduleManager, final int moduleId,
            final String namespace) {
            moduleManager.registerModule(new ModuleManager.ModuleAgent() {
                @Override
                public int getModuleId() {
                    return moduleId;
                }

                @Override
                public boolean requestAppForCamera() {
                    return true;
                }

                @Override
                public String getScopeNamespace() {
                    return namespace;
                }

                @Override
                public ModuleController createModule(AppController app, Intent intent) {
                    return new GifPhotoModule(app);

                }
            });
    }

    private static void registerAutoVideoModule(ModuleManager moduleManager, final int moduleId,
            final String namespace) {
            moduleManager.registerModule(new ModuleManager.ModuleAgent() {

                @Override
                public int getModuleId() {
                    return moduleId;
                }

                @Override
                public boolean requestAppForCamera() {
                    return true;
                }

                @Override
                public String getScopeNamespace() {
                    return namespace;
                }

                @Override
                public ModuleController createModule(AppController app, Intent intent) {
                    Log.i(TAG, "Create Module moduleId=" + moduleId + ",namespace=" + namespace);
                    return new AutoVideoModule(app);
                }
            });
    }

    private static void registerTimelapseVideoModule(ModuleManager moduleManager,
            final int moduleId,
            final String namespace) {
            moduleManager.registerModule(new ModuleManager.ModuleAgent() {

                @Override
                public int getModuleId() {
                    return moduleId;
                }

                @Override
                public boolean requestAppForCamera() {
                    return true;
                }

                @Override
                public String getScopeNamespace() {
                    return namespace;
                }

                @Override
                public ModuleController createModule(AppController app, Intent intent) {
                    Log.i(TAG, "Create Module moduleId=" + moduleId + ",namespace=" + namespace);
                    return new TimelapseVideoModule(app);
                }
            });
    }

    private static void registerSlowmotionVideoModule(ModuleManager moduleManager,
            final int moduleId,
            final String namespace) {
            moduleManager.registerModule(new ModuleManager.ModuleAgent() {

                @Override
                public int getModuleId() {
                    return moduleId;
                }

                @Override
                public boolean requestAppForCamera() {
                    return true;
                }

                @Override
                public String getScopeNamespace() {
                    return namespace;
                }

                @Override
                public ModuleController createModule(AppController app, Intent intent) {
                    Log.i(TAG, "Create Module moduleId=" + moduleId + ",namespace=" + namespace);
                    return new SlowmotionVideoModule(app);
                }
            });
    }

    private static void registerGifVideoModule(ModuleManager moduleManager, final int moduleId,
            final String namespace) {
            moduleManager.registerModule(new ModuleManager.ModuleAgent() {
                @Override
                public int getModuleId() {
                    return moduleId;
                }

                @Override
                public boolean requestAppForCamera() {
                    return true;
                }

                @Override
                public String getScopeNamespace() {
                    return namespace;
                }

                @Override
                public ModuleController createModule(AppController app, Intent intent) {
                    return new GifVideoModule(app);
                }
            });
    }

    /* SPRD: Fix 474859 Add new Feature of scenery @{ */
    private static void registerSceneryModule(ModuleManager moduleManager, final int moduleId,
            final String namespace) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent() {
            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                return true;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app, Intent intent) {
                return new DreamSceneryModule(app);
            }
        });
    }
    /* @} */

    private static void registerUcamFilterPhotoModule(ModuleManager moduleManager, final int moduleId,final String namespace) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent()  {
            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                return true;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app, Intent intent) {
                return new DreamFilterModule(app);
            }
        });
    }

    private static void registerIntentCaptureModule(ModuleManager moduleManager,
            final int moduleId, final String namespace) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent() {
            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                return true;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app, Intent intent) {
                return new DreamIntentCaptureModule(app);
            }
        });
    }


    private static void registerAudioPictureModule(ModuleManager moduleManager,
            final int moduleId, final String namespace) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent() {
            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                return true;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app,
                    Intent intent) {
                return new AudioPictureModule(app);
            }
        });
    }

    private static void registerVgestureModule(ModuleManager moduleManager,
            final int moduleId, final String namespace) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent() {
            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                return true;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app,
                    Intent intent) {
                return new VgesturePhotoModule(app);
            }
        });
    }

    private static void registerQrCodeModule(ModuleManager moduleManager,
            final int moduleId, final String namespace) {
        moduleManager.registerModule(new ModuleManager.ModuleAgent() {
            @Override
            public int getModuleId() {
                return moduleId;
            }

            @Override
            public boolean requestAppForCamera() {
                return true;
            }

            @Override
            public String getScopeNamespace() {
                return namespace;
            }

            @Override
            public ModuleController createModule(AppController app,
                    Intent intent) {
                return new QrCodePhotoModule(app);
            }
        });
    }
}
