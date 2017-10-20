#include "stdafx.h"
#include<stdio.h>
#include <stdbool.h>
#include <time.h>

#include <conio.h>
#include "NIDAQmx.h"
#include <windows.h>

//extern "C" {
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
//}

#include <time.h>

#include <interface.h>


//structure
typedef struct {
	int x;
	int z;
} TPosition;

typedef struct {
	bool ocupado;
	time_t validade;
} palete;

//mailboxes
xQueueHandle mbx_x;  //for goto_x
xQueueHandle mbx_z;  //for goto_z
xQueueHandle mbx_xz;
xQueueHandle mbx_input;
xQueueHandle mbx_button;
xQueueHandle mbx_input_switch;
xQueueHandle mbx_pallet_in;
xQueueHandle mbx_expired;

//semahores
xSemaphoreHandle sem_x_done;
xSemaphoreHandle sem_z_done;
xSemaphoreHandle put_done;
xSemaphoreHandle take_done;
xSemaphoreHandle put_start;
xSemaphoreHandle take_start;


palete matriz[3][3] = {0};
TPosition global_Pos;



int getBitValue(uInt8 value, uInt8 n_bit)
// given a byte value, returns the value of bit n
{
	return(value & (1 << n_bit));
}

void setBitValue(uInt8  *variable, int n_bit, int new_value_bit)
// given a byte value, set the n bit to value
{
	taskENTER_CRITICAL();
	uInt8  mask_on = (uInt8)(1 << n_bit);
	uInt8  mask_off = ~mask_on;
	if (new_value_bit)  *variable |= mask_on;
	else                *variable &= mask_off;
	taskEXIT_CRITICAL();
}

void move_z_up()
{
	taskENTER_CRITICAL();
	uInt8 p = ReadDigitalU8(2); //read port 2
	setBitValue(&p, 2, 0);    
	setBitValue(&p, 3, 1);      
	WriteDigitalU8(2, p); //update port 2
	taskEXIT_CRITICAL();
}
void move_z_down()
{
	taskENTER_CRITICAL();
	uInt8 p = ReadDigitalU8(2); //read port 2
	setBitValue(&p, 3, 0);    
	setBitValue(&p, 2, 1);      
	WriteDigitalU8(2, p); //update port 2
	taskEXIT_CRITICAL();
}
void move_x_right()
{
	taskENTER_CRITICAL();
	uInt8 p = ReadDigitalU8(2); //read port 2
	setBitValue(&p, 6, 0);    //  set bit 6 to  low level
	setBitValue(&p, 7, 1);      //set bit 7 to high level
	WriteDigitalU8(2, p); //update port 2
	taskEXIT_CRITICAL();
}
void move_x_left()
{
	taskENTER_CRITICAL();
	// no xTaskDelay or while loops etc here
	uInt8 vp2 = ReadDigitalU8(2);
	setBitValue(&vp2, 7, 0);
	setBitValue(&vp2, 6, 1);
	WriteDigitalU8(2, vp2);
	taskEXIT_CRITICAL();
}

void move_y_inside()
{
	taskENTER_CRITICAL();
	uInt8 p = ReadDigitalU8(2); //read port 2
	setBitValue(&p, 4, 0);   
	setBitValue(&p, 5, 1);      
	WriteDigitalU8(2, p); //update port 2
	taskEXIT_CRITICAL();
}
void move_y_outside()
{
	taskENTER_CRITICAL();
	uInt8 p = ReadDigitalU8(2); //read port 2
	setBitValue(&p, 5, 0);   
	setBitValue(&p, 4, 1);      
	WriteDigitalU8(2, p); //update port 2
	taskEXIT_CRITICAL();
}

