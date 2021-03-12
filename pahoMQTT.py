import paho.mqtt.client as mqtt #import the client1
broker_address="10.170.81.241" 
#broker_address="iot.eclipse.org" #use external broker
client = mqtt.Client("P1") #create new instance
client.connect(broker_address) #connect to broker
client.publish("EM/startcontroller","Detect New Controller")#publish
print("Subscribing to topic","EM/datacontroller")
client.subscribe("EM/datacontroller")
