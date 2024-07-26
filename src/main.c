/*
 * Copyright (c) 2023 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <inttypes.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>


//For the thread
#define STACK_SIZE 1024
#define THREAD_PRIORITY 5

#if !DT_NODE_EXISTS(DT_NODELABEL(load_switch))
#error "Overlay for power output node not properly defined."
#endif
#if !DT_NODE_EXISTS(DT_NODELABEL(another_switch))
#error "Overlay for power output node not properly defined."
#endif

//Thread structure and stack definition.
K_THREAD_STACK_DEFINE(thread_stack, STACK_SIZE);
struct k_thread thread_data;


static const struct gpio_dt_spec load_switch =
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(load_switch), gpios, {0});

static const struct gpio_dt_spec another_switch =
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(another_switch), gpios, {0});




//Thread Function.
void thread_function(void *unused1, void *unused2, void *unused3)
{
    while (1)
    {
        gpio_pin_set_dt(&load_switch, 1);
        printf("Switching on(from thread)\n");
        k_sleep(K_MSEC(1000));
        gpio_pin_set_dt(&load_switch, 0);
        printf("Switching off(from thread)\n");
        k_sleep(K_MSEC(1000));
    }
}


int main(void)
{
	int err;

	if (!gpio_is_ready_dt(&load_switch)) {
		printf("The load switch pin GPIO port is not ready.\n");
		return 0;
	}
	if (!gpio_is_ready_dt(&another_switch)) {
		printf("The load switch pin GPIO port is not ready.\n");
		return 0;
	}
	printf("Initializing pin with inactive level.\n");
	err = gpio_pin_configure_dt(&load_switch, GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		printf("Configuring GPIO pin failed: %d\n", err);
		return 0;
	}
	err = gpio_pin_configure_dt(&another_switch, GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		printf("Configuring GPIO pin failed: %d\n", err);
		return 0;
	}

    k_tid_t my_tid = k_thread_create(&thread_data, thread_stack, STACK_SIZE, thread_function, NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(my_tid, "thread_function");

	printf("Waiting one second.\n");

	k_sleep(K_MSEC(1000));

	printf("Setting pin to active level.\n");

	err = gpio_pin_set_dt(&load_switch, 1);
	if (err != 0) {
		printf("Setting GPIO pin level failed: %d\n", err);
	}
	while (1)
	{
		gpio_pin_set_dt(&another_switch, 1);
		printk("Switching on\n");
        // printf("Switching on(printf)\n");
        k_sleep(K_MSEC(1000));
		gpio_pin_set_dt(&another_switch, 0);
        printk("Switching off\n");
        // printf("Switching off(printf)\n");
		k_sleep(K_MSEC(1000));
	}
	
	return 0;
}
