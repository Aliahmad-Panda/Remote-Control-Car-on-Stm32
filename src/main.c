/*
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <inttypes.h>


#define LED0_NODE DT_ALIAS(led0)


#define STACK_SIZE 1024
#define THREAD_PRIORITY 5

#define SLEEP_TIME_MS 1

K_THREAD_STACK_DEFINE(thread_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_stack_1, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_stack_2, STACK_SIZE);
struct k_thread thread_data;
struct k_thread thread_data_1;
struct k_thread thread_data_2;

#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

K_QUEUE_DEFINE(my_queue);

struct k_mutex my_mutex;

struct my_item {
    void *fifo_reserved;  // First word reserved for use by the kernel.
    int data;             // Your data.
};

void enqueue_item(int value) {
    static struct my_item item;
    item.data = value;
    k_queue_append(&my_queue, &item);
}
void dequeue_item(void) {
    struct my_item *item = k_queue_get(&my_queue, K_NO_WAIT);
    if (item != NULL) {
        printk("Dequeued item: %d\n", item->data);
    }
    else{
        printk("Queue is empty\n");
    }
}





#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif


static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int i = 0;

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_toggle_dt(&led);
    //led_state = !led_state;    
    //gpio_pin_set_dt(&led, led_state);

    printk("Button pressed \n");
}


void my_thread(void *a, void *b, void *c)
{   
    
    while(1){
        gpio_pin_toggle_dt(&led);
        enqueue_item(i);
        k_msleep(5000);
        k_mutex_lock(&my_mutex, K_FOREVER);
        i++;
        k_mutex_unlock(&my_mutex);
    }
}

void my_thread_1(void *a, void *b, void *c)
{
    while(1){
        dequeue_item();
        k_msleep(2000);
    }
}

void my_thread_2(void *a, void *b, void *c)
{
    while(1){
        k_mutex_lock(&my_mutex, K_FOREVER);
        printk("Thread 2\n");
        i++;
        k_mutex_unlock(&my_mutex);
        k_msleep(7000);
    }
}

int main(void)
{
    int ret;
    k_mutex_init(&my_mutex);
    if (!device_is_ready(led.port)) {
        printk("Error: LED device %s is not ready\n", led.port->name);
        return;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return;
    }

    if (!gpio_is_ready_dt(&button)) {
        printk("Error: button device %s is not ready\n", button.port->name);
        return 0;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
        return 0;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
        return 0;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);
    printk("Set up button at %s pin %d\n", button.port->name, button.pin);

    k_tid_t my_tid = k_thread_create(&thread_data, thread_stack, STACK_SIZE, my_thread, NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
    k_tid_t my_tid_1 = k_thread_create(&thread_data_1, thread_stack_1, STACK_SIZE, my_thread_1, NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
    k_tid_t my_tid_2 = k_thread_create(&thread_data_2, thread_stack_2, STACK_SIZE, my_thread_2, NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(my_tid, "my_thread");
    k_thread_name_set(my_tid_1, "my_thread_1");
    k_thread_name_set(my_tid_2, "my_thread_2");

    while (1) {
        //gpio_pin_toggle_dt(&led);
        //printk("Light not changed \n");
        k_msleep(1);
    }
    return 0;
}
