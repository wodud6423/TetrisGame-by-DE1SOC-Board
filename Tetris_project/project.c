/*******************************************************************************
part3code.c
*******************************************************************************/
#include "address_map_nios2.h"
#include "nios2_ctrl_reg_macros.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240


#define FILL_PIXEL(x,y,color) \
	*(short int *)(pixel_buffer_start + (((y)&0xFF) << 10) + (((x)&0x1FF)  << 1)) = color

volatile int pixel_buffer_start;
volatile int *pixel_ctrl_ptr;

volatile int *SW_ptr = (int*)SW_BASE;
volatile int *KEY_ptr = (int*)KEY_BASE;
volatile int *TIMER_ptr = (int*)TIMER_BASE;
volatile int* HEX0_ptr = (int*)HEX3_HEX0_BASE;

volatile int RUN;
int count;
volatile int i;
int timer;
volatile int x;
volatile int y;
int dx;

short int front_buffer[512 * 256];
short int back_buffer[512 * 256];

void draw_line(int x1, int y1, int x2, int y2, short int color);
void wait_for_vsync();
void clear_screen();
void draw_square(int x1, int y1, int x2, int y2,short int color);
void draw_dot(int x, int y, short int color);
void draw_block(int k, int x, int y,int rot, int(*stack)[18]);
void draw_1(int x, int y,int rot,short int color);
void draw_2(int x, int y,int rot, short int color);
void draw_3(int x, int y,int rot, short int color);
void draw_4(int x, int y,int rot, short int color);
void draw_5(int x, int y,int rot, short int color);
void draw_stack(int(*stack)[18]);
void draw_tet(int k,int userchange);
int seg_code(int i);
void displayHex3_0(int a);
int checkforstuck(int x, int y, int k, int rot, int(*stack)[18]);
int *stack(int x, int y, int k, int rot, int(*stack)[18]);
void drawA(int A[10][27]);
void draw_P(int x, int y);
void draw_ONE(int x, int y);
void draw_TWO(int x, int y);
void draw_L(int x, int y);
void draw_Y(int x, int y);
void draw_ONE(int x, int y);
void draw_TWO(int x, int y);
void draw_W(int x, int y);
void draw_D(int x, int y);
void draw_I(int x, int y);
void draw_N(int x, int y);
void draw_A(int x,int y);
void draw_G(int x,int y);
void draw_E(int x,int y);
void draw_T(int x,int y);
void draw_S(int x,int y);
void draw_R(int x,int y);
void draw_M(int x,int y);
void draw_start(void);
void draw_end(int num);
void small_dot(int x,int y);
void draw_con(int k, int rot);
void mini_small_dot(int x, int y);
void draw_miniP(int x, int y);
void draw_miniL(int x, int y);
void draw_miniA(int x, int y);
void draw_miniY(int x, int y);
void draw_miniE(int x, int y);
void draw_miniR(int x, int y);
void draw_miniONE(int x, int y);
void draw_miniTWO(int x, int y);

