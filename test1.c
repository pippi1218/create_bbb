#include <stdio.h>
#include <string.h>
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
#define LHIGH_SPEED 3500000
#define RHIGH_SPEED 3500000
#define SPEED 4000000
#define LSPEED 3000000
#define RSPEED 3100000
/**************************/

//各自の接続に応じて変更
/**************************/
char PIN_PWM[2][7] = {{"P9_14"}, {"P9_22"}};	 //PWM有効化後の番号
int pwm_pin_num[2] = {15, 16};					 //PWMに使用するのBBBピン番号
int motor_gpio_num[2][2] = {{61, 60}, {65, 46}}; //モータで使用するGPIO番号
int gpio_num[4] = {49, 115, 27, 47};
/*************************/

void gpio_export(int n);							   //gpioの有効化関数
void gpio_unexport(int n);							   //gpioの有効化解除の関数
int gpio_open(int n, char *file, int flag);			   //gpioの設定ファイルを開く関数
void init_pwm(int motor_num);						   //PWM初期化関数 motor_num=0もしくは1
void run_pwm(int motor_num, int duty, int drive_mode); //モータ用出力関数 motor_num=0もしくは1/drive_mode 0:停止，1:正転，-1:逆転
void close_pwm(int motor_num);						   //PWM終了関数
int kbhit(void);									   //キー入力関数
int line(int gpio_num);								   //ライントレーサ関数

/*********************************/
//init_pwm(モータ番号)を呼び出して，初期化の設定を行う，モータ番号（0～接続個数-1）
//run_pwm(int motor_num,int duty,int drive_mode)を呼びだして動かす．motor_num：モータ番号/duty：整数値/drive_mode：停止，正転，逆転
//close_pwm(モータ番号)を呼び出して，終了の処理を実施
/*********************************/

