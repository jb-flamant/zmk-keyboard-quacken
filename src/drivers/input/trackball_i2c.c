/*
 * I2C Trackball Input Driver
 * Polls a trackball exposed as an I2C target (jb-flamant/trackball, branch
 * feat/i2c-target) and reports its motion and buttons as input events, to be
 * consumed by a `zmk,input-listener`.
 *
 * Register map (start index written, then N bytes read with auto-increment):
 *   0x00  WHO_AM_I  0x3C
 *   0x01  BUTTONS   bit0 left, bit1 right, bit2 middle
 *   0x02  DX        int8, cleared on read
 *   0x03  DY        int8, cleared on read
 */

#define DT_DRV_COMPAT jbflamant_trackball_i2c

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(trackball_i2c, CONFIG_INPUT_LOG_LEVEL);

#define TRACKBALL_REG_WHO_AM_I 0x00
#define TRACKBALL_REG_BUTTONS  0x01
#define TRACKBALL_WHO_AM_I     0x3C
#define TRACKBALL_BUTTON_COUNT 3

struct trackball_i2c_config {
    struct i2c_dt_spec bus;
    uint32_t poll_period_ms;
};

struct trackball_i2c_data {
    const struct device *dev;
    struct k_work_delayable poll_work;
    uint8_t buttons;
};

static void trackball_i2c_poll(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct trackball_i2c_data *data = CONTAINER_OF(dwork, struct trackball_i2c_data, poll_work);
    const struct device *dev = data->dev;
    const struct trackball_i2c_config *cfg = dev->config;
    uint8_t buf[3]; /* BUTTONS, DX, DY */

    /* The trackball may be unplugged: just retry on the next period. */
    int ret = i2c_burst_read_dt(&cfg->bus, TRACKBALL_REG_BUTTONS, buf, sizeof(buf));
    if (ret != 0) {
        LOG_DBG("Read failed (%d)", ret);
        goto reschedule;
    }

    const uint8_t buttons = buf[0];
    const int8_t dx = (int8_t)buf[1];
    const int8_t dy = (int8_t)buf[2];
    const uint8_t changed = (buttons ^ data->buttons) & BIT_MASK(TRACKBALL_BUTTON_COUNT);
    const bool has_motion = (dx != 0) || (dy != 0);

    for (int i = 0; i < TRACKBALL_BUTTON_COUNT; i++) {
        if ((changed & BIT(i)) != 0) {
            const bool last = !has_motion && ((changed >> (i + 1)) == 0);
            input_report_key(dev, INPUT_BTN_0 + i, (buttons & BIT(i)) != 0, last, K_FOREVER);
        }
    }
    data->buttons = buttons;

    if (has_motion) {
        input_report_rel(dev, INPUT_REL_X, dx, false, K_FOREVER);
        input_report_rel(dev, INPUT_REL_Y, dy, true, K_FOREVER);
    }

reschedule:
    k_work_schedule(&data->poll_work, K_MSEC(cfg->poll_period_ms));
}

static int trackball_i2c_init(const struct device *dev) {
    const struct trackball_i2c_config *cfg = dev->config;
    struct trackball_i2c_data *data = dev->data;
    uint8_t who_am_i;

    if (!i2c_is_ready_dt(&cfg->bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    /* Not fatal: the trackball may be plugged or powered after the keyboard. */
    if (i2c_reg_read_byte_dt(&cfg->bus, TRACKBALL_REG_WHO_AM_I, &who_am_i) != 0) {
        LOG_WRN("Trackball not responding at 0x%02x, polling anyway", cfg->bus.addr);
    } else if (who_am_i != TRACKBALL_WHO_AM_I) {
        LOG_WRN("Unexpected WHO_AM_I 0x%02x", who_am_i);
    }

    data->dev = dev;
    k_work_init_delayable(&data->poll_work, trackball_i2c_poll);
    k_work_schedule(&data->poll_work, K_MSEC(cfg->poll_period_ms));

    return 0;
}

#define TRACKBALL_I2C_INST(n)                                                                      \
    static struct trackball_i2c_data trackball_i2c_data_##n;                                       \
    static const struct trackball_i2c_config trackball_i2c_config_##n = {                          \
        .bus = I2C_DT_SPEC_INST_GET(n),                                                            \
        .poll_period_ms = DT_INST_PROP(n, poll_period_ms),                                         \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, trackball_i2c_init, NULL, &trackball_i2c_data_##n,                    \
                          &trackball_i2c_config_##n, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,      \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(TRACKBALL_I2C_INST)