/* Push Button 활성화 */
void config_KEYs(void)
{
	/* 4개의 Push Button 활성화  
	인터럽트 설정 -> interrupt register Setting*/
	*(KEY_ptr + 2) = 0b1111;
}
/* Interval Timer 활성화 */
/* Timer Base Address : 0xFF202000 */
void config_TIMER(void) {
	int mil = 1000000 - 1; //10 milliseconds => 
	*(TIMER_ptr + 2) = mil;
	mil = mil >> 16;
	// Timer_ptr + 2 가 타이머 시작 하위 16bit 
	// Timer_ptr + 3 이 타이머 시작 상위 16bit이다.
	*(TIMER_ptr + 3) = mil;
	// Timer_ptr + 1dl Control Register 영역
	// 7 = ob111 => | START | CONT | ITO | 시작, 자동 리셋모드, 인터럽트 활성화 
	*(TIMER_ptr + 1) = 7; 
}
/* IRQ 활성화*/
/*I/O Peripheral IRQ #
	Interval timer 0
	Pushbutton switch parallel port 1
	Second Interval timer 2
	Audio port 6
	PS/2 port 7
	JTAG port 8
	IrDA port 9
	Serial port 10
	JP1 Expansion parallel port 11
	JP2 Expansion parallel port 12
	PS/2 port dual 23
	NIOS2_WRITE_IENABLE(IRQ 번호값) : 해당 인터럽트 활성화
	NIOS2_WRITE_STATUS(Interrupt 활성화 상태값) : 인터럽트 상태 정보값
*/
void enable_nios2_interrupts_on(void)
{
	NIOS2_WRITE_IENABLE(0x1);
	NIOS2_WRITE_STATUS(1);
}
/* Project Main */
/* goto 문을 이용한 처리*/
int main(void){
	srand(time(NULL));
	int speed = 0;
	int userchage = 1;
	int player1 = 0;
	int player2 = 0;
	int k=0;
	int dy=0;
	int key=0;
	int swit=0;
	*(KEY_ptr) = 0b0000;
	*(SW_ptr) = 0b0000000000;
	timer = 0;
	config_KEYs(); // push button 컨트롤 활성화
	config_TIMER(); // Interval interrupt 컨트롤 활성화
	enable_nios2_interrupts_on(); // IRQ 활성화

RESTART: {
	// 떨어지는 블록 정보 : 1~5
	k = rand() % 5 + 1; 
	// 테트리스 떨어지는 블록 한번의 길이
	dy = 10;
	// 총 테트리스 프레임 크기
	x = 150;
	y = 40;

	i = 0;
	RUN = 0;

	static int yst_bl[22][18] = {
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
						{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1} };
	int rot = 1;
	int nums[22][18];
	int height;
	int width;
	for (height = 0; height < 22; height++)
		for (width = 0; width < 18; width++) {
			nums[height][width] = yst_bl[height][width];
		} // 테트리스 블록 초기화
	int nums2[22][18];
	for (height = 0; height < 22; height++)
		for (width = 0; width < 18; width++) {
			nums2[height][width] = yst_bl[height][width];
		} // 테트리스 
	int nums3[22][18];
	// Double Buffer front & back 를 이용한 VGA 제어
	// 이는 Tearing 과 Flickering artifact를 제거하기 위함
	// PIXEL_BUF_CTRL_BASE = 0xFF203020
	// PIXEL_BUF_CTRL_BASE + 1 = front 
	pixel_ctrl_ptr = (int*)PIXEL_BUF_CTRL_BASE;
	// front buffer 설정
	*(pixel_ctrl_ptr + 1) = front_buffer;
	// 화면으로 동기화되기를 대기
	wait_for_vsync();

	// 실제 출력될 화면 할당
	pixel_buffer_start = *pixel_ctrl_ptr;
	// 출력 화면 초기화(pixel_buffer_start 값 초기화)
	clear_screen();
	// back buffer도 설정
	*(pixel_ctrl_ptr + 1) = back_buffer;
	draw_start();
	RUN = 0;
	// 게임 루프 진행
	while (1) {
		// back buffer 할당 -> 출력 화면으로
		pixel_buffer_start = *(pixel_ctrl_ptr + 1);
		int(*change)[18] = (void*)stack(x, y, k, rot, nums2);
		for (height = 0; height < 22; height++) {
			for (width = 0; width < 18; width++) {
				nums[height][width] = change[height][width];
			}
		}
		int stuckok = 0; // 충돌여부를 확인하는 변수
		int removepara = 0; // 
		int gameover = 0; // gameover 여부
		int checkch = 0; // 
		swit = *(SW_ptr); // 입력받은 switch 값
		key = *(KEY_ptr); // 입력받은 push_button 값
		int stimer = timer;
		
		stuckok = checkforstuck(x, y, k, rot, nums);
		// 입력된 push_button이 0x02이고 stuckok
		if ((key & 0x02)&&((stuckok & 0b01) != 0b01)) { 
			dx = -10; // 블록을 왼쪽으로 이동
			key = 0;
		}
		else if ((key & 0x01) && ((stuckok & 0b10) != 0b10)) {
			dx = 10; // 블록을 오른쪽으로 이동
			key = 0;
		}
		else if ((key & 0x04) &&(speed <= 7)) {
			speed += 1; // 아래로 속도를 높임
			key = 0;
		}
		else if ((key & 0x08)&&(speed > 0)) {
			speed -= 1; // 속도를 한단계 낮춤
			key = 0;
		}
		if (timer % 100 == 0) {
			if (swit & 0x01) { // 스위치 버튼의 입력값이 존재했다면?
				rot += 1; // 회전 값을 적용
				if (rot == 5)
					rot = 1;
				swit = 0;
				stuckok = checkforstuck(x, y, k, rot, nums);
				// 회전이 불가하다면? 이전 회전 상태에 대한 값을 취소
				if ((stuckok & 0b10) == 0b10 || (stuckok & 0b01) == 0b01) {
					rot -= 1;
					if (rot < 1)
						rot = 4;
				}
			}
			else if (swit & 0x02) {
				rot -= 1;
				if (rot < 1)
					rot = 4;
				swit = 0;
				stuckok = checkforstuck(x, y, k, rot, nums);
				if ((stuckok & 0b10) == 0b10 || (stuckok & 0b01) == 0b01) {
					rot += 1;
					if (rot == 5)
						rot = 1;
				}
			}
			x += dx;
			if (x >= 200 || x <= 90) {
				x -= dx;
			}
			dx = 0;
		}
		// 초기화 루틴
		*(KEY_ptr) = 0;
		*(SW_ptr) = 0;
		for (height = 0; height < 22; height++) {
			for (width = 0; width < 18; width++) {
				if (nums2[height][width] == nums[height][width])
					checkch = 0;
				else {
					checkch = 1;
					break;
				}
			}
			if (checkch == 1)
				break;
		}
		if (checkch ==0 ) {
			if ((stimer < 100000) && (timer% 100 == 0)) {
				y += dy;
				count++;
			}
			if ((stimer >= 100000) && (timer % 40 == 0)) {
				y += dy;
				count++;
			}
		}
		else if (checkch == 1) {
			x = 150;
			y = 30;
			k = rand() % 5 + 1;
			speed = 0;
			rot = 1;
			for (height = 0; height < 22; height++)
				for (width = 0; width < 18; width++) {
					nums2[height][width] = nums[height][width];
				}
		} // ���� ������ �ٴڿ� ���� ���
		for (height = 0; height < 21; height++) {
			for (width = 0; width < 18; width++) {
				if (nums2[height][width] == 1)
					removepara = 1;
				else {
					removepara = 0;
					break;
				}
			}
			// ������ ���� �׿����� Ȯ��
			if (removepara == 1) {
				for (int a = height; a > 0; a--) {
					for (width = 0; width < 18; width++) {
							nums2[a][width] = nums2[a-1][width];
					}
				}
				height--;
			}
		}
		// �� ���� ü������ ü����� �����Ͽ� �� ���� ���ְ� ���� �������� �Ʒ��� ����

		for (width = 4; width < 14; width++) {
			if (nums2[0][width] == 1) {
				gameover = 1;
				break;
			}
			else {
				gameover = 0;
			}
		}
		if (gameover == 1) {
			if (userchage == 1) {
				player1 = count;
				count = 0;
				userchage = 2;
				x = 150;
				y = 30;
				k = rand() % 5 + 1;
				rot = 1;
				for (height = 0; height < 22; height++) {
					for (width = 0; width < 18; width++) {
						nums2[height][width] = yst_bl[height][width];
					}
				}
			}
			//���ӿ����� PLAYER2�� �ʱ�ȭ
			else if (userchage == 2) {
				player2 = count;
				count = 0;
				userchage = 0;
			}
		}//���ӿ����� ����� ���
		if (userchage == 0) {
			while (1) {
				pixel_buffer_start = *(pixel_ctrl_ptr + 1);
				if (player1 > player2) {
					displayHex3_0(player1);
					clear_screen();
					draw_end(1);

				}
				else if (player2 > player1) {
					displayHex3_0(player2);
					clear_screen();
					draw_end(2);
				}
				else {
					displayHex3_0(player1);
					clear_screen();
					draw_end(3);
				}
				wait_for_vsync();
			}
		}
		// ���ӿ����Ǿ�����, 
		displayHex3_0(count);
		clear_screen();
		draw_block(k, x, y, rot, nums2);
		draw_tet(k, userchage);
		wait_for_vsync();

		}
	}
}
void draw_end(int num){
		draw_P(30, 110);
		draw_L(65, 110);
		draw_A(100, 110);
		draw_Y(135, 110);
		draw_E(175, 110);
		draw_R(210, 110);
		if (num == 1) {
			draw_ONE(240, 110);
		}
		else if (num == 2) {
			draw_TWO(245, 110);
		}
		else if (num == 3) {
			draw_D(90,160);
			draw_R(125,160);
			draw_A(160,160);
			draw_W(195, 160);
			return;
		}
		draw_W(90, 160);
		draw_I(135, 160);
		draw_N(175, 160);
	}
