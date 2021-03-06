# Voip-App-Jack
A low-latency Voip app written in C with Jack. To compile the program, you should first install JackAudio sound server and development files from your distribution's package manager. There is Jack1 and Jack2, your development files and sound server must be the same version. Compile the program using `gcc jack_combined.c -o [nameYouWannaGive] -ljack -pthread` and run the program as `sudo ./[nameYouWanneGive] [IpAddress]`. As an example, `sudo ./jack_combined.exe 127.0.0.1` will work on the same host.

My code is a bit modified version of the following link : https://github.com/jackaudio/example-clients/blob/master/simple_client.c . I edited the code to add two callback functions, stereo output, and UDP sockets. One callback function gets data coming from microphone and the other callback function sends data to speakers. There is no separate POSIX threads since JackAudio callback functions work as threads.

I tried PulseAudio, Java Sound API and ALSA API to do the same task of sending sound over IP. Although PulseAudio and Java Sound API were much easier to use than both ALSA API and JackAudio, they were inefficient, meaning there were more than 1.5 sec of delay. I managed to do capture and playback almost in real-time with ALSA API, however, I wasn't able to get it to work between two separate processes on the same host and on different hosts.

Lastly, I came across with Jack while searching for sound programming. It is a low-latency sound server for professional audio work. The fact that it is low-lantency made it a great choice for my program, although Jack API was confusing to understand in the beginning.

My implemetation isn't a perfect one(especially for this). I probably miused Jack API and that there are tons of bugs in the program. However, it does its job well in sending and receiving sound from one host to another without much latency in LAN. I did not get a chance to try it in WAN due to my router not allowing UDP port forwarding.
