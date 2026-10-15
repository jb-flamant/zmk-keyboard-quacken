
if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE src/behaviors/behavior_mod_hold.c)
  target_sources(app PRIVATE src/behaviors/behavior_dead_key.c)
endif() # ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

target_sources_ifdef(CONFIG_INPUT_TRACKBALL_I2C app PRIVATE src/drivers/input/trackball_i2c.c)
