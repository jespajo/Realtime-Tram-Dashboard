cc := gcc
cc += -Wall -Werror
cc += -g3
cc += -fsanitize=address -fsanitize=undefined
dashboard: tram_dashboard.c; $(cc) -o $@ $<
