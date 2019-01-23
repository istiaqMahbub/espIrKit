from mqttClient import MqttClientThreaded
import base64
import json
import time

def serialize_instance(obj):
    d = {}
    d.update(vars(obj))
    return d

def onMqttMsgCallback():
    pass

class ir_payload():
    def __init__(self):
        self.sender_ip = "192.169.244.37"
        self.cmd = "my_cmd"
        self.id_cmd = "56"
        self.token = "0"
        self.len_data = "10"
        self.data = "AQIDBAUGBwgJCgsMDQ4="

if __name__ == "__main__":
    try:
        mqttClient = MqttClientThreaded(onMqttMsgCallback)
        mqttClient.daemon = True
        mqttClient.start()
        msg = [1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6,
               7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2,
               3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8,
               9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0,1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
               1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0,1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
               1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0,1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6,
               7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2,
               3, 4, 5, 6, 7, 8, 9, 0]
        msgArray = bytearray(msg)
        encoded = base64.b64encode(msgArray)
        payload = ir_payload()
        token = 0
        token = token+1
        payload.token = str(token)
        payload.len_data = str(len(encoded))
        payload.data = encoded
        payload_serialized = serialize_instance(payload)
        payload_json = json.dumps(payload_serialized)
        print(payload_json)
        mqttClient.publishForMe('home/02:30:12:25:AA:B6/wifi/000684CE/cmd', payload_json)
        while True:
            token = token + 1
            payload.token = str(token)
            payload.len_data = str(len(encoded))
            payload.data = encoded
            payload_serialized = serialize_instance(payload)
            payload_json = json.dumps(payload_serialized)
            print(payload_json)
            mqttClient.publishForMe('home/02:30:12:25:AA:B6/wifi/000684CE/cmd', payload_json)
            print('----------------------------\r\n\r\n')
            time.sleep(5)
    except KeyboardInterrupt, SystemExit:
        print('\n! Received keyboard interrupt, quitting threads.\n')
