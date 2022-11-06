from absl import flags
import sys
FLAGS = flags.FLAGS
FLAGS(sys.argv)

import socket
import keyboard
from multiprocessing import Process, Manager, Value
import random
import torch
import cv2
import tensorflow as tf
import numpy as np
import time
from deep_sort import preprocessing
from deep_sort import nn_matching
from deep_sort.detection import Detection
from deep_sort.tracker import Tracker
from tools import generate_detections as gdet
import matplotlib.pyplot as plt


IP = ""
UDP_PORT = [12345, 9931]
MOT_PORT = 22334
TCP_PORT = []
WIDTH = 320
HEIGHT = 240
DSIZE = 57600
SSIZE = 4
UNIT = WIDTH/10

max_cosine_distance = 0.5
nn_budget = None
nms_max_overlap = 0.8

def mult_udp(num, s):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((IP, UDP_PORT[num]))
    while (True):
        data, addr = sock.recvfrom(DSIZE + 1)
        if data[0] > SSIZE - 1: continue
        s[data[0]] = data[1:(DSIZE + 1)]
def control_key(k):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((IP, MOT_PORT))
    while(True):
        print("Ready to receive Motor Signal..!")
        data, addr = sock.recvfrom(1)
        print("Receive success! Device is connected")
        #temp = 0
        loop = True
        while (loop):
            CMD = 0
            if keyboard.is_pressed("up"):
                CMD = CMD | 8
            if keyboard.is_pressed("down"):
                CMD = CMD | 4
            if keyboard.is_pressed("left"):
                CMD = CMD | 2
            if keyboard.is_pressed("right"):
                CMD = CMD | 1
            if keyboard.is_pressed("="):
                CMD = CMD | 16
            if keyboard.is_pressed("-"):
                CMD = CMD | 32
            if keyboard.is_pressed("esc"):
                CMD = CMD | 128
                loop = False
            if(CMD == 0 and k[0] != 0):
                CMD = CMD | k[0]
            sock.sendto(CMD.to_bytes(1, byteorder="little"), addr)
            sock.sendto(CMD.to_bytes(1, byteorder="little"), addr)
            time.sleep(0.05 + 0.125*(k[0] & 15))
            if(not loop):
                sock.sendto(CMD.to_bytes(1, byteorder="little"), addr)
            #if (temp != CMD):
             #   sock.sendto(CMD.to_bytes(1, byteorder="little"), addr)
              #  sock.sendto(CMD.to_bytes(1, byteorder="little"), addr)
            #temp = CMD
        print("Device is unconnected.")
def convert_boxes(image, boxes):
    returned_boxes = []
    for box in boxes:
        box[0] = (box[0] * image.shape[1]).astype(int)
        box[1] = (box[1] * image.shape[0]).astype(int)
        box[2] = (box[2] * image.shape[1]).astype(int)
        box[3] = (box[3] * image.shape[0]).astype(int)
        box[2] = int(box[2]-box[0])
        box[3] = int(box[3]-box[1])
        box = box.astype(int)
        box = box.tolist()
        if box != [0,0,0,0]:
            returned_boxes.append(box)
    return returned_boxes
def transform_images(x_train, size):
    x_train = tf.image.resize(x_train, (size, size))
    x_train = x_train / 255
    return x_train

priority = {'person' : 0,
            'dog' : 1,
            'cat' : 1}
