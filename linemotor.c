#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <termios.h>

//各自の使用するボードで変更
/**************************/
#define OCP_NUM 3 //ocp.▲の▲に該当する番号
#define PWM_PERIOD 10000000
#define BONE_CAPEMGR_NUM 9 //bone_capemgr.●の●に該当
/**************************/

//各自の接続に応じて変更
/**************************/
char PIN_PWM[2][7]={{"P9_14"},{"P9_22"}}; //PWM有効化後の番号
int pwm_pin_num[2]={15,16}; //PWMに使用するのBBBピン番号
int motor_gpio_num[2][2]={{61,60},{65,46}}; //モータで使用するGPIO番号

int gpio_num[4] = {49,115,27,47};
/*************************/

void gpio_export(int n);	//gpioの有効化関数
void gpio_unexport(int n); //gpioの有効化解除の関数
int gpio_open(int n, char *file, int flag);	//gpioの設定ファイルを開く関数
void init_pwm(int motor_num); //PWM初期化関数 motor_num=0もしくは1
void run_pwm(int motor_num,int duty,int drive_mode); //モータ用出力関数 motor_num=0もしくは1/drive_mode 0:停止，1:正転，-1:逆転
void close_pwm(int motor_num); //PWM終了関数
int kbhit(void); //キー入力関数
int line(int gpio_num);//ライントレーサ関数

/*********************************/
//init_pwm(モータ番号)を呼び出して，初期化の設定を行う，モータ番号（0～接続個数-1）
//run_pwm(int motor_num,int duty,int drive_mode)を呼びだして動かす．motor_num：モータ番号/duty：整数値/drive_mode：停止，正転，逆転
//close_pwm(モータ番号)を呼び出して，終了の処理を実施
/*********************************/


//このプログラムは，モータ0番が常に停止している状態
//モータ1番に関しては何もしていない

int main(){
	int i;
    int s[4];
	
	init_pwm(0);
	init_pwm(1);

    s[2] = 0;
    s[1] = 0;

	while(1){

        //ライントレーサ関数の呼び出し
        /*
        for(i = 0; i < 4; i++){
            s[i] = line(gpio_num[i]);
        }
        */


	   /*
		ライントレーサ（４つ使用）
		┌───────────────────────────┐
		│・　・　・　・　・　・　・　・│
		└───────────────────────────┘
				4	3	2	1
	   */

        //条件による走行
        if(s[2] == 1 && s[1] == 1){   		//センサ２とセンサ３(真ん中のセンサ)が２つとも白判定
            run_pwm(0,10000000,1);			//モータ２を正転（前に進行）
			run_pwm(1,10000000,-1);			//モータ１を逆転（前に進行）
            printf("前進\n");
        }else if(s[2] == 1 && s[1] == 0){	//センサ２が黒判定、センサ３が白判定
            run_pwm(1,9000000,-1);			//モータ２を逆転（左に旋回）
            printf("左旋回\n");
        }else if(s[2] == 0 && s[1] == 1){	//センサ２が白判定、センサ３が黒判定
            run_pwm(0,9000000,1);			//モータ１を正転（右に旋回）
            printf("右旋回\n");
        }else{								
            run_pwm(0,0,0);					//モータ１を停止
            run_pwm(1,0,0);					//モータ２を停止
            printf("停止中\n");	
        }      


		//キー入力関数
		if(kbhit()) {
			if(getchar()=='q')	//「q」で終了
				break;
		}
	}
	
	run_pwm(0,0,0);		//モータ１を停止
	run_pwm(1,0,0);	    //モータ２を停止

	//PWMの終了
	close_pwm(0);
	close_pwm(1);	

	return 0;
	
}


//ライントレース用関数
int line(int gpio_num){
    int fd;
    int i;
    char c;
    
    //GPIOの有効化
    gpio_export(gpio_num);

    //directionへinの書き込み
    fd=gpio_open(gpio_num, "direction", O_WRONLY);
    write(fd, "in", 2);
    close(fd);

    //読み取る
    // \r    

    while(1){
        fd = gpio_open(gpio_num, "value", O_RDONLY);
        read(fd, &c, 1);

        if(c == '1'){
            printf("GPIO番号%dの判定は白\n",&gpio_num);
            return 1;
        }else{
            printf("GPIO番号%dの判定は黒\n",&gpio_num);
            return 0;
        }  
    }
    close(fd);

    //GPIOの無効化
    for(i = 0; i < 4; i++){
        gpio_unexport(gpio_num);
    }
    
}