void draw_start(void){
	while(RUN == 0)
	{
		// 출력할 화면 버퍼의 값으로 할당
		pixel_buffer_start = *(pixel_ctrl_ptr+1);
		clear_screen();
		draw_S(10,110);
		draw_T(35,110);
		draw_A(65,110);
		draw_R(100,110);
		draw_T(125,110);
		draw_G(175,110);
		draw_A(210,110);
		draw_M(245,110);
		draw_E(285,110);
		if (*(KEY_ptr) & 0b1111 > 0)
			RUN = 1;
		wait_for_vsync();
	}
}

int *stack(int x, int y,int k, int rot,int (*stack)[18])
{
	int x_ = (x -60)/10;
	int y_ = (y-20)/10;
	static int stackout[22][18];
	for (int height = 0; height < 22; height++) {
		for (int width = 0; width < 18; width++) {
			stackout[height][width] = stack[height][width];
		}
	}
	switch (k) {
	case 1:
		switch (rot) {											
			case 1:	
			if(stackout[y_+1][x_]  == 1){
				stackout[y_-3][x_] = 1;
				stackout[y_-2][x_] = 1;
				stackout[y_-1][x_] = 1;
				stackout[y_][x_] = 1;
			}
				break;
			case 2:
			if(stackout[y_+1][x_]  == 1 || stackout[y_+1][x_+1]  == 1 || stackout[y_+1][x_+2]  == 1 || stackout[y_+1][x_+3]  == 1 ){
				stackout[y_][x_] = 1;
				stackout[y_][x_+1] = 1;
				stackout[y_][x_+2] = 1;
				stackout[y_][x_+3] = 1;
				}
				break;
			case 3:
				if (stackout[y_+1][x_] == 1) {
					stackout[y_ - 3][x_] = 1;
					stackout[y_ - 2][x_] = 1;
					stackout[y_ - 1][x_] = 1;
					stackout[y_][x_] = 1;
			}
				break;
			case 4:
				if(stackout[y_+1][x_]  == 1 || stackout[y_+1][x_+1]  == 1 || stackout[y_+1][x_+2]  == 1 || stackout[y_+1][x_+3]  == 1 ){
				stackout[y_][x_] = 1;
				stackout[y_][x_+1] = 1;
				stackout[y_][x_+2] = 1;
				stackout[y_][x_+3] = 1;
			}
				break;
		}
		break;
	case 2:
		switch (rot) {
			case 1:
			if(stackout[y_+1][x_] == 1 || stackout[y_+1][x_+1] == 1|| stackout[y_+1][x_+2] == 1){
				stackout[y_-1][x_] = 1;
				stackout[y_][x_] = 1;
				stackout[y_][x_+1] = 1;
				stackout[y_][x_+2] = 1;
			}
				break;
			case 2:
			if(stackout[y_+1][x_] == 1 || stackout[y_-1][x_+1] == 1){
				stackout[y_][x_] = 1;
				stackout[y_-1][x_] = 1;
				stackout[y_-2][x_] = 1;
				stackout[y_-2][x_+1] = 1;
			}
				break;
			case 3:
			if(stackout[y_][x_] == 1 || stackout[y_][x_+1] == 1|| stackout[y_+1][x_+2] == 1){
				stackout[y_-1][x_] = 1;
				stackout[y_-1][x_+1] = 1;
				stackout[y_-1][x_+2] = 1;
				stackout[y_][x_+2] = 1;
			}
				break;
			case 4:
			if(stackout[y_+1][x_] == 1 || stackout[y_+1][x_+1] == 1){
				stackout[y_][x_] = 1;
				stackout[y_][x_+1] = 1;
				stackout[y_-1][x_+1] = 1;
				stackout[y_-2][x_+1] = 1;
			}
				break;
		}
		break;
	case 3:
		switch (rot) {
			case 1:
			if(stackout[y_+1][x_] == 1 || stackout[y_+1][x_+1] == 1){
				stackout[y_][x_] = 1;
				stackout[y_-1][x_] = 1;
				stackout[y_][x_+1] = 1;
				stackout[y_-1][x_+1] = 1;
			}
				break;
			case 2:
				if (stackout[y_ + 1][x_] == 1 || stackout[y_ + 1][x_ + 1] == 1) {
					stackout[y_][x_] = 1;
					stackout[y_ - 1][x_] = 1;
					stackout[y_][x_ + 1] = 1;
					stackout[y_ - 1][x_ + 1] = 1;
			}
				break;
			case 3:
				if (stackout[y_ + 1][x_] == 1 || stackout[y_ + 1][x_ + 1] == 1) {
					stackout[y_][x_] = 1;
					stackout[y_ - 1][x_] = 1;
					stackout[y_][x_ + 1] = 1;
					stackout[y_ - 1][x_ + 1] = 1;
			}
				break;
			case 4:
				if (stackout[y_ + 1][x_] == 1 || stackout[y_ + 1][x_ + 1] == 1) {
					stackout[y_][x_] = 1;
					stackout[y_ - 1][x_] = 1;
					stackout[y_][x_ + 1] = 1;
					stackout[y_ - 1][x_ + 1] = 1;
			}
				break;
		}
		break;
	case 4:
		switch (rot) {
			case 1:
			if(stackout[y_+1][x_] == 1 || stackout[y_+1][x_+1] == 1 || stackout[y_+1][x_+2] == 1){
				stackout[y_][x_] = 1;
				stackout[y_][x_+1] = 1;
				stackout[y_-1][x_+1] = 1;
				stackout[y_][x_+2] = 1;
			}
				break;
			case 2:
				if(stackout[y_+1][x_] == 1 || stackout[y_][x_+1] == 1 ){
				stackout[y_][x_] = 1;
				stackout[y_-1][x_] = 1;
				stackout[y_-2][x_] = 1;
				stackout[y_-1][x_+1] = 1;
			}
				break;
			case 3:
			if(stackout[y_][x_] == 1 || stackout[y_+1][x_+1] == 1 || stackout[y_ ][x_ + 2] == 1){
				stackout[y_-1][x_] = 1;
				stackout[y_-1][x_+1] = 1;
				stackout[y_][x_+1] = 1;
				stackout[y_-1][x_+2] = 1;
			}
				break;
			case 4:
			if(stackout[y_][x_] == 1 || stackout[y_+1][x_+1] == 1){
				stackout[y_-1][x_] = 1;
				stackout[y_][x_+1] = 1;
				stackout[y_-1][x_+1] = 1;
				stackout[y_-2][x_+1] = 1;
			}
				break;
		}
		break;
	case 5:
		switch (rot) {
			case 1:
			if(stackout[y_+1][x_] == 1 || stackout[y_+1][x_+1] == 1|| stackout[y_][x_+2] == 1){
				stackout[y_][x_] = 1;
				stackout[y_][x_+1] = 1;
				stackout[y_-1][x_+1] = 1;
				stackout[y_-1][x_+2] = 1;
			}
				break;
			case 2:
			if(stackout[y_][x_] == 1 || stackout[y_+1][x_+1] == 1){
				stackout[y_-2][x_] = 1;
				stackout[y_-1][x_] = 1;
				stackout[y_-1][x_+1] = 1;
				stackout[y_][x_+1] = 1;
			}
				break;
			case 3:
				if (stackout[y_ + 1][x_] == 1 || stackout[y_ + 1][x_ + 1] == 1 || stackout[y_][x_ + 2] == 1) {
					stackout[y_][x_] = 1;
					stackout[y_][x_ + 1] = 1;
					stackout[y_ - 1][x_ + 1] = 1;
					stackout[y_ - 1][x_ + 2] = 1;
			}
				break;
			case 4:
				if (stackout[y_][x_] == 1 || stackout[y_ + 1][x_ + 1] == 1) {
					stackout[y_ - 2][x_] = 1;
					stackout[y_ - 1][x_] = 1;
					stackout[y_ - 1][x_ + 1] = 1;
					stackout[y_][x_ + 1] = 1;
			}
				break;
		}
		break;
	}
	return (int*)stackout;
}

