from threading import Thread
import time
import datetime


class MqttClientThreaded(Thread):
    # Callbacks
    def on_connect(self, mqttClient, obj, flags, rc):
        pass

    def on_message(self, mqttClient, obj, msg):
        #print(msg.topic+" "+str(msg.qos)+" "+str(msg.payload))
        self.onMsgCallback(msg)

    def on_publish(self, mqttClient, obj, mid):
        pass

    def on_subscribe(self, mqttClient, obj, mid, granted_qos):
        pass

    def __init__(self, onMsgCallback):
        ''' Constructor. '''
        Thread.__init__(self)
        MQTT_HOST = "192.169.244.37"
        MQTT_PORT = 1883


        self.onMsgCallback = onMsgCallback

        import paho.mqtt.client as paho
        self.mqttClient = paho.Client(client_id="PID Client1", clean_session=True)

        # Set callbacks
        self.mqttClient.on_message = self.on_message
        self.mqttClient.on_connect = self.on_connect
        self.mqttClient.on_subscribe = self.on_subscribe

        # Connect and subscribe
        # print(str(self.id) + " Connecting to " +MQTT_HOST +": " +MQTT_SUB_TOPIC )
        self.mqttClient.username_pw_set("kanok", "kanok")
        self.mqttClient.connect(MQTT_HOST, MQTT_PORT, 60)

    def publishForMe(self, topic, msg):
        #print("mqtt client->publishForMe ", topic)
        self.mqttClient.publish(topic, msg, 2)

    def run(self):
        self.rc = 0
        print('running')
        while self.rc == 0:
            try:
                self.rc = self.mqttClient.loop()
            except KeyboardInterrupt:
                sys.exit(0)
