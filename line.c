#include <stdio.h>
#include <unistd.h>


void gpio_export(int n);	//gpioの有効化関数
void gpio_unexport(int n); //gpioの有効化解除の関数
int gpio_open(int n, char *file, int flag);	//gpioの設定ファイルを開く関数

//GPIO1_17 P9 23  (49) 
//GPIO3_19 P9 27  (115)
//GPIO0_27 P8 17  (27)
//GPIO1_15 P8 15  (47)

int gpio_num[4] = {49,115,27,47};

int main(){
    int fd;
    int i;
    char c;
    

    //GPIOの有効化
    gpio_export(gpio_num[i]);

    //directionへinの書き込み
    fd=gpio_open(gpio_num[i], "direction", O_WRONLY);
    write(fd, "in", 2);
    close(fd);

    //読み取る
    while(1){

        for(i = 0; i < 4; i++){
            fd = gpio_open(gpio_num[i], "value", O_RDONLY);
            read(fd, &c, 1);

            if(c == '1'){
                printf("GPIO番号%dの判定は白\n",i);
            }else{
                printf("GPIO番号%dの判定は黒\n",i);
            }
        }
        
    }

    close(fd);

    //GPIOの無効化
    for(i = 0; i < 4; i++){
        gpio_unexport(gpio_num[i]);
    }
    
}
//gpioの有効化関数
void gpio_export(int n){
	int fd;
	char buf[40];

	sprintf(buf, "%d", n);

	fd = open("/sys/class/gpio/export", O_WRONLY);
	write(fd, buf, strlen(buf));
	close(fd);
}

//gpioの有効化解除の関数
void gpio_unexport(int n){
	int fd;
	char buf[40];

	sprintf(buf, "%d", n);

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	write(fd, buf, strlen(buf));
	close(fd);
}

//gpioの設定ファイルを開く関数
int gpio_open(int n, char *file, int flag){
	int fd;
	char buf[40];

	sprintf(buf, "/sys/class/gpio/gpio%d/%s", n, file);

	fd = open(buf, flag);
	return fd;
}