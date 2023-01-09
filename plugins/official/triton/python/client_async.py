import sys
import asyncio
import argparse
import numpy as np
from scipy.special import softmax
from PIL import Image
from google.protobuf import empty_pb2

import grpc
import infer_pb2
import infer_pb2_grpc
import yolo_detection

model_repo = {}

async def run_resnet50_fp32(model, args) -> None:
  pass

async def run_yolov3_bf16(model, args) -> None:
  net_w = 416
  net_h = 416
  labels_path = './data/coco.names'
  with open(labels_path, "r") as f:
    labels = [l.rstrip() for l in f]

  img_path = args.img
  if img_path == None:
    print("warning, img is None")
    exit()

  # Preprocess
  # Resize
  img = Image.open(img_path)
  resized_image = img.resize((net_w, net_h))
  img_data = np.asarray(resized_image).astype("float32")
  # Convert hwc to chw
  img_data = np.transpose(img_data, (2, 0, 1))
  # Normalization
  img_data = img_data / 255.0
  img_data = img_data.tobytes()
  # Input
  request = infer_pb2.InferRequest()
  request.name = args.model
  request.request_id = 999
  for i in range(0, model.batch_size):
    input_data = request.inputs.add()
    tensor = input_data.tensor.add()
    tensor.name = 'input'
    tensor.data = img_data

  # Inference
  async with grpc.aio.insecure_channel('localhost:50051') as channel:
    stub = infer_pb2_grpc.ModelInferStub(channel)
    response = await stub.Infer(request)

  # Postprocess
  classes = 80
  thresh = 0.5
  nms_thresh = 0.45
  print('RequestId %d, Inference Result:' % response.request_id)
  layer = [
            [3, 85, 52, 52],
            [3, 85, 26, 26],
            [3, 85, 13, 13]
          ]
  biases = [
             10.0, 13.0, 16.0, 30.0, 33.0, 23.0, 30.0, 61.0, 62.0,
             45.0, 59.0, 119.0, 116.0, 90.0, 156.0, 198.0, 373.0, 326.0
           ]
  mask = [
           [0, 1, 2],
           [3, 4, 5],
           [6, 7, 8]
         ]
  i = 0
  for output in response.outputs:
    j = 0
    print('batch[%d]' % i)
    det_out = []
    for tensor in output.tensor:
      size = layer[j][0]*layer[j][1]*layer[j][2]*layer[j][3]
      out_shape = (layer[j][0], layer[j][1], layer[j][2], layer[j][3])
      out = np.frombuffer(tensor.data, dtype=np.uint16, count=size)
      out = out.reshape(out_shape)
      out = (out.astype(np.int32) << 16).view(np.float32)
      layer_out = {}
      layer_out["type"] = "Yolo"
      layer_out["biases"] = biases
      layer_out["mask"] = mask[j]
      layer_out["output"] = out
      layer_out["classes"] = classes
      det_out.append(layer_out)
      j += 1
    dets = yolo_detection.fill_network_boxes(
        (net_w, net_h), (img.width, img.height), thresh, 1, det_out
    )
    yolo_detection.do_nms_sort(dets, classes, nms_thresh)
    yolo_detection.show_detections(img, dets, thresh, labels, classes)
    print()
    i += 1

def run(model, args):
  model_name = args.model
  if model_name == 'resnet50_fp32':
    asyncio.run(run_resnet50_fp32(model, args))
  elif model_name == 'yolov3_bf16':
    asyncio.run(run_yolov3_bf16(model, args))

def get_model_repository():
  with grpc.insecure_channel('localhost:50052') as channel:
    stub = infer_pb2_grpc.GreeterStub(channel)
    response = stub.GetModelRepository(empty_pb2.Empty())
    print("Model Repository:")
    print("------------------------------------"
          "------------------------------------")
    for model in response.model:
      print("name:%s, batch_size:%d, "
            "input num:%d, output num:%d, running:%d" %
            (model.name, model.batch_size,
            len(model.inputs), len(model.outputs), model.running))
      i = 0
      for tensor in model.inputs:
        print("input[%d], name:%s, dtype:%d\nshape: [" %
                (i, tensor.name, tensor.dtype), end='')
        for n in tensor.shape:
          print("%d " % n, end='')
        print("]")
        i += 1
      print("------------------------------------"
            "------------------------------------")
      model_repo[model.name] = model
    print()

if __name__ == '__main__':
  get_model_repository()
  parser = Argparse.argumentParser()
  parser.add_argument('--model', type=str, required=True, help="model name")
  parser.add_argument('--topn', type=int, default=3, help="top N of result")
  parser.add_argument('--img', type=str, default=None, help="image file for "
                      "the inference inputing")
  args = parser.parse_args()

  name = args.model
  if name not in model_repo:
    print("find model %s failed" % name)
    exit()
  model = model_repo[name]
  run(model, args)