chase = 0
chase_pri = 2
if __name__ == '__main__':
    manager = Manager()
    k = manager.list([0])
    nowkey = Process(target=control_key, args=(k,))
    nowkey.start()
    model = torch.hub.load('yolov5', 'yolov5s', source='local', pretrained='true')
    model_filename = 'model_data/mars-small128.pb'
    encoder = gdet.create_box_encoder(model_filename, batch_size=1)
    metric = nn_matching.NearestNeighborDistanceMetric('cosine', max_cosine_distance, nn_budget)
    tracker = Tracker(metric)
    # labels = classes, scores[0] = labels[:,4], boxes[0] = labels[:,0:4]
    from _collections import deque
    pts = [deque(maxlen=30) for _ in range(1000)]
    s = manager.list([b'\xff' * DSIZE for x in range(SSIZE)])
    fourcc = cv2.VideoWriter_fourcc(*'DIVX')
    order = 0
    udp1 = Process(target=mult_udp, args=(0, s,))
    udp2 = Process(target=mult_udp, args=(1, s,))
    udp1.start()
    udp2.start()

    tab = False
    cur = True
    pre = False
    while True:
        cur = False
        picture = b''
        for i in range(SSIZE):
            picture += s[i]
        frame = np.frombuffer(picture, dtype=np.uint8)
        frame = frame.reshape(HEIGHT, WIDTH, 3)

        if keyboard.is_pressed("tab"):
            cur = True
            k[0] = 0
        if cur and not pre:
            tab = not tab
        if tab:
            re = model([frame])
            labels, cord = re.xyxyn[0][:, -1].cpu().numpy(), re.xyxyn[0][:, :-1].cpu().numpy()
            boxes, scores = [cord[:, 0:4]], [cord[:, 4]]

            names = []
            for i in range(len(labels)):
                names.append(model.names[int(labels[i])])
            names = np.array(names)
            converted_boxes = convert_boxes(frame, boxes[0])
            features = encoder(frame, converted_boxes)

            detections = [Detection(bbox, score, class_name, feature) for bbox, score, class_name, feature in
                          zip(converted_boxes, scores[0], names, features)]

            boxs = np.array([d.tlwh for d in detections])
            scores = np.array([d.confidence for d in detections])
            classes = np.array([d.class_name for d in detections])
            indices = preprocessing.non_max_suppression(boxs, classes, nms_max_overlap, scores)
            detections = [detections[i] for i in indices]

            tracker.predict()
            tracker.update(detections)
            k[0] = 0
            cmap = plt.get_cmap('tab20b')
            colors = [cmap(i)[:3] for i in np.linspace(0, 1, 20)]
            cdd = [set([]), set([]), set([])]
            disap = True
            for track in tracker.tracks:
                if not track.is_confirmed() or track.time_since_update > 1:
                    continue
                bbox = track.to_tlbr()
                class_name = track.get_class()
                color = colors[int(track.track_id) % len(colors)]
                color = [i * 255 for i in color]

                cv2.rectangle(frame, (int(bbox[0]), int(bbox[1])), (int(bbox[2]), int(bbox[3])), color, 2)
                cv2.rectangle(frame, (int(bbox[0]), int(bbox[1] - 30)), (int(bbox[0]) + (len(class_name)
                                                                                       + len(str(track.track_id))) * 17,
                                                                       int(bbox[1])), color, -1)
                cv2.putText(frame, class_name + "-" + str(track.track_id), (int(bbox[0]), int(bbox[1] - 10)), 0, 0.75,
                            (255, 255, 255), 2)

                height, width, _ = frame.shape
                if track.track_id == chase:
                    if chase_pri < 0 : continue
                    disap = False
                    center_x = int(((bbox[0]) + (bbox[2])) / 2)
                    area = int((center_x*2 - WIDTH) // UNIT)
                    if center_x > int(width / 2 + UNIT):
                        #k[0] = 97
                        k[0] = area | 96
                        cv2.putText(frame, "Right", (20, 20), 0, 0.75, (0, 0, 0), 2)
                    elif center_x < int(width/2 - UNIT):
                        #k[0] = 98
                        k[0] = -area | 112
                        cv2.putText(frame, "Left", (20, 20), 0, 0.75, (0, 0, 0), 2)
                    else:
                        #k[0] = 104
                        k[0] = 96
                        cv2.putText(frame, "Front", (20, 20), 0, 0.75, (0, 0, 0), 2)
                else:
                    if class_name in priority:
                        if priority[class_name] < chase_pri :
                            chase_pri = -1
                        cdd[priority[class_name]].add(track.track_id)
                    else:
                        cdd[2].add(track.track_id)
            if disap:
                chase_pri = 2
                k[0] = 0
                for i in range(len(cdd)):
                    if len(cdd[i]) > 0:
                        chase = random.choice(tuple(cdd[i]))
                        chase_pri = i
                        break



        cv2.imshow("frame", frame)
        pre = cur
        if cv2.waitKey(1) & 0xFF == ord('q'):
            cv2.destroyAllWindows()
            udp1.terminate()
            udp2.terminate()
            nowkey.terminate()
            break
            # yol.terminate()