void stop_x()
{
	taskENTER_CRITICAL();
	uInt8 p = ReadDigitalU8(2); //read port 2
	setBitValue(&p, 6, 0);   
	setBitValue(&p, 7, 0);   
	WriteDigitalU8(2, p); //update port 2
	taskEXIT_CRITICAL();
}
void stop_y()
{
	taskENTER_CRITICAL();
	uInt8 p = ReadDigitalU8(2); //read port 2
	setBitValue(&p, 4, 0);   
	setBitValue(&p, 5, 0);   
	WriteDigitalU8(2, p); //update port 2
	taskEXIT_CRITICAL();
}
void stop_z()
{
	taskENTER_CRITICAL();
	uInt8 p = ReadDigitalU8(2); //read port 2
	setBitValue(&p, 2, 0);
	setBitValue(&p, 3, 0);
	WriteDigitalU8(2, p); //update port 2
	taskEXIT_CRITICAL();
}

void goto_x(int x)
{
	//printf("to a ir x\n");
	//printf("tava aqui %d\n", get_x_pos());
	if (get_x_pos() < x) {
		printf("tava pequeno\n");
		move_x_right();
	}
	else if (get_x_pos() > x) {
		move_x_left();
		printf("tava grande\n");
	}
	//   while position not reached
	//printf("to quase\n");
	while (get_x_pos() != x) 
		Sleep(1);

	//printf("chegei\n\n");
	stop_x();
	
}
void goto_y(int y) 
{
	if (get_y_pos()< y)
		move_y_inside();
	else if (get_y_pos() > y)
		move_y_outside();
	//   while position not reached
	while (get_y_pos() != y)
		Sleep(1);
	stop_y();
}
void goto_z(int z)
{
	//printf("to a ir z\n");
	//printf("tava aqui %d\n", get_z_pos());
	if (get_z_pos() < z) {
		//printf("tava pequeno\n");
		move_z_up();
	}
	else if (get_z_pos() > z) {
		//printf("tava grande\n");
		move_z_down();
	}
	//printf("to quase\n");
	//   while position not reached
	while (get_z_pos() != z)
		Sleep(1);
	//printf("chegei\n\n");
	stop_z();
}

bool is_button_1_on()
{
	int vp1 = ReadDigitalU8(1);
	
	if (getBitValue(vp1, 5))
	{
		//printf("1 %d \n", getBitValue(vp1, 5));
		return true;
	}
	return false;


}
bool is_button_2_on()
{
	int vp1 = ReadDigitalU8(1);
	if (getBitValue(vp1, 6))
		return true;
	return false;
}
bool is_button_both_on()
{
	int vp1 = ReadDigitalU8(1);
	if (getBitValue(vp1, 5) && getBitValue(vp1, 5))
		return true;
	return false;
}

bool is_at_z_up()
{
	int vp0 = ReadDigitalU8(0);
	int vp1 = ReadDigitalU8(1);
	if (!getBitValue(vp0, 6) || !getBitValue(vp1, 0) || !getBitValue(vp1, 2))
		return true;
	return false;
}

bool is_at_z_dn() 
{
	int vp0 = ReadDigitalU8(0);
	int vp1 = ReadDigitalU8(1);
	if (!getBitValue(vp0, 7) || !getBitValue(vp1, 1) || !getBitValue(vp1, 3))
		return true;
	return false;
}

void goto_z_up()
{
	//if (is_at_down_level(1) || is_at_down_level(2) || is_at_down_level(3))
	move_z_up();
	while (!is_at_z_up())
		vTaskDelay(10);
	stop_z();
}
void goto_z_dn()
{
	move_z_down();
	while (!is_at_z_dn(1)  /* …. similar to goto_z_up  */)
		vTaskDelay(10);
	stop_z();
}

void goto_xz(int x, int z, bool _wait_till_done)
{
	//goto_x(x);
	//goto_z(z);
	TPosition pos;
	pos.x = x;
	pos.z = z;
	xQueueSend(mbx_xz, &pos, portMAX_DELAY);
	if (_wait_till_done) {
		while (get_x_pos() != x) { vTaskDelay(1); }
		while (get_z_pos() != z) { vTaskDelay(1); }
	}
}

