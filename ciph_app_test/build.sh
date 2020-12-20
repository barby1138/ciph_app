
gcc -o ciph_app_test ciph_app_test.c \
        -I../libciph_agent -I../libdpdk_cryptodev_client \
        -L../lib/linux/release \
        -lpthread -lrt -lciph_agent -lmemif_client -lstdc++
