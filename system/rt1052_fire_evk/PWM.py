#
# Copyright (c) 2006-2018, RT-Thread Development Team
#
# SPDX-License-Identifier: Apache-2.0
#
from machine import PWM
pwm=PWM(1,3,1000,100)
pwm.freq(2000)
pwm.duty()
pwm.deinit()