int is_at_z(Pos)
{
	if (get_z_pos() == Pos)
		return 1;
	else
		return 0;
}
int is_at_x(pos)
{
	if (get_x_pos() == pos)
		return 1;
	else
		return 0;
}
int is_at_y(pos)
{
	if (get_y_pos() == pos)
		return 1;
	else
		return 0;
}
int is_at_cell(x, z)
{
	if (is_at_x(x) && is_at_z(z))
		return 1;
	else
		return 0;
}


int get_x_pos()
{
	uInt8 p = ReadDigitalU8(0);
	if (!getBitValue(p, 2))
		return 1;
	if (!getBitValue(p, 1))
		return 2;
	if (!getBitValue(p, 0))
		return 3;
	return(-1);
}
int get_z_pos()
{
	uInt8 p0 = ReadDigitalU8(0);
	uInt8 p1 = ReadDigitalU8(1);
	if (!getBitValue(p1, 3))
		return 1;
	if (!getBitValue(p1, 1))
		return 2;
	if (!getBitValue(p0, 7))
		return 3;
	return(-1);
}
int get_y_pos()
{
	uInt8 p0 = ReadDigitalU8(0);
	if (!getBitValue(p0, 5))
		return 1;
	if (!getBitValue(p0, 4))
		return 2;
	if (!getBitValue(p0, 3))
		return 3;



	return(-1);
}

int get_level() {
	uInt8 p0 = ReadDigitalU8(0);
	uInt8 p1 = ReadDigitalU8(1);

	if (!getBitValue(p1, 2) || !getBitValue(p1, 0) || !getBitValue(p0, 6))
		return 1;
	if (!getBitValue(p1, 3) || !getBitValue(p1, 1) || !getBitValue(p0, 7))
		return 0;
	else
		return -1;

}
void gotoLowLevel()
{
	move_z_down();

	do
	{
		Sleep(1);
	} while (get_level() == 1 || get_level() == -1);

	stop_z();

}
void gotoUpLevel()
{
	//printf("e pa subir pa onde? (%d) \n", get_z_pos());
	//printf("puta \n");
	move_z_up();
	//printf("ola babes tudo bem \n");

	
	do
	{
		//printf("comia te");
		//printf("%d \n ", get_level());
		Sleep(1);
	} while (get_level() == 0 || get_level()==-1);

	//printf("ja ca estou \n ");
	stop_z();
}

void putPiece() {
	// it is expected that lift is well positioned at a cell
	// it is expected that “center” (y==2) sensor of the // cage is  activated
	
	//printf("SOBE\n");
	gotoUpLevel();

	//printf("to a entrar\n");
	goto_y(3);

	//printf("e DESCE\n");
	gotoLowLevel();

	//printf("marcha a tras\n");
	goto_y(2);
	
}
void get_piece() 
{
	goto_y(3);
	gotoUpLevel();
	goto_y(2);
	gotoLowLevel();
}

void show_menu()
{
	printf("\npress: w,a,s,d e->stop p-put o-> retrive g->(xy) or t->terminate");
	printf("\n(1) sw1 ....... Put a part in a specific x, z position");
	printf("\n(2) sw2 ....... Retrieve a part from a specific x, z position");
	printf("\n(3) sw1 AND sw2  (slight) ……. EMERGENCY STOP");
	printf("\n(4) sw1 OR sw2  ………………….. RESUME");
	printf("\n(5) sw1 AND sw2  (long)  …….. end program");
	//…./….
}

void callibration() {
	move_y_outside();
	while (!is_at_y(2)) {
		_sleep(10);
	}
	stop_y();

	move_x_right();
	while (!is_at_x(3)) {
		_sleep(10);
	}
	stop_x();
	goto_x(1);
	move_z_down();
	while (!is_at_z(1)) {
		_sleep(10);
	}
	stop_z();
}

