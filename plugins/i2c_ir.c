#include "lirc_driver.h"
#include <stdio.h>


static int i2cir_init() {
    /* Open device/hardware */
    send_buffer_init();
    return 1;
}

static int i2cir_deinit(void) {
    /* close device */
    return 1;
}

static int i2cir_send(struct ir_remote* remote, struct ir_ncode* code) {
    int result = send_buffer_put(remote, code);
    if (!result)
        return 0;

    FILE* test_file = fopen("/home/pi/i2c-ir.log", "a");
    if (test_file) {
        int buffer_len = send_buffer_length();
        const lirc_t* buffer_data = send_buffer_data();

        fprintf(test_file, "Buffer: %d items\n", buffer_len);

        int i;
        for (i = 0; i < buffer_len; i++) {
            fprintf(test_file, "  %d\n", buffer_data[i]);
        }

        fclose(test_file);
    }

    /* Payload signal is now available in global variable send_buf. */
    /* Process sendbuf (see transmit.h). */
    return 1;
}

static int i2cir_drvctl(unsigned int cmd, void *arg) {
    /* Do something device specific, e.g.
       return ioctl(drv.fd, cmd, arg); */
    return cmd == 42 ? 1 : 0;
}

struct driver i2cir_driver = {
	.name		    = "i2c-ir",
	.device		    = NULL,
	.features	    = LIRC_CAN_SEND_PULSE,
	.send_mode	    = LIRC_MODE_PULSE,
	.rec_mode	    = 0,
	.code_length	= 0,
	.init_func	    = i2cir_init,
	.deinit_func	= i2cir_deinit,
	.open_func	    = default_open,
	.close_func	    = default_close,
	.send_func	    = i2cir_send,
	.rec_func	    = NULL,
	.decode_func	= NULL,
	.drvctl_func	= i2cir_drvctl,
	.readdata	    = NULL,
	.api_version	= 2,
	.driver_version = "0.9.2",
	.info		    = "No info available"
};

const struct driver* hardwares[] = { &i2cir_driver, NULL };

