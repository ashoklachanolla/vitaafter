rm -rf /VITA_TL/RPI_MQTT/raspberry-pi-mqtt-master/program
rm -rf /VITA_TL/RPI_MQTT/raspberry-pi-mqtt-master/sample
#find . -type f -name '*.so' -delete
#find . -type f -name '*.o' -delete

cd VITA_DM/Test/
make
cd ../../
cd VITA_TL/RPI_MQTT/raspberry-pi-mqtt-master/
make