void task_storage_services(void *param)
{
	int cmd = -1;
	while (true) {
		show_menu();
		// get selected option from keyboard
		printf("\n\nEnter option=");
		xQueueReceive(mbx_input, &cmd, portMAX_DELAY);

		if (cmd == 'a') // hidden option
			move_x_left(); // not in the show_menu  
		if (cmd == 'd') // hidden option            
			move_x_right();  // not in the show_menu  
		if (cmd == 'w')
			move_z_up();
		if (cmd == 's')
			move_z_down();
		if (cmd == 'e') { // hidden option  
			stop_x();
			stop_z();
		}
		if (cmd == 'p')
			putPiece();
		if (cmd == 'o')
			get_piece();
		if (cmd == 'g') // hidden option  
		{
			int x, z; // you can use scanf or one else you like
			printf("\nX=");
			xQueueReceive(mbx_input, &x, portMAX_DELAY);

			x = x - '0'; //convert from ascii code to number
			printf("\nZ=");
			xQueueReceive(mbx_input, &z, portMAX_DELAY);
			z = z - '0'; //convert from ascii code to number

			if (x >= 1 && x <= 3 && z >= 1 && z <= 3)
				goto_xz(x, z, false);
			else
				printf("\nWrong (x,z) coordinates, are you sleeping?... ");
		}
		if (cmd == 't') // hidden option
		{
			WriteDigitalU8(2, 0); //stop all motores;
			vTaskEndScheduler(); // terminates application
		}
	}   // end while
}

void manage_expiration_task(void *param)
{
	time_t rawtime;
	int i = 1;
	int j = 1;
	int time_val = 0;
	int c;
	bool there;
	

	while(true)
	{
		
			time_val = difftime(time(&rawtime), matriz[i-1][j-1].validade);
			there = matriz[i-1][j-1].ocupado;
			if (time_val > 30 && there)
			{
				//xQueueSend(mbx_expired, &c, portMAX_DELAY);
				//printf("date %d \n", time_val);
				printf("expired \n");
				//printf("pos %d %d \n", i, j);
			}
			//printf("pos %d %d \n", i,j);
			i++;
			if(i==4)
			{
				i = 1;
				j++;
				if (j == 4) {
					i = 1;
					j = 1;
				}
			}
		
	}

	
}

void add_pallet_matriz_task(void *param)
{
	int x;
	int z;
	TPosition pos;
	time_t rawtime;
	struct tm * timeinfo;
	 
	while (true)
	{
		xQueueReceive(mbx_pallet_in, &pos, portMAX_DELAY);
		
		time(&rawtime);
	
		matriz[global_Pos.x - 1][global_Pos.z - 1].ocupado = true;
		matriz[global_Pos.x - 1][global_Pos.z - 1].validade = rawtime;
		
	}
}


void goto_x_task(void *param)
{
	while (true)
	{
		int x;
		xQueueReceive(mbx_x, &x, portMAX_DELAY);
		goto_x(x);
		xSemaphoreGive(sem_x_done);
	}
}
void goto_z_task(void *param)
{
	while (true)
	{
		int z;
		xQueueReceive(mbx_z, &z, portMAX_DELAY);
		goto_z(z);
		xSemaphoreGive(sem_z_done);
	}
}
void goto_xz_task(void *param)
{
	while (true) {
		TPosition position;
		//wait until a goto request is received
		xQueueReceive(mbx_xz, &position, portMAX_DELAY);

		//send request for each goto_x_task and goto_z_task 
		xQueueSend(mbx_x, &position.x, portMAX_DELAY);
		xQueueSend(mbx_z, &position.z, portMAX_DELAY);

		//wait for goto_x completion
		xSemaphoreTake(sem_x_done, portMAX_DELAY);

		//wait for goto_z completion           
		xSemaphoreTake(sem_z_done, portMAX_DELAY);
	}
}