int main()
{
	int i;
	int s[4];
	char *lineColor;
	char *move;

	init_pwm(0);
	init_pwm(1);

	for(i = 0; i < 4; i++){
		gpio_export(gpio_num[i]);
	}

	while (1)
	{
		/*
		ライントレーサ（４つ使用）
		┌───────────────────────────┐
		│・　・　・　・　・　・　・　・│
		└───────────────────────────┘
				4	3	2	1
	   */

		//ライントレーサ関数の呼び出し
		for (i = 0; i < 4; i++)
		{
			s[i] = line(gpio_num[i]);

			if (s[i] == 1)
			{
				lineColor = "black";
			}
			else
			{
				lineColor = "white";
			}

			//条件による走行
			if (s[3] == 1 && s[2] == 0 && s[1] == 0 && s[0] == 1)//黒白白黒
			{
				run_pwm(0, LSPEED, 1);  //↑
				run_pwm(1, RSPEED, -1); //↑
				move = "go";//↑ ↑
			}
			else if (s[3] == 1 && s[2] == 0 && s[1] == 1 && s[0] == 1)//黒白黒黒
			{					
				//run_pwm(0, 0, 0); //●
				run_pwm(0, LSPEED, -1);  //↓
				run_pwm(1, RSPEED, -1); //↑
				move = "left";//● ↑
			}
			else if (s[3] == 1 && s[2] == 1 && s[1] == 0 && s[0] == 1)//黒黒白黒
			{
				run_pwm(0, LSPEED, 1); //↑
				run_pwm(1, 0, 0); //●
				move = "right";//↑ ●
			}
			else if(s[3] == 1 && s[2] == 1 && s[1] == 0 && s[0] == 0)//黒黒白白
			{
				run_pwm(0, LSPEED, 1); //↑
				run_pwm(1, 0, 0); //●
				move = "right";//↑ ●
			}
			else if(s[3] == 0 && s[2] == 0 && s[1] == 1 && s[0] == 1)//白白黒黒
			{
				run_pwm(0, 0, 0); //●
				run_pwm(1, RSPEED, -1); //↑
				move = "left";//● ↑
			}
			else if(s[3] == 0 && s[2] == 0 && s[1] == 0 && s[0] == 1)//白白白黒
			{
				run_pwm(0, LHIGH_SPEED, -1); //↓
				run_pwm(1, RHIGH_SPEED, -1); //↑
				move = "leftturn";//↓ ↑
			}
			else if(s[3] == 1 && s[2] == 0 && s[1] == 0 && s[0] == 0)//黒白白白
			{
				run_pwm(0, SPEED, -1); //↓
				run_pwm(1, SPEED, -1); //↑
				move = "leftturn";//↑ ↓
			}
			else if(s[3] == 0 && s[2] == 1 && s[1] == 1 && s[0] == 1)//白黒黒黒
			{
				//run_pwm(0, LHIGH_SPEED, -1); //↓
				//run_pwm(1, 0, 0); //●
				//move = "leftturn";//↓ ●

				run_pwm(0, SPEED, 1); //↑
				run_pwm(1, SPEED, 1); //↓
				move = "rightturn";//↑ ↓
			}
			else if(s[3] == 1 && s[2] == 1 && s[1] == 1 && s[0] == 0)//黒黒黒白
			{
				//run_pwm(0, SPEED, 1); //↑
				//run_pwm(1, SPEED, 1); //↓
				//move = "rightturn";//↑ ↓

				run_pwm(0, LHIGH_SPEED, -1); //↓
				run_pwm(1, RHIGH_SPEED, -1); //↑
				move = "leftturn";//↓ ↑
			}
			else if(s[3] == 1 && s[2] == 0 && s[1] == 1 && s[0] == 0)
			{
				run_pwm(0, LHIGH_SPEED, -1); //↓
				run_pwm(1, RHIGH_SPEED, -1); //↑
				move = "leftturn";//↓ ↑
			}
			else if(s[3] == 0 && s[2] == 1 && s[1] == 0 && s[0] == 1)
			{
				run_pwm(0, LHIGH_SPEED, 1); //↑
				//run_pwm(1, RHIGH_SPEED, 1); //↓
				run_pwm(1, 0, 0); //●
				move = "rightturn";//↑ ●
			}
			else if(s[3] == 0 && s[2] == 0 && s[1] == 0 && s[0] == 0){
				run_pwm(0, LHIGH_SPEED, 1); //↑
				//run_pwm(1, RHIGH_SPEED, 1); //↓
				run_pwm(1, 0 ,0);//●
				move = "rightturn";//↑ ●
			}
            else
			{
				run_pwm(0, 0, 0); //モータ１を停止
				run_pwm(1, 0, 0); //モータ２を停止
				move = "stop";
			}

			printf("センサ:%d 色:%s\n", i+1, lineColor);
			
		}
		printf("状態:%s\n",move);

		//キー入力関数
		if (kbhit())
		{
			if (getchar() == 'q') //「q」で終了
				break;
		}
	}

	run_pwm(0, 0, 0); //モータ１を停止
	run_pwm(1, 0, 0); //モータ２を停止

	for(i = 0;i < 4; i++){
		gpio_unexport(gpio_num[i]);
	}

	//PWMの終了
	close_pwm(0);
	close_pwm(1);

	return 0;
}

//ライントレース用関数
int line(int gpio_num)
{
	int fd;
	char c;

	fd = gpio_open(gpio_num, "direction", O_WRONLY);
	write(fd, "in", 2);
	close(fd);

	//読み取り
	fd = gpio_open(gpio_num, "value", O_RDONLY);
	read(fd, &c, 1);
	close(fd);

	if (c == '1')
	{
		//黒
		return 1;
	}
	else
	{
		//白
		return 0;
	}
}


