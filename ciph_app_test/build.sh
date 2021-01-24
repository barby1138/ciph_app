
gcc -o ciph_app_test_v ciph_app_test_verify.c \
        -I../include \
        -L../lib \
        -lpthread -lrt -lciph_agent -lmemif_client -lstdc++ -std=c11