void put_task(void *param)
{
	int x;
	int pos_x;
	int pos_y;
	TPosition pos;
	pos.x = 0;
	pos.z = 0;
	
	
	while (true)
	{
		int sw1 = 0;
		int sw2 = 0;
		int i = 0;
		int pos_x = 0;
		int pos_z = 0;

		xSemaphoreTake(put_start, portMAX_DELAY);
		vTaskDelay(100);
		printf("\nINSIRA POS X (TEM 2 SEGUNDOS) \n");
		while (i < 200)
		{
			if (is_button_1_on()) {
				sw1 = 1;
				//printf(" o 1 ta ligado caralho");
			}
			if (is_button_2_on()) {
				sw2 = 1;
			}
			vTaskDelay(1);
			i++;
			//printf("1 %d \n", i);
		}
		i = 0;

		if (!sw1 && !sw2)
		{
			pos_x = 1;
		}
		if (!sw1 && sw2)
		{
			pos_x = 2;
		}
		if (sw1 && !sw2)
		{
			pos_x = 3;
		}

		printf("Posicao x = %d \n", pos_x);
		printf("INSIRA POS Y (TEM 2 SEGUNDOS) \n");
		sw1 = 0;
		sw2 = 0;
		while (i < 200)
		{
			if (is_button_1_on())
				sw1 = 1;
			if (is_button_2_on())
				sw2 = 1;
			vTaskDelay(1);
			i++;
		}
		i = 0;
		if (!sw1 && !sw2)
			pos_z = 1;

		if (!sw1 && sw2)
			pos_z = 2;

		if (sw1 && !sw2)
			pos_z = 3;

		printf("Posicao z = %d \n", pos_z);


		goto_xz(pos_x, pos_z, true);
		global_Pos.x = pos_x;
		global_Pos.z = pos_z;
		//printf("q estas qui a fazer puta \n");
		
		putPiece();
		xQueueSend(mbx_pallet_in, &pos, portMAX_DELAY);
		xSemaphoreGive(put_done);
	}
}
void get_task(void *param)
{
	int x;
	int pos_x;
	int pos_y;

	while (true)
	{
		int sw1 = 0;
		int sw2 = 0;
		int i = 0;
		int pos_x = 0;
		int pos_z = 0;

		xSemaphoreTake(take_start, portMAX_DELAY);
		vTaskDelay(100);
		printf("INSIRA POS X (TEM 2 SEGUNDOS) \n");
		while (i < 200)
		{
			if (is_button_1_on()) {
				sw1 = 1;
				//printf(" o 1 ta ligado caralho");
			}
			if (is_button_2_on()) {
				sw2 = 1;
			}
			vTaskDelay(1);
			i++;
			//printf("1 %d \n", i);
		}
		i = 0;

		if (!sw1 && !sw2)
		{
			pos_x = 1;
		}
		if (!sw1 && sw2)
		{
			pos_x = 2;
		}
		if (sw1 && !sw2)
		{
			pos_x = 3;
		}

		printf("Posicao x = %d \n", pos_x);
		printf("INSIRA POS Y (TEM 2 SEGUNDOS) \n");
		sw1 = 0;
		sw2 = 0;
		while (i < 200)
		{
			if (is_button_1_on())
				sw1 = 1;
			if (is_button_2_on())
				sw2 = 1;
			vTaskDelay(1);
			i++;
		}
		i = 0;
		if (!sw1 && !sw2)
			pos_z = 1;

		if (!sw1 && sw2)
			pos_z = 2;

		if (sw1 && !sw2)
			pos_z = 3;

		printf("Posicao z = %d \n", pos_z);


		goto_xz(pos_x, pos_z, true);
		//printf("q estas qui a fazer puta \n");
		get_piece();
		matriz[pos_x-1][pos_z-1].ocupado = false;
		xSemaphoreGive(take_done);
	}
}

void receive_instructions_task(void *ignore) {
	int c = 0;
	while (true) {
		c = getch();
		putchar(c);
		xQueueSend(mbx_input, &c, portMAX_DELAY);
	}
}

