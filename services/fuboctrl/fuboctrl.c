/*
 * Copyright (c) 2015 by Martin Loidl <martin.loidl@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "fuboctrl.h"

#include "protocols/mqtt/mqtt.h"
#include "protocols/ecmd/ecmd-base.h"


#define MQTT_TOPIC_PREFIX	"home/hk2"
#define MQTT_TEMP_TOPIC	MQTT_TOPIC_PREFIX "/temp/%16s"
#define MQTT_CHANNEL_TOPIC	MQTT_TOPIC_PREFIX "/io/#"
#define MQTT_CHANNEL_FORMAT	MQTT_TOPIC_PREFIX "/io/%i"

#define TOPIC_LENGTH	(sizeof(MQTT_TOPIC_PREFIX) + 23)

void
fuboctrl_publish_cb(char const *topic, uint16_t topic_length,
					const void *payload, uint16_t payload_length)
{
	FUBOCTRLDEBUG("MQTT Publish: Len(%d) '%s'\n", topic_length, topic);

	if (topic_length < 13) {
		return;
	}

	uint8_t channel = 0;
	uint8_t	value = 0;
	uint8_t ret;
  	char strvalue[2];
  	char *my_topic = NULL;

  	my_topic = malloc(topic_length + 1);
  	if (my_topic == NULL) {
		FUBOCTRLDEBUG("malloc (%i) failed", topic_length + 1);
	  	goto cleanup;
	}

  	memcpy(my_topic, topic, topic_length);
  	my_topic[topic_length] = '\0';
	ret = sscanf_P(my_topic, PSTR(MQTT_CHANNEL_FORMAT), &channel);
	if (ret == 1) {
		if (channel < 1 || channel > 8) {
			goto cleanup;
		}

		memcpy(strvalue, payload, 1);
		strvalue[1] = '\0';
		sscanf_P(strvalue, PSTR("%hhu"), &value);

		if (value != 0 && value != 1) {
			goto cleanup;
		}

		FUBOCTRLDEBUG("CH %i: %i\n", channel, value);

		if (value == 1) {
			PORTC |= _BV(channel-1);
		} else if (value == 0) {
			PORTC &= ~(_BV(channel-1));
		}

	} else {
		FUBOCTRLDEBUG("MQTT Publish: wrong topic\n");
	}

cleanup:
  	if (my_topic != NULL) free (my_topic);
  	return;
}

static void
fuboctrl_connack_cb(void)
{
	FUBOCTRLDEBUG("MQTT Sub: " MQTT_CHANNEL_TOPIC "\n");
	mqtt_construct_subscribe_packet(MQTT_CHANNEL_TOPIC);
}

static const mqtt_callback_config_t mqtt_callback_config PROGMEM = {
	.connack_callback = fuboctrl_connack_cb,
	.poll_callback = NULL,
	.close_callback = NULL,
	.publish_callback = fuboctrl_publish_cb,
};

/*
	If enabled in menuconfig, this function is called during boot up of ethersex
*/
int16_t
fuboctrl_init(void)
{
	FUBOCTRLDEBUG ("init\n");

  	// set every pin on PORTC as output
  	DDRC = 0xFF;
	// enter your code here
	mqtt_register_callback(&mqtt_callback_config);

	return ECMD_FINAL_OK;
}

/*
	If enabled in menuconfig, this function is periodically called
	change "timer(500,fuboctrl_periodic)" if needed
*/
int16_t
fuboctrl_periodic(void)
{
  	// sending temperature information every 1min
  	static uint8_t runNo = 1;
  	if (runNo < 6) {
	  runNo++;
	  return ECMD_FINAL_OK;
	}

  	runNo = 1;

  	FUBOCTRLDEBUG ("periodic\n");
	// enter your code here

	char topic[TOPIC_LENGTH];
	char temp[7];

	strncpy(topic, MQTT_TOPIC_PREFIX "/temp/28481f28020000fa", TOPIC_LENGTH);
	strncpy(temp, "25.66", 7);


	mqtt_construct_publish_packet(topic, temp, 5, false);

	return ECMD_FINAL_OK;
}

/*
	This function will be called on request by menuconfig, if wanted...
	You need to enable ECMD_SUPPORT for this.
	Otherwise you can use this function for anything you like
*/
int16_t
fuboctrl_onrequest(char *cmd, char *output, uint16_t len){
	FUBOCTRLDEBUG ("main\n");
	// enter your code here

	return ECMD_FINAL_OK;
}

/*
	-- Ethersex META --
	header(services/fuboctrl/fuboctrl.h)
	ifdef(`conf_FUBOCTRL_INIT_AUTOSTART',`init(fuboctrl_init)')
	ifdef(`conf_FUBOCTRL_PERIODIC_AUTOSTART',`timer(500,fuboctrl_periodic())')
*/
