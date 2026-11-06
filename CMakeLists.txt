
if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE src/behaviors/behavior_mod_hold.c)
  target_sources(app PRIVATE src/behaviors/behavior_dead_key.c)
endif() # ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

target_sources_ifdef(CONFIG_INPUT_TRACKBALL_I2C app PRIVATE src/drivers/input/trackball_i2c.c)

# Uses ZMK's private kscan GPIO helpers (kscan_gpio.h), compiled with ZMK_KSCAN_GPIO_DRIVER.
if (CONFIG_QUACKEN_KSCAN_GPIO_DIRECT_RESILIENT)
  target_sources(app PRIVATE src/drivers/kscan/kscan_gpio_direct_resilient.c)
  target_include_directories(app PRIVATE ${APPLICATION_SOURCE_DIR}/module/drivers/kscan)
endif()
