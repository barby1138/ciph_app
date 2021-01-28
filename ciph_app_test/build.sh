
gcc -o ciph_app_test ciph_app_test.c \
        -I../include \
        -L../lib \
        -lpthread -lrt -lciph_agent -lmemif_client -lstdc++ -std=c11
