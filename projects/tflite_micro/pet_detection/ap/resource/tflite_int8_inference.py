#!/usr/bin/env python3
"""Run TFLite int8 object detection on a folder of images and save visualized results."""

from __future__ import annotations

import argparse
from pathlib import Path

import cv2
import numpy as np

# try:
#     from tflite_runtime.interpreter import Interpreter
# except ImportError:
#     import tensorflow as tf

#     Interpreter = tf.lite.Interpreter
import tensorflow as tf
Interpreter = tf.lite.Interpreter
# try:
#     import yaml
# except ImportError:
#     yaml = None

import yaml
IMAGE_SUFFIXES = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}
COLORS = [(56, 56, 255), (151, 157, 255), (31, 112, 255), (29, 178, 255), (49, 210, 207)]


def default_model_path() -> Path:
    candidates = [
        Path("yolov8n_full_integer_quant.tflite"),
    ]
    for path in candidates:
        if path.exists():
            return path
    return candidates[-1]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run TFLite int8 detection and save annotated images.")
    parser.add_argument("--model", type=Path, default=default_model_path(), help="Path to the TFLite model.")
    parser.add_argument("--input", type=Path, default=Path("test_images"), help="Folder with input images.")
    parser.add_argument("--output", type=Path, default=Path("output"), help="Folder to save results.")
    parser.add_argument("--conf", type=float, default=0.3, help="Confidence threshold.")
    parser.add_argument("--iou", type=float, default=0.45, help="NMS IoU threshold.")
    return parser.parse_args()


def load_names(model_path: Path, num_classes: int) -> list[str]:
    metadata_path = model_path.parent / "metadata.yaml"
    if yaml is None or not metadata_path.exists():
        return [f"class{i}" for i in range(num_classes)]

    with open(metadata_path, "r", encoding="utf-8") as f:
        metadata = yaml.safe_load(f) or {}

    names = metadata.get("names", {})
    if isinstance(names, dict):
        return [str(names[i]) for i in sorted(names)]
    if isinstance(names, list):
        return [str(name) for name in names]
    return [f"class{i}" for i in range(num_classes)]


def letterbox(image: np.ndarray, new_h: int, new_w: int) -> tuple[np.ndarray, float, int, int]:
    h, w = image.shape[:2]
    scale = min(new_h / h, new_w / w)
    resized_h, resized_w = int(h * scale), int(w * scale)
    resized = cv2.resize(image, (resized_w, resized_h), interpolation=cv2.INTER_LINEAR)

    top = (new_h - resized_h) // 2
    left = (new_w - resized_w) // 2
    padded = np.full((new_h, new_w, 3), 114, dtype=np.uint8)
    padded[top : top + resized_h, left : left + resized_w] = resized
    return padded, scale, top, left


def preprocess(image_rgb: np.ndarray, input_details: dict) -> tuple[np.ndarray, float, int, int]:
    input_h, input_w = map(int, input_details["shape"][1:3])
    padded, scale, pad_top, pad_left = letterbox(image_rgb, input_h, input_w)
    image = padded.astype(np.float32) / 255.0

    if np.issubdtype(input_details["dtype"], np.floating):
        return image[np.newaxis], scale, pad_top, pad_left

    q_scale, q_zero = input_details["quantization"]
    quantized = np.round(image / q_scale + q_zero)
    info = np.iinfo(input_details["dtype"])
    quantized = np.clip(quantized, info.min, info.max).astype(input_details["dtype"])
    return quantized[np.newaxis], scale, pad_top, pad_left


def dequantize(output: np.ndarray, output_details: dict) -> np.ndarray:
    if np.issubdtype(output_details["dtype"], np.floating):
        return output.astype(np.float32)

    q_scale, q_zero = output_details["quantization"]
    return (output.astype(np.float32) - q_zero) * q_scale


def xywh_to_xyxy(boxes: np.ndarray) -> np.ndarray:
    converted = np.empty_like(boxes)
    converted[:, 0] = boxes[:, 0] - boxes[:, 2] / 2
    converted[:, 1] = boxes[:, 1] - boxes[:, 3] / 2
    converted[:, 2] = boxes[:, 0] + boxes[:, 2] / 2
    converted[:, 3] = boxes[:, 1] + boxes[:, 3] / 2
    return converted