// 출력된 상태가 왼쪽 혹은 오른쪽으로 이동 불가인지를 출력하는 함수
// 입력 인자 : x : 블록 열 / y : 블록 행 / k : 현재 입력된 블록 번호
//  / rot : 회전 값 / stack : 실제 테트리스 블록 맵

// 각 5개의 블록별로 입력된 회전 상태에 따른 충돌 여부를 판단.
// 충돌되어 있다면 충돌된 방향(왼/오) 를 출력.
int checkforstuck(int x, int y, int k, int rot, int(*stack)[18])
{
	int x_ = (x - 60) / 10;
	int y_ = (y - 20) / 10;
	int stuckresult = 0;
	static int stackout[22][18];
	for (int height = 0; height < 22; height++) {
		for (int width = 0; width < 18; width++) {
			stackout[height][width] = stack[height][width];
		}
	}
	// 입력된 push_buffton 값에 따라 
	switch (k) {
	case 1:
		switch (rot) {
		case 1:
			if (stackout[y_ ][x_-1] == 1 ) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 1] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 2:
			if (stackout[y_][x_-1] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 4] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 3:
			if (stackout[y_][x_ - 1] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 1] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 4:
			if (stackout[y_][x_ - 1] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 4] == 1) {
				stuckresult += 0b10;
			}
			break;
		}
		break;
	case 2:
		switch (rot) {
		case 1:
			if (stackout[y_][x_-1] == 1 || stackout[y_ -1][x_ -1] == 1 ) {
				stuckresult += 0b01;
			}
			if (stackout[y_ - 1][x_ + 1] || stackout[y_][x_ + 3] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 2:
			if (stackout[y_][x_ - 1] == 1 || stackout[y_ - 1][x_ - 1] == 1||stackout[y_-2][x_ - 1] == 1 ) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 1] == 1 || stackout[y_ - 1][x_ + 1] == 1 || stackout[y_ - 2][x_ + 2] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 3:
			if (stackout[y_-1][x_-1] == 1 || stackout[y_][x_+1] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_ - 1][x_ + 3] == 1 || stackout[y_][x_ + 3] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 4:
			if (stackout[y_][x_-1] == 1 ||stackout[y_-1][x_]==1|| stackout[y_ - 2][x_] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 2] == 1 || stackout[y_ - 1][x_ + 2] == 1 || stackout[y_ - 2][x_ + 2] == 1) {
				stuckresult += 0b10;
			}
			break;
		}
		break;
	case 3:
		switch (rot) {
		case 1:
			if (stackout[y_][x_-1] == 1 || stackout[y_ -1][x_ -1] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 2] == 1 || stackout[y_ - 1][x_ + 2] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 2:
			if (stackout[y_][x_ - 1] == 1 || stackout[y_ - 1][x_ - 1] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 2] == 1 || stackout[y_ - 1][x_ + 2] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 3:
			if (stackout[y_][x_ - 1] == 1 || stackout[y_ - 1][x_ - 1] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 2] == 1 || stackout[y_ - 1][x_ + 2] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 4:
			if (stackout[y_][x_ - 1] == 1 || stackout[y_ - 1][x_ - 1] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 2] == 1 || stackout[y_ - 1][x_ + 2] == 1) {
				stuckresult += 0b10;
			}
			break;
		}
		break;
	case 4:
		switch (rot) {
		case 1:
			if (stackout[y_ ][x_-1] == 1 || stackout[y_-1][x_] == 1 ) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 3] == 1 || stackout[y_ - 1][x_ + 2] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 2:
			if (stackout[y_][x_-1] == 1 || stackout[y_-1][x_ -1] == 1|| stackout[y_ - 2][x_ - 1] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_ - 2][x_ + 1] == 1 || stackout[y_ - 1][x_ + 2] == 1 || stackout[y_][x_ + 1] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 3:
			if (stackout[y_-1][x_-1] == 1 || stackout[y_][x_] == 1 ) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 2] == 1 || stackout[y_ - 1][x_ + 3] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 4:
			if (stackout[y_-2][x_] == 1 || stackout[y_ -1][x_ -1] == 1|| stackout[y_][x_] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_ - 2][x_ + 2] == 1 || stackout[y_ - 1][x_ + 2] == 1 || stackout[y_][x_ + 2] == 1) {
				stuckresult += 0b10;
			}
			break;
		}
		break;
	case 5:
		switch (rot) {
		case 1:
			if (stackout[y_][x_-1] == 1 || stackout[y_ -1][x_] == 1 ) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 2] == 1 || stackout[y_ - 1][x_ + 3] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 2:
			if (stackout[y_-2][x_-1] == 1 || stackout[y_ -1][x_ -1] == 1|| stackout[y_][x_] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_ - 2][x_ + 1] == 1 || stackout[y_ - 1][x_ + 2] == 1 || stackout[y_][x_ + 2] == 1) {
				stuckresult += 0b10;
			}
			break;
		case 3:
			if (stackout[y_][x_ - 1] == 1 || stackout[y_ - 1][x_] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_][x_ + 2] == 1 || stackout[y_ - 1][x_ + 3] == 1) {
				stuckresult += 0b10;
			}
			break;
			if (stackout[y_ - 2][x_ - 1] == 1 || stackout[y_ - 1][x_ - 1] == 1 || stackout[y_][x_] == 1) {
				stuckresult += 0b01;
			}
			if (stackout[y_ - 2][x_ + 1] == 1 || stackout[y_ - 1][x_ + 2] == 1 || stackout[y_][x_ + 2] == 1) {
				stuckresult += 0b10;
			}
			break;
		}
		break;
	}
	return stuckresult;
}
void draw_stack(int(*stack)[18]) {
	for (int h = 0; h < 22; h++) {
		for (int w = 4; w < 14; w++) {
			int x = (w * 10) + 60;
			int y = (h * 10) + 20;
			if (h != 21) {
				if (stack[h][w] == 1)
					draw_dot(x, y, 0);
			}
			else if (h == 21) {
				draw_dot(x, y, 0xFFFF);


			}
		}
	}
}
void draw_P(int x, int y) {
	int i;
	int x1[19] = { x,x,x,x,x,x ,x ,x + 5,x + 5,x + 10,x + 10,x + 15,x + 15,x + 20,x + 20,x+25 ,x + 25,x + 25,x + 25};
	int y1[19] = { y ,y + 5,y + 10,y + 15,y + 20,y +25,y + 30, y ,y + 15,y,y + 15,y ,y +15,y ,y + 15,y,y+5,y + 10,y + 15 };
	for (i = 0; i < 19; i++) {
		small_dot(x1[i], y1[i]);
	}
}
void draw_miniP(int x, int y) {
	int i;
	int x1[19] = { x,x,x,x,x,x ,x ,x + 2,x + 2,x + 4,x + 4,x + 6,x + 6,x + 8,x + 8,x + 10 ,x + 10,x + 10,x + 10 };
	int y1[19] = { y ,y + 2,y + 4,y + 6,y + 8,y + 10,y +12, y ,y + 6,y,y + 6,y ,y + 6,y ,y + 6,y,y + 2,y + 4,y + 6 };
	for (i = 0; i < 19; i++) {
		mini_small_dot(x1[i], y1[i]);
	}
}