//PWM初期化関数
void init_pwm(int motor_num){
	int i,fd;
	char path[60],path3[60],path4[60];
	FILE *fp;
	for(i=0;i<2;i++){
		gpio_export(motor_gpio_num[motor_num][i]);
		
		fd=gpio_open(motor_gpio_num[motor_num][i], "direction", O_WRONLY);
		write(fd, "out", 3);
		close(fd);

		sprintf(path3, "/sys/class/gpio/gpio%d/value", motor_gpio_num[motor_num][i]);
		fp = fopen(path3, "w");
		fprintf(fp, "%d", 0);
		fclose(fp);
	}

	/*PWM機能の有効化*/
	
	sprintf(path4, "/sys/devices/bone_capemgr.%d/slots", BONE_CAPEMGR_NUM);
	fp = fopen(path4,"w");
	fprintf(fp, "am33xx_pwm");
	fclose(fp);

	/*ピンの設定（PIN_PWM指定のピン）*/
	sprintf(path, "bone_pwm_%s",PIN_PWM[motor_num]);
	sprintf(path4, "/sys/devices/bone_capemgr.%d/slots", BONE_CAPEMGR_NUM);
	fp = fopen(path4,"w");
	fprintf(fp, path);
	fclose(fp);

	/*安全のため，PWM出力の停止*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/run",OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp = fopen(path, "wb");
	fprintf(fp, "%d", 0);
	fclose(fp);

	/*PWM周期の設定*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/period",OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp = fopen(path, "wb");
	fprintf(fp, "%d", PWM_PERIOD);
	fclose(fp);

	/*PWM極性の設定*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/polarity",OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp=fopen(path, "wb");
	fprintf(fp, "%d", 0);
	fclose(fp);

	/*PWM　ON状態時間の初期化*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/duty",OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp=fopen(path, "wb");
	fprintf(fp, "%d", 0);
	fclose(fp);

	/*PWM出力の開始*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/run",OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp=fopen(path, "wb");
	fprintf(fp, "%d", 1);
	fclose(fp);

}

//PWM終了関数
void close_pwm(int motor_num){
	FILE *fp;
	char path[60];
	int i;
	
	/*PWM　duty0出力*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/duty",OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp=fopen(path, "wb");
	fprintf(fp, "%d", 0);
	fclose(fp);
	
	/*PWM出力の停止*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/run",OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp=fopen(path, "wb");
	fprintf(fp, "%d", 0);
	fclose(fp);

	//GPIOの解放
	for(i=0;i<2;i++){
		gpio_unexport(motor_gpio_num[motor_num][i]);
	}
}

//モータ用出力関数
void run_pwm(int motor_num,int duty,int drive_mode){
	int i;
	char path[60],path3[60];
	FILE *fp;

	//一時停止
	if(drive_mode==0){
		for(i=0;i<2;i++){
			sprintf(path3, "/sys/class/gpio/gpio%d/value", motor_gpio_num[motor_num][i]);
			fp = fopen(path3, "w");
			fprintf(fp, "%d", 1);
			fclose(fp);
		}
	}
	
	//モータ正転
	else if(drive_mode==1){
		for(i=0;i<2;i++){
			sprintf(path3, "/sys/class/gpio/gpio%d/value", motor_gpio_num[motor_num][i]);
			fp = fopen(path3, "w");
			if(i==0){
				fprintf(fp, "%d", 1);
				fclose(fp);
			}
			else{
				fprintf(fp, "%d", 0);
				fclose(fp);
			}
		}

	}

	//モータ逆転
	else if(drive_mode==-1){
		for(i=0;i<2;i++){
			sprintf(path3, "/sys/class/gpio/gpio%d/value", motor_gpio_num[motor_num][i]);
			fp = fopen(path3, "w");
			if(i==0){
				fprintf(fp, "%d", 0);
				fclose(fp);
			}
			else{
				fprintf(fp, "%d", 1);
				fclose(fp);
			}
		}
	}

	//入力したdutyでPWM信号を出力
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/duty",OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp=fopen(path, "wb");
	fprintf(fp, "%d", duty);
	fclose(fp);
	usleep(200);
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

int kbhit(void){
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}


