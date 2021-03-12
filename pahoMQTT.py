import paho.mqtt.client as mqtt                 #import the client1
import time

###################################################################

def on_message(client,userdata,message):
    print("message received " ,str(message.payload.decode("utf-8")))
    print("message topic=",message.topic)
    print("message qos=",message.qos)
    print("message retain flag=",message.retain)

###################################################################

broker_address="10.170.81.241" 
#broker_address="iot.eclipse.org" #use external broker
print("creating new instance")
client = mqtt.Client("P1") #create new instance
client.on_message=on_message #attach function to callback
print("connecting to broker")
client.connect(broker_address) #connect to broker
client.loop_start() #start the loop
print("Subscribing to topic","EM/datacontroller")
client.subscribe("EM/datacontroller")
print("Publishing message to topic","EM/startcontroller")
client.publish("EM/startcontroller","Detect New Controller")#publish
time.sleep(4) #wait
client.loop_stop() #stop the loop

