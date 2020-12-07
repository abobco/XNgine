while true; do 
  inotifywait -r -e modify,create,delete /home/abobco/RaspberryPi/OCV_Projects
  rsync -avze "ssh -i /home/abobco/.ssh/id_rsa" /home/abobco/RaspberryPi/OCV_Projects pi@192.168.0.182:/home/pi/shared/novfp/RaspberryPi/OCV_Projects
done