void draw_L(int x, int y) {
	int i;
	int x1[12] = { x,x,x,x,x,x ,x ,x + 5,x + 10,x + 15,x + 20,x + 25 };
	int y1[12] = { y ,y + 5,y + 10,y + 15,y + 20,y + 25,y + 30,y + 30 ,y + 30 ,y + 30 ,y + 30 ,y + 30 };
	for (i = 0; i < 12; i++) {
		small_dot(x1[i], y1[i]);
	}
}

void draw_miniL(int x, int y) {
	int i;
	int x1[12] = { x,x,x,x,x,x ,x ,x + 2,x + 4,x + 6,x + 8,x + 10 };
	int y1[12] = { y ,y + 2,y + 4,y + 6,y + 8,y + 10,y + 12,y + 12 ,y + 12 ,y + 12 ,y + 12 ,y + 12 };
	for (i = 0; i < 12; i++) {
		mini_small_dot(x1[i], y1[i]);
	}
}
void draw_miniA(int x, int y) {
	int i;
	int x1[18] = { x,x,x,x,x,x + 2,x + 2,x + 4,x + 4,x + 6,x + 6,x + 8,x + 8,x + 10,x + 10,x + 10,x + 10,x + 10 };
	int y1[18] = { y + 4,y + 6,y + 8,y + 10,y + 12,y + 2,y + 8,y,y + 8,y,y + 8,y + 2,y + 8,y + 4,y + 6,y + 8,y + 10,y + 12 };
	for (i = 0; i < 18; i++) {
		mini_small_dot(x1[i], y1[i]);
	}
}
void draw_Y(int x, int y) {
	int i;
	int x1[16] = { x,x,x+5,x+5,x+10,x+10 ,x+15 ,x + 15,x + 15,x + 15,x + 20,x + 20,x + 25,x + 25,x + 30,x + 30 };
	int y1[16] = { y ,y + 5,y + 5,y + 10,y + 10,y + 15,y + 15, y+20 ,y + 25,y+ 30,y + 15,y+10 ,y + 10, y+5 ,y + 5,y };
	for (i = 0; i < 16; i++) {
		small_dot(x1[i], y1[i]);
	}
}
void draw_miniY(int x, int y) {
	int i;
	int x1[16] = { x,x,x + 2,x + 2,x + 4,x + 4 ,x + 6 ,x + 6,x + 6,x + 6,x + 8,x + 8,x + 10,x + 10,x + 12,x + 12 };
	int y1[16] = { y ,y + 2,y + 2,y + 4,y + 4,y + 6,y + 6, y + 8 ,y + 10,y + 12,y + 6,y + 4 ,y + 4, y + 2 ,y + 2,y };
	for (i = 0; i < 16; i++) {
		mini_small_dot(x1[i], y1[i]);
	}
}
void draw_miniE(int x, int y) {
	int i;
	int x1[19] = { x,x,x,x,x,x,x,x + 2,x + 2,x + 2,x + 4,x + 4,x + 4,x + 6,x + 6,x + 6,x + 8,x + 8,x + 8 };
	int y1[19] = { y,y + 2,y + 4,y + 6,y + 8,y + 10,y + 12,y,y + 6,y + 12,y,y + 6,y + 12,y,y + 6,y + 12,y,y + 6,y + 12 };
	for (i = 0; i < 19; i++) {
		mini_small_dot(x1[i], y1[i]);
	}
}
void draw_miniR(int x, int y) {
	int i;
	int x1[18] = { x,x,x,x,x,x,x,x + 2,x + 2,x + 4,x + 4,x + 6,x + 6,x + 6,x + 8,x + 8,x + 8,x + 8 };
	int y1[18] = { y,y + 2,y + 4,y + 6,y + 8,y + 10,y + 12,y,y + 6,y,y + 6,y,y + 6,y + 8,y + 2,y + 4,y + 10,y + 12 };
	for (i = 0; i < 18; i++) {
		mini_small_dot(x1[i], y1[i]);
	}
}
void draw_miniONE(int x, int y) {
	int i;
	int x1[11] = { x + 2,x + 2,x + 4,x + 4,x + 6,x + 6,x + 6,x + 6,x + 6,x + 6,x + 6 };
	int y1[11] = { y + 4,y + 2 ,y + 2,y ,y ,y + 2,y + 4,y + 6,y + 8,y + 10,y + 12 };
	for (i = 0; i < 11; i++) {
		mini_small_dot(x1[i], y1[i]);
	}
}
void draw_miniTWO(int x, int y) {
	int i;
	int x1[21] = { x ,x ,x + 2,x + 2,x + 2,x + 4,x + 4,x + 4,x + 4,x + 6,x + 6,x + 6 ,x + 6,x + 6,x + 8,x + 8,x + 8,x + 8,x + 8,x + 10,x + 12 };
	int y1[21] = { y + 4,y + 12,y + 4 ,y + 2,y + 12 ,y + 2 ,y ,y + 10,y + 12,y ,y + 2,y + 8,y + 10,y + 12,y + 2,y + 4,y + 6,y + 8,y + 12,y + 12,y + 12 };
	for (i = 0; i < 21; i++) {
		mini_small_dot(x1[i], y1[i]);
	}
}
void draw_W(int x, int y) {
	int i;
	int x1[24] = { x,x,x,x+5,x+5,x+5 ,x+10 ,x + 10,x + 10,x + 15,x + 15,x + 20,x + 20,x + 25,x + 25,x + 30,x + 30,x + 30,x+35,x+35,x+35,x+40,x+40,x+40 };
	int y1[24] = { y ,y +5,y + 10,y + 10,y + 15,y + 20,y + 20, y+25 ,y + 30, y+25, y + 20 ,y+20, y+15 , y + 25, y + 20,y + 20, y + 25 ,y + 30,y + 10,y + 15,y + 20, y ,y + 5,y + 10,};
	for (i = 0; i < 24; i++) {
		small_dot(x1[i], y1[i]);
	}
}
void draw_D(int x, int y) {
	int i;
	int x1[22] = { x,x,x,x ,x ,x  ,x  ,x + 5,x + 5,x + 10,x + 10,x + 15,x + 15,x+15,x+15,x + 20,x + 20,x + 20,x + 20,x + 25,x + 25,x + 25 };
	int y1[22] = { y ,y + 5,y + 10,y + 15,y + 20,y + 25,y + 30, y  ,y + 30, y , y + 30 , y  ,y + 5,y + 25 ,y + 30,y + 5,y + 10,y + 20,y + 25,y+10,y+15,y+20 };
	for (i = 0; i < 22; i++) {
		small_dot(x1[i], y1[i]);
	}
}
void draw_I(int x, int y) {
	int i;
	int x1[11] = { x+10,x+10,x+15,x + 15, x + 15, x + 15, x + 15, x + 15, x + 15,x+20,x+20};
	int y1[11] = { y,y+30,y ,y + 5,y + 10,y + 15,y + 20,y + 25,y + 30, y  ,y + 30 };
	for (i = 0; i < 11; i++) {
		small_dot(x1[i], y1[i]);
	}
}
void draw_N(int x, int y) {
	int i;
	int x1[22] = { x,x,x,x ,x ,x  ,x, x+5,x+5,x+10,x+10,x+15,x+15,x+20,x+20,x + 25 ,x + 25 ,x + 25 ,x + 25 ,x + 25 ,x + 25 ,x + 25 };
	int y1[22] = { y ,y + 5,y + 10,y + 15,y + 20,y + 25,y + 30,y+5,y+10,y+10,y+15,y+15,y+20,y+20,y+25 , y ,y + 5,y + 10,y + 15,y + 20,y + 25,y + 30 };
	for (i = 0; i < 22; i++) {
		small_dot(x1[i], y1[i]);
	}
}
void draw_A(int x, int y){
	int i;
	int x1[18] = {x,x,x,x,x,x+5,x+5,x+10,x+10,x+15,x+15,x+20,x+20,x+25,x+25,x+25,x+25,x+25};
	int y1[18] = {y+10,y+15,y+20,y+25,y+30,y+5,y+20,y,y+20,y,y+20,y+5,y+20,y+10,y+15,y+20,y+25,y+30};
	for (i = 0; i < 18; i++) {
		small_dot(x1[i], y1[i]);
	}
}
void draw_E(int x, int y){
	int i;
	int x1[19] = {x,x,x,x,x,x,x,x+5,x+5,x+5,x+10,x+10,x+10,x+15,x+15,x+15,x+20,x+20,x+20};
	int y1[19] = {y,y+5,y+10,y+15,y+20,y+25,y+30,y,y+15,y+30,y,y+15,y+30,y,y+15,y+30,y,y+15,y+30};
	for (i = 0; i < 19; i++) {
		small_dot(x1[i], y1[i]);
	}
}