def nms_per_class(
    boxes_xyxy: np.ndarray, scores: np.ndarray, conf_threshold: float, iou_threshold: float
) -> list[tuple[np.ndarray, float, int]]:
    keep = scores.max(axis=1) > conf_threshold
    if not np.any(keep):
        return []

    boxes_xyxy = boxes_xyxy[keep]
    scores = scores[keep]

    results = []
    boxes_xywh = np.column_stack(
        (
            boxes_xyxy[:, 0],
            boxes_xyxy[:, 1],
            boxes_xyxy[:, 2] - boxes_xyxy[:, 0],
            boxes_xyxy[:, 3] - boxes_xyxy[:, 1],
        )
    )

    for class_id in range(scores.shape[1]):
        class_scores = scores[:, class_id]
        class_keep = class_scores > conf_threshold
        if not np.any(class_keep):
            continue

        indices = cv2.dnn.NMSBoxes(
            boxes_xywh[class_keep].tolist(),
            class_scores[class_keep].tolist(),
            conf_threshold,
            iou_threshold,
        )
        if len(indices) == 0:
            continue

        for idx in np.array(indices).reshape(-1):
            results.append((boxes_xyxy[class_keep][idx], float(class_scores[class_keep][idx]), class_id))

    results.sort(key=lambda x: x[1], reverse=True)
    return results


def postprocess(
    output: np.ndarray,
    output_details: dict,
    input_h: int,
    input_w: int,
    scale: float,
    pad_top: int,
    pad_left: int,
    orig_h: int,
    orig_w: int,
    conf_threshold: float,
    iou_threshold: float,
) -> list[tuple[float, float, float, float, float, int]]:
    output = dequantize(output, output_details)[0]
    if output.shape[0] < output.shape[1]:
        output = output.T

    boxes = output[:, :4].copy()
    boxes[:, [0, 2]] *= input_w
    boxes[:, [1, 3]] *= input_h
    scores = output[:, 4:]

    detections = nms_per_class(xywh_to_xyxy(boxes), scores, conf_threshold, iou_threshold)

    results = []
    for box, score, class_id in detections:
        x1, y1, x2, y2 = box
        x1 = np.clip((x1 - pad_left) / scale, 0, orig_w)
        y1 = np.clip((y1 - pad_top) / scale, 0, orig_h)
        x2 = np.clip((x2 - pad_left) / scale, 0, orig_w)
        y2 = np.clip((y2 - pad_top) / scale, 0, orig_h)
        results.append((float(x1), float(y1), float(x2), float(y2), score, class_id))
    return results


def c_symbol_from_stem(stem: str) -> str:
    """Build a valid C identifier prefix from a filename stem."""
    out: list[str] = []
    for c in stem:
        if c.isalnum() or c == "_":
            out.append(c)
        else:
            out.append("_")
    s = "".join(out).strip("_") or "model_input"
    if s[0].isdigit():
        s = "k_" + s
    return s


def save_input_tensor_as_cc(
    out_path: Path,
    symbol_prefix: str,
    input_tensor: np.ndarray,
    input_details: dict,
) -> None:
    """Write flattened model input bytes as a C source file (unsigned char array + length)."""
    flat = np.ascontiguousarray(input_tensor).tobytes()
    n = len(flat)
    array_name = f"{symbol_prefix}_model_input"
    len_name = f"{array_name}_len"
    shp = input_tensor.shape
    q = input_details.get("quantization")
    qline = f"/* shape={list(shp)} dtype={input_tensor.dtype} */"
    if q is not None and q[0] not in (0, None) and not np.issubdtype(input_tensor.dtype, np.floating):
        qline = f"/* shape={list(shp)} dtype={input_tensor.dtype} quant_scale={q[0]!r} zero_point={q[1]!r} */"
    lines: list[str] = [
        "/* Auto-generated by tflite_int8_inference.py — model input tensor (preprocessed, as passed to Interpreter). */",
        "#include <stdint.h>",
        "",
        qline,
        f"const unsigned char {array_name}[] = {{",
    ]
    row: list[str] = []
    for i, b in enumerate(flat):
        row.append(f"0x{b:02X}")
        if len(row) >= 12:
            lines.append("  " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("  " + ", ".join(row) + ",")
    lines.append("};")
    lines.append("")
    lines.append(f"const unsigned int {len_name} = {n}U;")
    lines.append("")

    out_path.write_text("\n".join(lines), encoding="utf-8")


def save_detections_txt(
    out_path: Path,
    source_image_name: str,
    input_tensor: np.ndarray,
    input_details: dict,
    orig_w: int,
    orig_h: int,
    conf: float,
    iou: float,
    detections: list[tuple[float, float, float, float, float, int]],
    names: list[str],
    raw_output: np.ndarray,
    output_details: dict,
) -> None:
    """Save post-process detection list and metadata to a text file."""
    lines: list[str] = [
        f"# source_image: {source_image_name}",
        f"# model_input_shape: {list(input_tensor.shape)}",
        f"# model_input_dtype: {input_tensor.dtype}",
        f"# orig_image_size: {orig_w}x{orig_h}",
        f"# conf_threshold: {conf}",
        f"# iou_threshold: {iou}",
        f"# raw_output_shape: {list(raw_output.shape)}",
        f"# raw_output_dtype: {raw_output.dtype}",
        f"# num_detections: {len(detections)}",
        "#",
        "# x1 y1 x2 y2 score class_id class_name  (coordinates in original image pixels)",
    ]
    for x1, y1, x2, y2, score, class_id in detections:
        cname = names[class_id] if 0 <= class_id < len(names) else f"class_{class_id}"
        lines.append(
            f"{x1:.4f} {y1:.4f} {x2:.4f} {y2:.4f} {score:.6f} {class_id} {cname}"
        )
    oscale = output_details.get("quantization", (0.0, 0))
    if oscale[0]:
        lines.append(f"# output_quantization_scale: {oscale[0]}")
        lines.append(f"# output_quantization_zero_point: {oscale[1]}")
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def draw_detections(image: np.ndarray, detections: list[tuple[float, float, float, float, float, int]], names: list[str]) -> None:
    for x1, y1, x2, y2, score, class_id in detections:
        color = COLORS[class_id % len(COLORS)]
        p1 = (int(round(x1)), int(round(y1)))
        p2 = (int(round(x2)), int(round(y2)))
        cv2.rectangle(image, p1, p2, color, 2)

        label = f"{names[class_id]} {score:.2f}"
        (tw, th), baseline = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.6, 1)
        text_y = max(th + baseline + 4, p1[1])
        bg_p1 = (p1[0], text_y - th - baseline - 4)
        bg_p2 = (p1[0] + tw + 6, text_y)
        cv2.rectangle(image, bg_p1, bg_p2, color, -1)
        cv2.putText(
            image,
            label,
            (p1[0] + 3, text_y - baseline - 2),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.6,
            (255, 255, 255),
            1,
            cv2.LINE_AA,
        )