void manage_buttons_task(void *param)
{
	int x = 0;
	while (true) {

		xQueueReceive(mbx_button, &x, portMAX_DELAY);
		if (x == 1)
		{
				xQueueSend(mbx_input_switch, &x, portMAX_DELAY);
		}

		if (x == 2)
		{
				xQueueSend(mbx_input_switch, &x, portMAX_DELAY);
		}

		if (x == 3)
		{
		}

	}
}

void button_task(void *param)
{
	int i = 0;
	while (true)
	{

		int vp1 = ReadDigitalU8(1);
		if (getBitValue(vp1, 5))
		{
			i = 1;
			xSemaphoreGive(put_start);
			xSemaphoreTake(put_done, portMAX_DELAY);
			i = 0;
		}
		if (getBitValue(vp1, 6))
		{
			i = 2;
			xSemaphoreGive(take_start);
			xSemaphoreTake(take_done, portMAX_DELAY);
			i = 0;
		}
		if (getBitValue(vp1, 5) && getBitValue(vp1, 6))
		{
			i = 3;
			xQueueSend(mbx_button, &i, portMAX_DELAY);
			xSemaphoreTake(put_done, portMAX_DELAY);
			i = 0;
		}
	}		
}


void setup()
{
	int i = 1;
	int j = 1;

	while(j != 4){
		matriz[i][j].ocupado = false;
		i++;
		if (i == 4)
		{
			i = 1;
			j++;
		}
	}

}







void main(int argc, char **argv) {
	create_DI_channel(0);
	create_DI_channel(1);
	create_DO_channel(2);

	printf("\nwaiting for hardware simulator...");
	//WriteDigitalU8(2, 0);
	printf("\ngot access to simulator...");

	sem_x_done = xSemaphoreCreateCounting(1000, 0);  
	sem_z_done = xSemaphoreCreateCounting(1000, 0);
	put_done= xSemaphoreCreateCounting(1000, 0);
	take_done = xSemaphoreCreateCounting(1000, 0);
	put_start = xSemaphoreCreateCounting(1000, 0);
	take_start = xSemaphoreCreateCounting(1000, 0);
	


	mbx_x = xQueueCreate(10, sizeof(int));
	mbx_z = xQueueCreate(10, sizeof(int));
	mbx_xz = xQueueCreate(10, sizeof(TPosition));
	mbx_input = xQueueCreate(10, sizeof(int));
	mbx_button = xQueueCreate(10, sizeof(int));
	mbx_input_switch= xQueueCreate(10, sizeof(int));
	mbx_pallet_in= xQueueCreate(10, sizeof(int));
	mbx_expired= xQueueCreate(10, sizeof(int));

	xTaskCreate(task_storage_services, "task_storage_services", 100, NULL, 0, NULL);
	xTaskCreate(goto_x_task, "goto_x_task", 100, NULL, 0, NULL);
	xTaskCreate(goto_z_task, "goto_z_task", 100, NULL, 0, NULL);
	xTaskCreate(goto_xz_task, "v_gotoxz_task", 100, NULL, 0, NULL);
	xTaskCreate(receive_instructions_task, "receive_instructions_task", 100, NULL, 0, NULL);
	xTaskCreate(button_task, "button_task",100, NULL, 0, NULL);
	xTaskCreate(manage_buttons_task, "manage_buttons_task",100, NULL, 0, NULL);
	xTaskCreate(put_task, "put_task", 100, NULL, 0, NULL);
	xTaskCreate(get_task, "get_task", 100, NULL, 0, NULL);
	xTaskCreate(manage_expiration_task, "manage_expiration_task", 100, NULL, 0, NULL);
	xTaskCreate(add_pallet_matriz_task, "add_pallet_matriz_task", 100, NULL, 0, NULL);

	setup();

	callibration();

	vTaskStartScheduler();
}

