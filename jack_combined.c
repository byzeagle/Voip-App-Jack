#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/*
*	Author: Ugur Buyukdurak
*	Copyright 2020
*/

#include <jack/jack.h>

#define PORT 8080

int create_UDP_socket_send(struct sockaddr_in * server, const char * ip)
{
    int sock;
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket");
        exit(EXIT_FAILURE);
    }

    memset(server, 0, sizeof(*server));
    server->sin_family = AF_INET;
    server->sin_port = htons(PORT);
    server->sin_addr.s_addr = inet_addr(ip);

    return sock;
}

int create_UDP_socket_receive()
{
    struct sockaddr_in server;
    int sock;
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket");
        exit(EXIT_FAILURE);
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    return sock;
}

typedef struct{
	int socket;
	struct sockaddr_in server;
} __Info__;

jack_port_t *input_port;
jack_port_t *output_port1;
jack_port_t *output_port2;
jack_client_t *client1;
jack_client_t *client2;

int processFirst (jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *in, *out;

	__Info__ * info = (__Info__ *) arg;
	
	in = jack_port_get_buffer (input_port, nframes);
	sendto(info->socket, in, sizeof(jack_default_audio_sample_t) * nframes, 0, (struct sockaddr *)&(info->server), sizeof(info->server));

	return 0;
}

int processSecond (jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *out1, *out2;
	
	int * socket = (int *)arg;

	out1 = jack_port_get_buffer (output_port1, nframes);
	out2 = jack_port_get_buffer (output_port2, nframes);
	unsigned char buffer[sizeof(jack_default_audio_sample_t) * nframes];
	recvfrom(*socket, buffer, sizeof(jack_default_audio_sample_t) * nframes, 0, NULL, NULL);
	memcpy(out1, buffer, sizeof(jack_default_audio_sample_t) * nframes);
	memcpy(out2, buffer, sizeof(jack_default_audio_sample_t) * nframes);

	return 0;
}

void jack_shutdown (void *arg)
{
	exit (1);
}

int main (int argc, char *argv[])
{
	const char **ports;
	const char *client_name1 = "simple1";
	const char *client_name2 = "simple2";
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;

	client1 = jack_client_open (client_name1, options, &status, server_name);
	if (client1 == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}

	client2 = jack_client_open (client_name2, options, &status, server_name);
	if (client2 == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}

	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}

	if (status & JackNameNotUnique) {
		client_name1 = jack_get_client_name(client1);
		fprintf (stderr, "unique name `%s' assigned\n", client_name1);
	}

	struct sockaddr_in server;
	int socket = create_UDP_socket_send(&server, argv[1]);
	__Info__ info = {
		socket, 
		server
	};

	int socket2 = create_UDP_socket_receive();

	jack_set_process_callback (client1, processFirst, &info);
	jack_set_process_callback (client2, processSecond, &socket2);

	jack_on_shutdown (client1, jack_shutdown, 0);
	jack_on_shutdown (client2, jack_shutdown, 0);

	printf ("engine sample rate: %" PRIu32 "\n", jack_get_sample_rate (client1));
	printf ("engine sample rate: %" PRIu32 "\n", jack_get_sample_rate (client2));

	input_port = jack_port_register (client1, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	output_port1 = jack_port_register (client2, "output1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	output_port2 = jack_port_register (client2, "output2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 1);

	if ((input_port == NULL) || (output_port1 == NULL) || (output_port2 == NULL)) {
		fprintf(stderr, "no more JACK ports available\n");
		exit (1);
	}

	if (jack_activate (client1)) {
		fprintf (stderr, "cannot activate client");
		exit (1);
	}

	if (jack_activate (client2)) {
		fprintf (stderr, "cannot activate client2");
		exit (1);
	}

	ports = jack_get_ports (client1, NULL, NULL, JackPortIsPhysical | JackPortIsOutput);
	if (ports == NULL) {
		fprintf(stderr, "no physical capture ports\n");
		exit (1);
	}

	if (jack_connect (client1, ports[0], jack_port_name (input_port))) {
		fprintf (stderr, "cannot connect input ports\n");
	}

	free (ports);
	
	ports = jack_get_ports (client2, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		exit (1);
	}

	if (jack_connect (client2, jack_port_name (output_port1), ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	if (jack_connect (client2, jack_port_name (output_port2), ports[1])) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	sleep (-1);

	jack_client_close (client1);
	jack_client_close (client2);
	exit (0);
}
