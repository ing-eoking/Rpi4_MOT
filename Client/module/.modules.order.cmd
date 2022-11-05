cmd_/home/yckim/project/module/modules.order := {   echo /home/yckim/project/module/gpio_module.ko; :; } | awk '!x[$$0]++' - > /home/yckim/project/module/modules.order
