#include "lirc_driver.h"
#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#define I2C_ADDR            0x70
#define I2C_DEV             "/dev/i2c-1"
#define TIMEOUT_COUNT       30
#define TIMEOUT_DELAY_US    1000

static int i2c_fd = -1;

 // Initial I2C IR setup: 1 repeat, zero repeat delay
static uint8_t init_msg[3] = { 0x01, 0x01, 0x00 };

#define IR_STAGE_COUNT 64
typedef struct IRSEND_MSG {
    uint8_t     addr;
    uint8_t     length;
    uint16_t    timing[IR_STAGE_COUNT];
} IRSEND_MSG;

static int i2cir_deinit(void) {
    /* close device */
    if (i2c_fd >= 0) {
        close(i2c_fd);
    }
    return 1;
}

static int i2cir_init() {
    /* Open device/hardware */
    send_buffer_init();
    i2c_fd = open(I2C_DEV, O_RDWR);
    if (i2c_fd < 0) {
        LOGPRINTF(1, "I2C device open failed");
        return 0;
    }

    if (ioctl(i2c_fd, I2C_SLAVE, I2C_ADDR) < 0) {
        LOGPRINTF(1, "I2C address set failed");
        i2cir_deinit();
        return 0;
    }

    if (write(i2c_fd, init_msg, sizeof(init_msg)) != sizeof(init_msg)) {
        LOGPRINTF(1, "I2C device init failed");
        i2cir_deinit();
        return 0;
    }

    return 1;
}

static int i2cir_status() {
    uint8_t status_reg = 0;

    if (write(i2c_fd, &status_reg, 1) != 1) {
        LOGPERROR(1, "I2C status poll failed (w)");
        return -1;
    }

    if (read(i2c_fd, &status_reg, 1) != 1) {
        LOGPERROR(1, "I2C status poll failed (r)");
        return -1;
    }
    return ((int)status_reg) & 0xff;
}

static int i2cir_send(struct ir_remote* remote, struct ir_ncode* code) {
    int result = send_buffer_put(remote, code);
    if (!result)
        return 0;

    int buffer_len = send_buffer_length();
    const lirc_t* buffer_data = send_buffer_data();

    if (buffer_len < IR_STAGE_COUNT) {
        IRSEND_MSG send_msg;
        send_msg.addr = 3;
        send_msg.length = (uint8_t) buffer_len;

        int i;
        for (i = 0; i < buffer_len; i++) {
            send_msg.timing[i] = (uint16_t) buffer_data[i];
        }

        size_t send_size = 2 + buffer_len * sizeof(uint16_t); 
        if (write(i2c_fd, &send_msg, send_size) != send_size) {
            LOGPRINTF(1, "I2C device send failed");
            return 0;
        }

        useconds_t duration = (useconds_t) send_buffer_sum();
        usleep(duration);

        int timeout = TIMEOUT_COUNT;
        int status_reg = -1;

        while (timeout-- > 0) {
            status_reg = i2cir_status();
            if (!status_reg) {
                LOGPRINTF(1, "I2C-IR done: %d status timeouts", TIMEOUT_COUNT - timeout);
                return 1;
            }
            else {
                usleep(TIMEOUT_DELAY_US);
            }
        }

        LOGPRINTF(1, "I2C send timed out with no return to ready: %02x", status_reg);
        return 0;
    }
    else {
        LOGPRINTF(1, "Pulse data too long: %d items, total %d\n", buffer_len, send_buffer_sum());
        return 0;
    }

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

