
make
insmod mycalc.ko

gcc -o app app.c
//gcc -o app2 app2.c
//./app2&
./app

rmmod mycalc.ko
dmesg