void draw_R(int x, int y){
	int i;
	int x1[18] = {x,x,x,x,x,x,x,x+5,x+5,x+10,x+10,x+15,x+15,x+15,x+20,x+20,x+20,x+20};
	int y1[18] = {y,y+5,y+10,y+15,y+20,y+25,y+30,y,y+15,y,y+15,y,y+15,y+20,y+5,y+10,y+25,y+30};
	for (i = 0; i < 18; i++) {
		small_dot(x1[i], y1[i]);
	}
}

void draw_G(int x, int y){
	int i;
	int x1[21] = {x,x,x,x,x,x,x,x+5,x+5,x+10,x+10,x+15,x+15,x+15,x+20,x+20,x+20,x+25,x+25,x+25,x+25};
	int y1[21] = {y,y+5,y+10,y+15,y+20,y+25,y+30,y,y+30,y,y+30,y,y+20,y+30,y,y+20,y+30,y,y+20,y+25,y+30};
	for (i = 0; i < 21; i++) {
		small_dot(x1[i], y1[i]);
	}
}

void draw_M(int x, int y){
	int i;
	int x1[22] = {x,x,x,x,x,x,x,x+5,x+10,x+10,x+15,x+15,x+20,x+20,x+25,x+30,x+30,x+30,x+30,x+30,x+30,x+30};
	int y1[22] = {y,y+5,y+10,y+15,y+20,y+25,y+30,y+5,y+10,y+15,y+20,y+25,y+15,y+10,y+5,y,y+5,y+10,y+15,y+20,y+25,y+30};
	for (i = 0; i < 22; i++) {
		small_dot(x1[i], y1[i]);
	}
}

void draw_S(int x, int y){
	int i;
	int x1[15] = {x,x,x,x+5,x+5,x+5,x+10,x+10,x+10,x+15,x+15,x+15,x+20,x+20,x+20};
	int y1[15] = {y+5,y+10,y+30,y,y+15,y+30,y,y+15,y+30,y,y+15,y+30,y,y+20,y+25};
	for (i = 0; i < 15; i++) {
		small_dot(x1[i], y1[i]);
	}
}