def main() -> None:
    args = parse_args()
    if not args.model.exists():
        raise FileNotFoundError(f"Model not found: {args.model}")
    if not args.input.exists():
        raise FileNotFoundError(f"Input folder not found: {args.input}")

    args.output.mkdir(parents=True, exist_ok=True)
    image_paths = sorted(path for path in args.input.iterdir() if path.suffix.lower() in IMAGE_SUFFIXES)
    if not image_paths:
        raise FileNotFoundError(f"No images found in {args.input}")

    interpreter = Interpreter(model_path=str(args.model))
    interpreter.allocate_tensors()
    input_details = interpreter.get_input_details()[0]
    output_details = interpreter.get_output_details()[0]

    output_shape = output_details["shape"]
    num_classes = int(min(output_shape[1], output_shape[2]) - 4)
    names = load_names(args.model, num_classes)
    input_h, input_w = map(int, input_details["shape"][1:3])

    for i, image_path in enumerate(image_paths, 1):
        image_bgr = cv2.imread(str(image_path))
        if image_bgr is None:
            print(f"[{i}/{len(image_paths)}] skip {image_path.name}: failed to read")
            continue

        image_rgb = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2RGB)
        input_tensor, scale, pad_top, pad_left = preprocess(image_rgb, input_details)

        interpreter.set_tensor(input_details["index"], input_tensor)
        interpreter.invoke()
        output = interpreter.get_tensor(output_details["index"])

        detections = postprocess(
            output,
            output_details,
            input_h=input_h,
            input_w=input_w,
            scale=scale,
            pad_top=pad_top,
            pad_left=pad_left,
            orig_h=image_bgr.shape[0],
            orig_w=image_bgr.shape[1],
            conf_threshold=args.conf,
            iou_threshold=args.iou,
        )
        draw_detections(image_bgr, detections, names)

        stem = image_path.stem
        sym = c_symbol_from_stem(stem)
        cc_path = args.output / f"{stem}_model_input.cc"
        txt_path = args.output / f"{stem}_output.txt"
        save_input_tensor_as_cc(cc_path, sym, input_tensor, input_details)
        save_detections_txt(
            txt_path,
            image_path.name,
            input_tensor,
            input_details,
            int(image_bgr.shape[1]),
            int(image_bgr.shape[0]),
            args.conf,
            args.iou,
            detections,
            names,
            output,
            output_details,
        )

        save_path = args.output / image_path.name
        cv2.imwrite(str(save_path), image_bgr)
        print(
            f"[{i}/{len(image_paths)}] {image_path.name}: {len(detections)} detections -> {save_path}, {cc_path.name}, {txt_path.name}"
        )


if __name__ == "__main__":
    main()
