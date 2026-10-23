// Pet (YOLOv8n) postprocess implementation, ported from the pet_detection
// project demo. All file-scope static working buffers of the original were
// moved into the caller-provided pet_postproc_buf so instances never share
// writable state (multi-interpreter safe).

#include "tflm_pet_postprocess.h"

#include <math.h>

static float pet_box_iou_xyxy(float x1a, float y1a, float x2a, float y2a,
                              float x1b, float y1b, float x2b, float y2b)
{
    float xx1 = fmaxf(x1a, x1b);
    float yy1 = fmaxf(y1a, y1b);
    float xx2 = fminf(x2a, x2b);
    float yy2 = fminf(y2a, y2b);
    float w = fmaxf(0.f, xx2 - xx1);
    float h = fmaxf(0.f, yy2 - yy1);
    float inter = w * h;
    float aw = fmaxf(0.f, x2a - x1a);
    float ah = fmaxf(0.f, y2a - y1a);
    float bw = fmaxf(0.f, x2b - x1b);
    float bh = fmaxf(0.f, y2b - y1b);
    float u = aw * ah + bw * bh - inter;
    return (u > 1e-6f) ? (inter / u) : 0.f;
}

void pet_letterbox_params(int orig_w, int orig_h, float *scale, int *pad_top, int *pad_left)
{
    float s = (float)k_pet_input_h / (float)orig_h;
    float s2 = (float)k_pet_input_w / (float)orig_w;
    if (s2 < s) {
        s = s2;
    }
    int nh = (int)(orig_h * s);
    int nw = (int)(orig_w * s);
    *pad_top = (k_pet_input_h - nh) / 2;
    *pad_left = (k_pet_input_w - nw) / 2;
    *scale = s;
}

static float pet_clipf(float v, float lo, float hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static int pet_nms_subset(const int *idxs, int n, int class_id, float iou_thresh, int *out_idx,
                          struct pet_postproc_buf *buf)
{
    for (int i = 0; i < n; i++) {
        buf->order[i] = idxs[i];
        buf->order_score[i] = buf->scores[idxs[i]][class_id];
    }
    for (int a = 0; a < n - 1; a++) {
        for (int b = a + 1; b < n; b++) {
            if (buf->order_score[b] > buf->order_score[a]) {
                float ts = buf->order_score[a];
                buf->order_score[a] = buf->order_score[b];
                buf->order_score[b] = ts;
                int ti = buf->order[a];
                buf->order[a] = buf->order[b];
                buf->order[b] = ti;
            }
        }
    }
    int kept = 0;
    for (int i = 0; i < n; i++) {
        int ri = buf->order[i];
        float x1a = buf->boxes_xyxy[ri][0], y1a = buf->boxes_xyxy[ri][1];
        float x2a = buf->boxes_xyxy[ri][2], y2a = buf->boxes_xyxy[ri][3];
        bool ok = true;
        for (int t = 0; t < kept; t++) {
            int kj = out_idx[t];
            if (pet_box_iou_xyxy(x1a, y1a, x2a, y2a,
                                 buf->boxes_xyxy[kj][0], buf->boxes_xyxy[kj][1],
                                 buf->boxes_xyxy[kj][2], buf->boxes_xyxy[kj][3]) > iou_thresh) {
                ok = false;
                break;
            }
        }
        if (ok) {
            out_idx[kept++] = ri;
        }
    }
    return kept;
}

int pet_postprocess_yolov8_int8(const int8_t *out_buf, float out_scale, int out_zp,
                                float scale, int pad_top, int pad_left,
                                int orig_w, int orig_h,
                                struct pet_postproc_buf *buf)
{
    for (int i = 0; i < k_pet_num_candidates; i++) {
        float row[k_pet_out_channels];
        for (int k = 0; k < k_pet_out_channels; k++) {
            int8_t q = out_buf[k * k_pet_num_candidates + i];
            row[k] = ((float)q - (float)out_zp) * out_scale;
        }
        float cx = row[0] * (float)k_pet_input_w;
        float cy = row[1] * (float)k_pet_input_h;
        float w = row[2] * (float)k_pet_input_w;
        float h = row[3] * (float)k_pet_input_h;
        buf->boxes_xyxy[i][0] = cx - w * 0.5f;
        buf->boxes_xyxy[i][1] = cy - h * 0.5f;
        buf->boxes_xyxy[i][2] = cx + w * 0.5f;
        buf->boxes_xyxy[i][3] = cy + h * 0.5f;
        for (int c = 0; c < k_pet_num_classes; c++) {
            buf->scores[i][c] = row[4 + c];
        }
    }

    int nf = 0;
    for (int i = 0; i < k_pet_num_candidates; i++) {
        float m = buf->scores[i][0];
        if (buf->scores[i][1] > m) {
            m = buf->scores[i][1];
        }
        if (m > k_pet_conf_threshold) {
            buf->filt[nf++] = i;
        }
    }
    if (nf == 0) {
        return 0;
    }

    int nt = 0;

    for (int c = 0; c < k_pet_num_classes; c++) {
        int nw = 0;
        for (int j = 0; j < nf; j++) {
            int i = buf->filt[j];
            if (buf->scores[i][c] > k_pet_conf_threshold) {
                buf->work[nw++] = i;
            }
        }
        if (nw == 0) {
            continue;
        }
        int nk = pet_nms_subset(buf->work, nw, c, k_pet_iou_threshold, buf->kept_idx, buf);
        for (int t = 0; t < nk; t++) {
            int ri = buf->kept_idx[t];
            if (nt >= k_pet_num_candidates) {
                break;
            }
            buf->merge[nt].x1 = buf->boxes_xyxy[ri][0];
            buf->merge[nt].y1 = buf->boxes_xyxy[ri][1];
            buf->merge[nt].x2 = buf->boxes_xyxy[ri][2];
            buf->merge[nt].y2 = buf->boxes_xyxy[ri][3];
            buf->merge[nt].score = buf->scores[ri][c];
            buf->merge[nt].class_id = c;
            nt++;
        }
    }

    for (int a = 0; a < nt - 1; a++) {
        for (int b = a + 1; b < nt; b++) {
            if (buf->merge[b].score > buf->merge[a].score) {
                struct PetDet t = buf->merge[a];
                buf->merge[a] = buf->merge[b];
                buf->merge[b] = t;
            }
        }
    }

    int nout = 0;
    for (int i = 0; i < nt && nout < k_pet_max_dets; i++) {
        float x1 = pet_clipf((buf->merge[i].x1 - (float)pad_left) / scale, 0.f, (float)orig_w);
        float y1 = pet_clipf((buf->merge[i].y1 - (float)pad_top) / scale, 0.f, (float)orig_h);
        float x2 = pet_clipf((buf->merge[i].x2 - (float)pad_left) / scale, 0.f, (float)orig_w);
        float y2 = pet_clipf((buf->merge[i].y2 - (float)pad_top) / scale, 0.f, (float)orig_h);
        buf->dets[nout].x1 = x1;
        buf->dets[nout].y1 = y1;
        buf->dets[nout].x2 = x2;
        buf->dets[nout].y2 = y2;
        buf->dets[nout].score = buf->merge[i].score;
        buf->dets[nout].class_id = buf->merge[i].class_id;
        nout++;
    }
    return nout;
}