void draw_T(int x, int y){
	int i;
	int x1[11] = {x+5,x+10,x+15,x+15,x+15,x+15,x+15,x+15,x+15,x+20,x+25};
	int y1[11] = {y,y,y,y+5,y+10,y+15,y+20,y+25,y+30,y,y};
	for (i = 0; i < 11; i++) {
		small_dot(x1[i], y1[i]);
	}
}
void draw_ONE(int x, int y) {
	int i;
	int x1[11] = { x + 5,x + 5,x + 10,x + 10,x + 15,x + 15,x + 15,x + 15,x + 15,x + 15,x + 15 };
	int y1[11] = { y+10,y+5 ,y + 5,y ,y ,y + 5,y+10,y + 15,y+20,y+25,y+30 };
	for (i = 0; i < 11; i++) {
		small_dot(x1[i], y1[i]);
	}
}
void draw_TWO(int x, int y) {
	int i;
	int x1[21] = { x ,x ,x + 5,x + 5,x + 5,x + 10,x + 10,x + 10,x + 10,x + 15,x + 15,x + 15 ,x+15,x+15,x + 20,x+20,x+20,x+20,x+20,x+25,x+30};
	int y1[21] = { y + 10,y + 30,y + 10 ,y + 5,y+30 ,y+5 ,y ,y + 25,y + 30,y ,y + 5,y + 20,y+25,y+30,y+5,y+10,y+15,y+20,y+30,y+30,y+30 };
	for (i = 0; i < 21; i++) {
		small_dot(x1[i], y1[i]);
	}
}
void small_dot(int x, int y){
	draw_square(x,y,x+5,y+5,0);
}
void mini_small_dot(int x, int y) {
	draw_square(x, y, x + 2, y + 2, 0);
}
void draw_dot(int x, int y,short int color){
	draw_square(x,y,x+10,y+10,0xFFFF);
	draw_square(x+1,y+1,x+9,y+9,color);
}
void draw_tet(int k,int userchage){
	draw_line(100,200,200,200,0x8000);
	draw_line(100,190,200,190,0x8000);
	draw_line(100,180,200,180,0x8000);
	draw_line(100,170,200,170,0x8000);
	draw_line(100,160,200,160,0x8000);
	draw_line(100,150,200,150,0x8000);
	draw_line(100,140,200,140,0x8000);
	draw_line(100,130,200,130,0x8000);
	draw_line(100,120,200,120,0x8000);
	draw_line(100,110,200,110,0x8000);
	draw_line(100,100,200,100,0x8000);
	draw_line(100,90,200,90,0x8000);
	draw_line(100,80,200,80,0x8000);
	draw_line(100,70,200,70,0x8000);
	draw_line(100,60,200,60,0x8000);
	draw_line(100,50,200,50,0x8000);
	draw_line(100,40,200,40,0x8000);
	draw_line(100,30,200,30,0x8000);
	draw_line(100,20,200,20,0x8000);
	draw_line(100,210,200,210,0x8000);
	draw_line(100,220,200,220,0x8000);
	draw_line(100,230,200,230,0x8000);
	draw_line(100,20,100,230,0x8000);
	draw_line(110,20,110,230,0x8000);
	draw_line(120,20,120,230,0x8000);
	draw_line(130,20,130,230,0x8000);
	draw_line(140,20,140,230,0x8000);
	draw_line(150,20,150,230,0x8000);
	draw_line(160,20,160,230,0x8000);
	draw_line(170,20,170,230,0x8000);
	draw_line(180,20,180,230,0x8000);
	draw_line(190,20,190,230,0x8000);
	draw_line(200,20,200,230,0x8000);
	draw_line(230, 40, 290, 40 , 0x8000);
	draw_line(230, 110, 290, 110, 0x8000);
	draw_line(230, 40, 230, 110, 0x8000);
	draw_line(290, 40, 290, 110, 0x8000);
	switch (k) {
	case 1:
		draw_1(255, 90, 1, 0x001F);
		break;
	case 2:
		draw_2(245, 90, 1, 0x01E0);
		break;
	case 3:
		draw_3(250, 90, 1, 0x8100);
		break;
	case 4:
		draw_4(245, 90, 1, 0x8010);
		break;
	case 5:
		draw_5(245, 90, 1, 0x0110);
		break;
	}
	draw_miniP(210, 140);
	draw_miniL(226, 140);
	draw_miniA(242, 140);
	draw_miniY(258, 140);
	draw_miniE(274, 140);
	draw_miniR(290, 140);
	if (userchage == 1)
		draw_miniONE(306,140);
	else if (userchage == 2)
		draw_miniTWO(306, 140);
}


void draw_block(int k, int x, int y,int rot, int(*stack)[18]){
	switch(k){
		case 1:
			draw_1(x,y,rot,0x001F);
			break;
		case 2:
			draw_2(x,y,rot,0x01E0);
			break;
		case 3:
			draw_3(x,y,rot,0x8100);
			break;
		case 4:
			draw_4(x,y,rot,0x8010);
			break;
		case 5:
			draw_5(x,y,rot,0x0110);
			break;
	}
	draw_stack(stack);
}

int seg_code(int i) {
	char seg[10] = { 0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01111101,0b00000111, 0b01111111, 0b01100111 };
	return seg[i];
}
void displayHex3_0(int a) {
	unsigned int st;
	unsigned int s0;

	st = seg_code(a / 1000);
	a = a % 1000;
	s0 = st << 8;


	st = seg_code(a / 100);
	a = a % 100;
	s0 = s0 | st;
	s0 = s0 << 8;

	st = seg_code(a / 10);
	a = a % 10;
	s0 = s0 | st;
	s0 = s0 << 8;

	st = seg_code(a);
	s0 = s0 | st;
	*HEX0_ptr = s0;
}

// 일자 모양 테트리스 블록
// rot 값에 따라서 - -> | -> - -> | 되도록 구현하였다. 
/*
 1 1 1 1
*/
void draw_1(int x, int y,int rot,short int color){
	int i;
	int x1[4] = { x,x,x,x };
	int x2[4] = { x,x + 10,x + 20,x + 30 };
	int x3[4] = { x,x,x,x };
	int x4[4] = { x,x + 10,x + 20,x + 30 };
	int y1[4] = { y - 30,y - 20,y - 10,y };
	int y2[4] = { y ,y ,y ,y };
	int y3[4] = { y ,y - 10,y - 20,y - 30 };
	int y4[4] = { y ,y ,y ,y };
	switch (rot) {
		case 1:
			for (i = 0; i < 4; i++) {
				draw_dot(x1[i], y1[i],color);
			}
			break;
		case 2:
			for (i = 0; i < 4; i++) {
				draw_dot(x2[i], y2[i],color);
			}
			break;
		case 3:
			for (i = 0; i < 4; i++) {
				draw_dot(x3[i], y3[i],color);
			}
			break;
		case 4:
			for (i = 0; i < 4; i++) {
				draw_dot(x4[i], y4[i],color);
			}
			break;
	}
}
// ㄴ 모양의 블록 
/*
	1
	1 1 1
*/
void draw_2(int x, int y,int rot, short int color){
	int i;
	int x1[4] = { x,x,x + 10,x + 20 };
	int y1[4] = { y - 10,y,y,y };
	int x2[4] = { x,x,x,x + 10 };
	int y2[4] = { y - 20,y -10,y ,y-20 };
	int x3[4] = { x,x + 10,x + 20,x + 20 };
	int y3[4] = { y - 10 ,y - 10,y - 10,y };
	int x4[4] = { x,x+10,x+10,x+10 };
	int y4[4] = { y,y - 20,y - 10,y };
	switch (rot) {
	case 1:
		for (i = 0; i < 4; i++) {
			draw_dot(x1[i], y1[i], color);
		}
		break;
	case 2:
		for (i = 0; i < 4; i++) {
			draw_dot(x2[i], y2[i], color);
		}
		break;
	case 3:
		for (i = 0; i < 4; i++) {
			draw_dot(x3[i], y3[i], color);
		}
		break;
	case 4:
		for (i = 0; i < 4; i++) {
			draw_dot(x4[i], y4[i], color);
		}
		break;
	}
}

