#include <zephyr/kernel.h> //printf
#include <zephyr/drivers/uart.h> // uart
#include <zephyr/device.h> //device_is_ready

#define DEV_CONSOLE DEVICE_DT_GET(DT_CHOSEN(zephyr_console))

int main(void)
{
	printf("looking for device zephyr_console...");
	const struct device* dev = DEV_CONSOLE;

	if (!device_is_ready(dev)) {
		printf("device zephyr_console not ready!");
		return 1;
	}
	else
	{
		printf("device zephyr_console ready!");
	}

	unsigned char c;

	while (c != '1') {
		if (uart_poll_in(dev, &c) == 0) {
			uart_poll_out(dev, c);
		}
	}
	printf("ending program...");

	return 0;
}
