#ifndef _PARAMETERS_EX_FUN_H_
#define _PARAMETERS_EX_FUN_H_

namespace android{
namespace camera2{

status_t initializeEx(Parameters *p);
status_t setEx(Parameters *p , Parameters *validatedParams , CameraParameters2 *newParams);
status_t updateRequestEx(const Parameters *p , CameraMetadata *request);

};
};

#endif