// 네모 테트리스 블록
/*
 1 1
 1 1
*/
void draw_3(int x, int y,int rot, short int color){
	int i;
	int x1[4] = { x,x,x + 10,x + 10 };
	int y1[4] = { y,y - 10,y,y - 10 };
	int x2[4] = { x,x,x + 10,x + 10 };
	int y2[4] = { y,y - 10,y,y - 10 };
	int x3[4] = { x,x,x + 10,x + 10 };
	int y3[4] = { y,y - 10,y,y - 10 };
	int x4[4] = { x,x,x + 10,x + 10 };
	int y4[4] = { y,y - 10,y,y - 10 };
	switch (rot) {
	case 1:
		for (i = 0; i < 4; i++) {
			draw_dot(x1[i], y1[i], color);
		}
		break;
	case 2:
		for (i = 0; i < 4; i++) {
			draw_dot(x2[i], y2[i], color);
		}
		break;
	case 3:
		for (i = 0; i < 4; i++) {
			draw_dot(x3[i], y3[i], color);
		}
		break;
	case 4:
		for (i = 0; i < 4; i++) {
			draw_dot(x4[i], y4[i], color);
		}
		break;
	}
}

// 학교 모양 테트리스 블록
/* 
	  1
	1 1 1
*/
void draw_4(int x, int y, int rot,short int color){
	int i;
	int x1[4] = { x,x + 10,x + 10,x + 20 };
	int y1[4] = { y,y,y - 10,y };
	int x2[4] = { x,x,x ,x + 10 };
	int y2[4] = { y,y - 10,y - 20,y - 10 };
	int x3[4] = { x,x + 10,x + 10,x + 20};
	int y3[4] = { y - 10,y - 10 ,y,y - 10 };
	int x4[4] = { x,x + 10,x + 10,x + 10 };
	int y4[4] = { y - 10,y,y - 10,y - 20 };
	switch (rot) {
	case 1:

		for (i = 0; i < 4; i++) {
			draw_dot(x1[i], y1[i], color);
		}
		break;
	case 2:

		for (i = 0; i < 4; i++) {
			draw_dot(x2[i], y2[i], color);
		}
		break;
	case 3:

		for (i = 0; i < 4; i++) {
			draw_dot(x3[i], y3[i], color);
		}
		break;
	case 4:

		for (i = 0; i < 4; i++) {
			draw_dot(x4[i], y4[i], color);
		}
		break;
	}
}

// 계단 모양 테트리스 블록
/*
     1 1
   1 1 
*/
void draw_5(int x, int y,int rot, short int color){
	int i;
	int x1[4] = { x,x + 10,x + 10,x + 20 };
	int y1[4] = { y,y,y - 10,y - 10 };
	int x2[4] = { x ,x ,x + 10 ,x + 10 };
	int y2[4] = { y - 20,y - 10,y - 10,y };
	int x3[4] = { x,x + 10,x + 10,x + 20 };
	int y3[4] = { y,y,y - 10,y - 10 };
	int x4[4] = { x,x ,x + 10 ,x + 10 };
	int y4[4] = { y - 20,y - 10,y - 10,y };
	switch (rot) {
	case 1:
		for (i = 0; i < 4; i++) {
			draw_dot(x1[i], y1[i], color);
		}
		break;
	case 2:
		for (i = 0; i < 4; i++) {
			draw_dot(x2[i], y2[i], color);
		}
		break;
	case 3:
		for (i = 0; i < 4; i++) {
			draw_dot(x3[i], y3[i], color);
		}
		break;
	case 4:
		for (i = 0; i < 4; i++) {
			draw_dot(x4[i], y4[i], color);
		}
		break;
	}

}

// 순차적으로 테트리스 프레임을 체우기 위한 함수
void draw_line(int x1, int y1, int x2, int y2, short int color){
	int t;
	int st = (abs(y2-y1)>abs(x2-x1));
	if(st){
		t = x1;
		x1 = y1;
		y1 = t;
		t = x2;
		x2 = y2;
		y2 = t;
	}
	int dx = abs(x1-x2);
	int dy = abs(y1-y2);


	int error = -(dx/2);
	int y = y1;

	int i;
	if(y1<y2) i =1;
	else i = -1;

	int x;
	for (x=x1; x<=x2; x++){
		if(st) FILL_PIXEL(y,x,color);
		else FILL_PIXEL(x,y,color);
		error = error + dy;
		if(error >= 0){
			y = y+i;
			error = error - dx;
		}
	}
}

// 화면 Tearing 방지를 위한 vsync
// back buffer - buffer 내용을 스왑하여 back buffer를 출력하고,
// 
void wait_for_vsync(){
	register int status;
	// 해당 버퍼에 값을 1쓰면 back buffer 와 스왑됨.
	//
	*pixel_ctrl_ptr = 1;

	status = *(pixel_ctrl_ptr +3);
	while((status & 0x01) != 0){
		status = *(pixel_ctrl_ptr +3);
	}
}
// 화면 버퍼 초기화(하얀색으로 출력)
void clear_screen(){
	draw_square(0,0,SCREEN_WIDTH-1,SCREEN_HEIGHT-1,0xFFFF);
}
// 시작 x,y  끝 x,y 색깔을 입력받아서
// 화면을 체우는 함수
void draw_square(int x1, int y1, int x2, int y2,short int color){
	int x,y;
	for(x = x1; x<=x2; x++)
		for(y = y1; y<=y2; y++)
			FILL_PIXEL(x,y,color);
}
