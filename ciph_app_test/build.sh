
gcc -o ciph_app_test ciph_app_test.c \
        -mavx2 -std=c11 \
        -I../include \
        -L../lib \
        -lpthread -lrt -lciph_agent

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/../lib