//PWM初期化関数
void init_pwm(int motor_num)
{
	int i, fd;
	char path[60], path3[60], path4[60];
	FILE *fp;
	for (i = 0; i < 2; i++)
	{
		gpio_export(motor_gpio_num[motor_num][i]);

		fd = gpio_open(motor_gpio_num[motor_num][i], "direction", O_WRONLY);
		write(fd, "out", 3);
		close(fd);

		sprintf(path3, "/sys/class/gpio/gpio%d/value", motor_gpio_num[motor_num][i]);
		fp = fopen(path3, "w");
		fprintf(fp, "%d", 0);
		fclose(fp);
	}

	/*PWM機能の有効化*/

	sprintf(path4, "/sys/devices/bone_capemgr.%d/slots", BONE_CAPEMGR_NUM);
	fp = fopen(path4, "w");
	fprintf(fp, "am33xx_pwm");
	fclose(fp);

	/*ピンの設定（PIN_PWM指定のピン）*/
	sprintf(path, "bone_pwm_%s", PIN_PWM[motor_num]);
	sprintf(path4, "/sys/devices/bone_capemgr.%d/slots", BONE_CAPEMGR_NUM);
	fp = fopen(path4, "w");
	fprintf(fp, path);
	fclose(fp);

	/*安全のため，PWM出力の停止*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/run", OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp = fopen(path, "wb");
	fprintf(fp, "%d", 0);
	fclose(fp);

	/*PWM周期の設定*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/period", OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp = fopen(path, "wb");
	fprintf(fp, "%d", PWM_PERIOD);
	fclose(fp);

	/*PWM極性の設定*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/polarity", OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp = fopen(path, "wb");
	fprintf(fp, "%d", 0);
	fclose(fp);

	/*PWM　ON状態時間の初期化*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/duty", OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp = fopen(path, "wb");
	fprintf(fp, "%d", 0);
	fclose(fp);

	/*PWM出力の開始*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/run", OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp = fopen(path, "wb");
	fprintf(fp, "%d", 1);
	fclose(fp);
}

//PWM終了関数
void close_pwm(int motor_num)
{
	FILE *fp;
	char path[60];
	int i;

	/*PWM　duty0出力*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/duty", OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp = fopen(path, "wb");
	fprintf(fp, "%d", 0);
	fclose(fp);

	/*PWM出力の停止*/
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/run", OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp = fopen(path, "wb");
	fprintf(fp, "%d", 0);
	fclose(fp);

	//GPIOの解放
	for (i = 0; i < 2; i++)
	{
		gpio_unexport(motor_gpio_num[motor_num][i]);
	}
}

//モータ用出力関数
void run_pwm(int motor_num, int duty, int drive_mode)
{
	int i;
	char path[60], path3[60];
	FILE *fp;

	//一時停止
	if (drive_mode == 0)
	{
		for (i = 0; i < 2; i++)
		{
			sprintf(path3, "/sys/class/gpio/gpio%d/value", motor_gpio_num[motor_num][i]);
			fp = fopen(path3, "w");
			fprintf(fp, "%d", 1);
			fclose(fp);
		}
	}

	//モータ正転
	else if (drive_mode == 1)
	{
		for (i = 0; i < 2; i++)
		{
			sprintf(path3, "/sys/class/gpio/gpio%d/value", motor_gpio_num[motor_num][i]);
			fp = fopen(path3, "w");
			if (i == 0)
			{
				fprintf(fp, "%d", 1);
				fclose(fp);
			}
			else
			{
				fprintf(fp, "%d", 0);
				fclose(fp);
			}
		}
	}

	//モータ逆転
	else if (drive_mode == -1)
	{
		for (i = 0; i < 2; i++)
		{
			sprintf(path3, "/sys/class/gpio/gpio%d/value", motor_gpio_num[motor_num][i]);
			fp = fopen(path3, "w");
			if (i == 0)
			{
				fprintf(fp, "%d", 0);
				fclose(fp);
			}
			else
			{
				fprintf(fp, "%d", 1);
				fclose(fp);
			}
		}
	}

	//入力したdutyでPWM信号を出力
	sprintf(path, "/sys/devices/ocp.%d/pwm_test_%s.%d/duty", OCP_NUM, PIN_PWM[motor_num], pwm_pin_num[motor_num]);
	fp = fopen(path, "wb");
	fprintf(fp, "%d", duty);
	fclose(fp);
	usleep(200);
}

//gpioの有効化関数
void gpio_export(int n)
{
	int fd;
	char buf[40];

	sprintf(buf, "%d", n);

	fd = open("/sys/class/gpio/export", O_WRONLY);
	write(fd, buf, strlen(buf));
	close(fd);
}

//gpioの有効化解除の関数
void gpio_unexport(int n)
{
	int fd;
	char buf[40];

	sprintf(buf, "%d", n);

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	write(fd, buf, strlen(buf));
	close(fd);
}

//gpioの設定ファイルを開く関数
int gpio_open(int n, char *file, int flag)
{
	int fd;
	char buf[40];

	sprintf(buf, "/sys/class/gpio/gpio%d/%s", n, file);

	fd = open(buf, flag);
	return fd;
}

int kbhit(void)
{
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

	if (ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}
	return 0;
}
