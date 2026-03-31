#ifndef __CFACEDETECTCNN_H__
#define __CFACEDETECTCNN_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#define FACE_LANDMARK_POINTS    (5)
#define FACE_LANDMARK_BUFSIZE   (FACE_LANDMARK_POINTS * 2 * sizeof(short))



int facedetectcnn_size(void);
int facedetectcnn_init(void* handle);
int facedetectcnn_deinit(void* handle);
//default (confidence,overlap)(0.5, 0.3))
int facedetectcnn_run(void* handle, unsigned char* image, int width, int height, int channels, unsigned char* facebuf, int facelimit, int landmark, float confidence, float overlap);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__CFACEDETECTCNN_H__