/*****************************************************************************************************************
* ����� ��� ������ ������ ModBus ��������� ���������� �������0.
******************************************************************************************************************/
#ifndef _NET_										//������ �� ���������� ��������� �����
#define _NET_

#include <avr/io.h>								//����������� ���������� �����/������
#include <avr/interrupt.h>						//���������� ��������� ����������
#include <util/delay.h>							//����������� ���������� ��������� ��������

#ifndef F_CPU
# warning "F_CPU not defined. Frequency by default 14.7456MHz"
#define F_CPU 14745600UL							/*����������� ��������� �������� ������� ���������������� (���������� ��� ���������� ������ ������� ��������)*/
#endif

#include "ioport.h"
#include "mem.h"

#ifndef COM_RX_BUFFER_SIZE
# warning "COM_RX_BUFFER_SIZE not defined. Value will be set by default 64 bytes"
#define COM_RX_BUFFER_SIZE 64						/*����� ��������� ������*/
#endif
#ifndef COM_TX_BUFFER_SIZE
# warning "COM_TX_BUFFER_SIZE not defined. Value will be set by default 64 bytes"
#define COM_TX_BUFFER_SIZE 64						/*����� ����������� ������*/
#endif


//---------------------------- ������ � ����� RS485 ---------------------------
// --------------------- ��� ����������� ���� ��01-8600.� ---------------------
#define NET_DDR               DDRD
#define NET_PIN               PIND
#define NET_PORT              PORTD
#define NET_RX                0x00
#define NET_TX                0x01
#define NET_TXE               0x02

#define NET_UBRRH             UBRR0H
#define NET_UBRRL             UBRR0L
#define NET_UCSRA             UCSR0A
#define NET_UCSRB             UCSR0B
#define NET_UCSRC             UCSR0C
#define NET_UDR               UDR0

//---------------------------- ������ � ���������� ModBus ---------------------
char _Frame_pause;								//���� ���������� ��������� 1.5 ������� � Modbus RTU (1-�������� ��������/0-���������� �� ����)

char _UART_RX_Buf[COM_RX_BUFFER_SIZE];	//�������� ����� UART
char _UART_RX_dup[COM_RX_BUFFER_SIZE];	//����� ��������� ������ UART
int _UART_RX_point;						//������� ������� � �������� ������ UART
char _UART_RX_length;					//����� �������� ������� �� UART
char _UART_RX_end;						//������� ��������� �������� ������� �� UART

char _UART_TX_Buf[COM_TX_BUFFER_SIZE];	//���������� ����� UART
int _UART_TX_point;						//������� ������� � ���������� ������ UART
char _UART_TX_length;					//������ ������������ ������� �� UART
char _UART_TX_end;						//������� ��������� �������� �� UART

// ********************** �������� ������������ ���������� ���������� ��� ��������� MODBUS RTU *********************************
unsigned char m_coderr;	// �������� ��� ������
char Func;				//��� ������� (Func) �������� ����������� �������
int SubFunc;			//��� ���������� (SubFunc) ��������� ����� ������������ ���� �� �������
int Data;				//������� ������ (Data) ���-�� ������������� ������� (��������� ��� �������)

//-------------------------------------
void MODBUS_RTU(void);
//-------------------------------------
//======================== ���������� �� ���������� � ������� 0 ============================
// === ����� ������ ===
//------------------------------------------------------------------------------------------
SIGNAL(TIMER0_COMPA_vect)
{
	int temp;														//��������������� ����������
	
	_Frame_pause=0;													//��������� ����� ���������� ��������� 1.5 �������
	for (temp=0; temp<_UART_RX_point; temp++) _UART_RX_dup[temp]=_UART_RX_Buf[temp];//����������� ��������� ������
	_UART_RX_length=_UART_RX_point;								    //���������� ����� �������� �������
	_UART_RX_point=0;										        //��������� ��������� ������ �� ������
	TCCR0A=0x00;													//��������� �������
	TCCR0B=0x00;													//��������� �������
	_UART_RX_end=1;
//	LD_on;										// ��� ���������
}

//------------------------------------------------------------------------------------------
//======================== ���������� �� ���������� B ������� 0 ============================
//------------------------------------------------------------------------------------------
SIGNAL(TIMER0_COMPB_vect)
{
	_Frame_pause=1;														//���������� ���� ���������� ��������� 1.5 �������
}

//================================== ������� ������� ����������� ����� � ��������� Modbus RTU ====================================
//������� ���������:
//*buf-��������� �� ������ � ������� ���������� ������ ��� ������� ����������� �����
//Len-���������� ���� ����������� � ������� ����������� �����
int _CRC_calc(char *buf, char Len)
{
	unsigned int CRC;														//���������� ��� ������� ����������� �����
	char temp,temp2;														//��������������� ����������
	
	CRC=0xFFFF;																//��������� �������� CRC (�� ��������� ������� CRC)
	for (temp=0; temp<Len; temp++)											//���� ��� ���� ���� ����������� � �������
	{
		CRC=CRC^*buf;														//"����������� ���" (�� ��������� ������� CRC)
		buf++;																//��������� �� ��������� ����
		for (temp2=0; temp2<8; temp2++)										//���� ��� ������� ���� � �����
		if ((CRC & 0x0001)==0) CRC=CRC>>1;									//���� ������� ��� � �������, ������ ����� (�� ��������� ������� CRC)
		else
		{
			CRC=CRC>>1;														//�����	� "����������� ���" (�� ��������� ������� CRC)
			CRC=CRC^0xA001;													//������� ��� "������������ ���" 1010 0000 0000 0001
		}
	}
	
	return CRC;																//������� ����������� ����������� �����
}

/****************************************************************************************************************************************
Name:         void NetRxChar(void)
Description:  �-� ���������� �� ���������� - ���� ������ ���� � ����� UDR(����� ��������)
Output:       Data in NetRxData[]
new:		�������� ���� ����� � ����� �������������� ������� �������� ��������� ������ �� ModBus
*****************************************************************************************************************************************/
ISR(USART_RX_vect)
{
	unsigned char InByte; // ������� ����� - ��� ��������� �����

	if (NET_UCSRA & ((1<<FE0)+(1<<DOR0))) {InByte = NET_UDR; return;} // ��� ������ ������������ � ������������ ������
	InByte = NET_UDR;												//������ �������� ���������
	if (_Frame_pause==0)                                            //���� ���� ���������� ��������� 1.5 �������
	{
		_UART_RX_Buf[_UART_RX_point]=InByte;						//��������� �������� � �����
		TIFR0|=(1<<OCF0A)|(1<<OCF0B);                               //��������� ����� ���������� �� ������� (���������� ���� ���������� �������� � ������ ���������� ������� �����������)
		TCNT0=0;                                                    //��������� �������� �������� �������
		TCCR0A=(1<<WGM01);											// ��������� ������ ������ ������� "����� ��� ����������"
		TCCR0B=(1<<CS02);											//��������� ������������ ������� �������� ������� 256
		_UART_RX_point++;                                           //���������� �������� �������� ����
		if (_UART_RX_point>=(COM_RX_BUFFER_SIZE-1)) _UART_RX_point=0; //������ �� ������� ������������� �������
	}
	// ���������� �����������
//	if((_UART_RX_point == 1) && (InByte == (Address_device & 0xFF))) LD_off;	// ���� ��������� ���� ������ ���� = ������
}

//=========================================== ������� ������������� COM ����� ============================================================
//������� ��������� � �������� ����������:
//COM_speed - ��������� �������� �������� COM �����
//COM_param - ��������� �������� ���������� COM �����
//������� ����������:
//0x00-���� ��� ��������� ������ ���������, � ��������� ������ 0xFF
//========================================================================================================================================
char COM_Init(char COM_speed,char COM_param)
{
	unsigned char m;
	float temp;
	long Speed;
	switch (COM_speed)
		{
			case 0x03: Speed=1200; break;
			case 0x04: Speed=2400; break;
			case 0x05: Speed=4800; break;
			case 0x06: Speed=9600; break;
			case 0x07: Speed=19200; break;
			case 0x08: Speed=38400; break;
			case 0x09: Speed=57600; break;
			case 0x0A: Speed=115200; break;
			default: {Speed=9600;}
		}
	
	if ((Speed<=0) || (Speed>1000000)) return 0xFF;					//���� �� ����� ������ ��������
	
	temp=(float)F_CPU/Speed/16-0.5;									//������ �������� �������� �������� UART
	if (temp>=0.5)													//���� �������� ���������� ��� ������� ������
	{
			NET_UBRRH =(unsigned long)temp>>8;
			NET_UBRRL =(unsigned long)temp & 0x000000FF;
	}
	else return 0xFF;												//�����, ����� � �������

	switch (COM_param)
		{
			case 0x01: NET_UCSRC = (1<<UMSEL01)+(1<<UPM01)+(1<<USBS0)+(1<<UCSZ01)+(1<<UCSZ00); break;	// 8 bit, 2 ���� ����, event(8E2)
			case 0x02: NET_UCSRC = (1<<UMSEL01)+(1<<UCSZ01)+(1<<UCSZ00); break;							// 8 bit, 1 ���� ����, none (8N1)
			case 0x03: NET_UCSRC = (1<<UMSEL01)+(1<<USBS0)+(1<<UCSZ01)+(1<<UCSZ00); break;				// 8 bit, 2 ���� ����, none (8N2)
			default:   NET_UCSRC = (1<<UMSEL01)+(1<<UPM01)+(1<<UCSZ01)+(1<<UCSZ00);						// 8 bit, 1 ���� ����, event(8E1)
		}
	NET_UCSRB=(1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0)|(1<<TXCIE0);	//��������� ������-�����������(��������� RX, TX � ����������)
		
	m = NET_UDR; // ������� � �������� �� �������!!! ����� ���������))))
	m = NET_UDR; // ������� � �������� �� �������!!! ����� ���������))))
	m = NET_UDR; // ������� � �������� �� �������!!! ����� ���������))))
	m = NET_UDR; // ������� � �������� �� �������!!! ����� ���������))))
	
	// --- ��������� ������� ����������� ��� ������ � RS485 ---
	NET_PORT&=~(1<<NET_TX);									// �� TX - ������ - 0
	NET_DDR|=(1<<NET_TX);									// ����� TX - �����
	NET_PORT|=(1<<NET_RX);									// RX - �������� � +
	NET_DDR&=~(1<<NET_RX);									// ����� RX - ����
	NET_PORT&=~(1<<NET_TXE);								// ����������� UART0 �� �����, �� RXE - 0
	NET_DDR|=(1<<NET_TXE);									// ��������� ����� ���������� ��������� RS485, ����� RXE - �����
		
	// �������� ������� ������
	for(m=0;m<COM_RX_BUFFER_SIZE;m++) {_UART_RX_Buf[m] = 0;}
	for(m=0;m<COM_RX_BUFFER_SIZE;m++) {_UART_RX_dup[m] = 0;}
	for(m=0;m<COM_TX_BUFFER_SIZE;m++) {_UART_TX_Buf[m] = 0;}
	
	_UART_RX_point=0;										//��������� �� ������ ������
	_UART_RX_end=0;											//�������� ������� ��������� ������
	_UART_TX_end=1;											//���������� ���� ���������� �������
		
	// ���������� ������ �������0
	if (Speed>=19200) Speed=19200;							//���� �������� �������� 19200 ���/��� ��� ������, ������������ ������ ����� 1.75 ��.
	OCR0A=35*(float)F_CPU/Speed/256+0.5;					//������ ������������ 3.5 ��������
	OCR0B=15*(float)F_CPU/Speed/256+0.5;					//������ ������������ 1.5 ��������
	TIMSK0|=(1<<OCIE0A)|(1<<OCIE0B);						//���������� ���������� �� ���������� � � B ������� 0
	return 0;
}

//================================================= ������� ������ � COM ���� ===============================================================
//========================================= ������� ������� �������� ���������� �� UART =====================================================
//������� ��������� � �������� ����������:
//Len - ����� ������������ ������

void _UART_Go(int Len)
{
//	LD_on;													// ��� ���������
	_UART_TX_length=Len;									//����� ������������ ������
	_UART_TX_point=0;										//������� � ���������� ������
	_UART_TX_end=0;											//����� ����� ��������� �������� (�������� ����� �������)
	NET_UCSRB |= (1<<UDRIE0); 								// ��������� ���������� ��� ������� ������
	NET_PORT |= (1 << NET_TXE);								// �������������� �������� �� RS485
}
/********************************************************************************************************************************************
Name:         void NetTxByte(void)
Description:  �-� ���������� �� ���������� - ����� UDR ����
*********************************************************************************************************************************************/
ISR(USART_UDRE_vect)
{
  if (_UART_TX_point!=_UART_TX_length)			// ���� ���������� �� ��� �������
  {
    asm("wdr");
    NET_PORT |= NET_TXE;						// �������������� ��������
    NET_UDR = _UART_TX_Buf[_UART_TX_point];		//�������� ����� �� UART
    _UART_TX_point++;							// ��������� ���������
  }
  else NET_UCSRB &= ~(1<<UDRIE0);  /* ��������� ���������� TX ���� ��� ������� ��������*/
}

/********************************************************************************************************************************************
Description:  �-� ���������� �� ���������� - �������� ���������
              TxBuffer ���� �������� � NET.
*********************************************************************************************************************************************/
ISR(USART_TX_vect)
{
//  LD_off;							// ���� ���������
  NET_PORT &= ~(1 << NET_TXE);		// ����������� � ����� ������ RS485
  _UART_TX_end=1;					//���������� ���� ���������� ��������
}

/************************************* ������� ������ �������� ������� � ��������� Modbus RTU *****************************************
* ������� ���������: void
* ������� ����������:
* 	0x00-���� �������� ����� UART ����;
* 	0x01-������ �� ��������� ������ UART ���������� � ����� ��� ������;
* 	0x80-�� ������������ ���������� (������ �������� DCON);
* 	0xFF-�� ���������� ����������� ����� �������� ������.
* ������� ������� ��������� ������ � ������, ����� �������� ������ � �������� �������� ���������.
* ��� ����� ����������� ����� �������� � ������ ������.
* ������� ������� ��������� ������� �������� ���������� ���������� ����, ������ ����� ����������� �����.
****************************************************************************************************************************************/

char Read_Modbus(void)
{
	unsigned int CRC;									//���������� ��� ����������� �����
	
	if (_UART_RX_end==0) return 0;						//����� ���� ������� ��������� ������ ������ �� ����������
	_UART_RX_end=0;										//����� ����� ��������� ������
	if(_UART_RX_Buf[0] != Address_device) return 0xFD;	// ���� ����� �� ���
	if(_UART_RX_length<=5) return 0xFE;
	CRC=_CRC_calc(_UART_RX_Buf,_UART_RX_length-2);		//������ CRC
	// �������� ����������� �����, ���� ��������� � ����������� CRC16 �� ���������
	if (((CRC>>8)!=_UART_RX_Buf[_UART_RX_length-1]) || ((CRC & 0xFF)!=_UART_RX_Buf[_UART_RX_length-2])) return 0xFF; // ������� � ������� CRC

	return 1;
}

//======================================================================================================================================================================
//																	MODBUS RTU
//======================================================================================================================================================================

/*****************************************************************************
 ������ ��������� ���������� ������� DO � ���������� ������ DI
******************************************************************************
*****************************************************************************/
void MODBUS_K1_2(void)
{
//unsigned int n;
int n;
unsigned char b,d,nb,c;
asm("wdr");
d = SubFunc + Data;
n = 0;
// ������� ���-�� ���������� ����
nb = Data/8;
if(Data%8 > 0) nb += 1; // ���� ���� ������� - ����������� �� ������ �����

if(Func == 1) // ---------------- ���� ���� ������� ��������� DO -----------------------------
  {
  if(d > maxAddr_DO) {m_coderr=2; return;} // ���� ������������� ������ ��������� DO
  for(b = SubFunc,c = 0; b < d; b++) // ��������� ����������� ������� ���������� n
    {
	if(OutState(b) != 0) n|=(0x01 << c); // ���� ����������� ��� �� = 0 - ��������� ��������������� ��� � ����������
	c++;
	}
  }
else 			  // ----------------- ���� ���� ������� ��������� DI -----------------------------
  {
  if(d > maxAddr_DI) {m_coderr=2; return;} // ���� ������������� ������ ��������� DI
  for(b = SubFunc,c = 0; b < d; b++) // ��������� ����������� ������� ���������� n
    {
	if(InState(b) != 0) n|=(0x01 << c); // ���� ����������� ��� �� = 0 - ��������� ��������������� ��� � ����������
	c++;
	}
  }
// =================================== ������ �������� ����� =============================
 b = 0;										// �������� ���������
 _UART_TX_Buf[b++] = Address_device & 0xFF; // ������� ����� �����������
 _UART_TX_Buf[b++] = Func; 					// ������� ���������� �������
 _UART_TX_Buf[b++] =  nb;					// ������� ���- �� ���������� ����
 _UART_TX_Buf[b++] = n & 0xFF; 	   			// 1 ���� � ����� ������
 if(nb == 2) _UART_TX_Buf[b++] = ((n >> 8) & 0xFF); // 2 ���� - ���� ����
 // --------------- ������� CRC -------------------
 n = _CRC_calc(_UART_TX_Buf, b);				// ������� CRC
 _UART_TX_Buf[b++] = n & 0xFF;	  			// ����� ������� ���� - ������� �������� ������� ����
 _UART_TX_Buf[b++] = ((n >> 8) & 0xFF);		// ����� ������� ���� 

 _UART_TX_length = b;	  	  		 // ������� ��� - �� ������������ ���� �� ������
}

/*****************************************************************************
 ������ ���������� Holding ��� Input ��������� (������)
******************************************************************************
*****************************************************************************/
void MODBUS_K3_4(void)
{
// ���������� ����������
unsigned int n;
unsigned char m,b,d;
asm("wdr");
d = SubFunc + Data;
b = 0;

// ���������� �����
 _UART_TX_Buf[b++] = Address_device & 0xFF; // ������� ����� �����������
 _UART_TX_Buf[b++] = Func; 					// ������� ���������� �������
 _UART_TX_Buf[b++] = (Data<<1) & 0xFF;		// ���-�� ���� = ���-�� ����*2 � �������� �����
// ������� ��������
if(Func == 3) // ���� ���� ������� HOLDING ��������
  {
  if(d > maxAddrRegHOLD) {m_coderr=2; return;} // ������!!! ������������� ������ ���������
  for(m = SubFunc; m < d; m++) // ��������� ����������� ������� ����� ������
   {
   _UART_TX_Buf[b++] = (MB_Hreg[m] >> 8) & 0xFF; // ���������� ������� ����
   _UART_TX_Buf[b++] =  MB_Hreg[m] & 0xFF;  	 // ���������� ������� ����
   }
  }
else 			  // ���� ���� ������� INPUT ��������
  {
  if(d > maxAddrRegINP) {m_coderr=2; return;} // ������!!! ������������� ������ ���������
  for(m = SubFunc; m < d; m++) // ��������� ����������� ������� ����� ������
   {
   _UART_TX_Buf[b++] = (MB_Ireg[m] >> 8) & 0xFF; // ���������� ������� ����
   _UART_TX_Buf[b++] =  MB_Ireg[m] & 0xFF;  	   // ���������� ������� ����
   }
  }
// --- ������� CRC ---
 n = _CRC_calc(_UART_TX_Buf,b);				// ������� CRC
 _UART_TX_Buf[b++] = n & 0xFF;	  			// ����� ������� ���� - ������� �������� ������� ����
 _UART_TX_Buf[b++] = ((n >> 8) & 0xFF);		// ����� ������� ���� - ����� ������� ����

 _UART_TX_length = b;	  	  		 // ������� ��� - �� ������������ ���� �� ������
}

/*****************************************************************************
 ��������� ���������� ������ DO
******************************************************************************
*****************************************************************************/
void MODBUS_K5(void)
{
unsigned int n;
unsigned char b,d;
asm("wdr");
d = SubFunc + Data;
n = 0;
if(d > maxAddr_DO) {m_coderr=2; return;} // ���� ������������� ������ ���������
// ������ ��������� 1 ������ DO
if((_UART_RX_dup[4] == 0xFF) && (_UART_RX_dup[5] == 0x00)) OutControl(SubFunc,1);      // ���� ���������� (��� �����)
else if((_UART_RX_dup[4] == 0x00) && (_UART_RX_dup[5] == 0x00)) OutControl(SubFunc,0);	// ���� �������� (���� �����)
else {m_coderr=3; return;} // ���� ������� ������ ���� ������
// *************** ������ �������� ����� **********************************
b = 0;
 _UART_TX_Buf[b++] = Address_device & 0xFF;   		// ������� ����� �����������
 _UART_TX_Buf[b++] = Func; 		   					// ������� ���������� �������
 _UART_TX_Buf[b++] = _UART_RX_dup[2];				// ������� ��. ���� ������
 _UART_TX_Buf[b++] = _UART_RX_dup[3]; 	   	   		// ������� ��. ���� ������
 _UART_TX_Buf[b++] = _UART_RX_dup[4];				// ������� ��. ���� ������
 _UART_TX_Buf[b++] = _UART_RX_dup[5]; 	   	   		// ������� ��. ���� ������
// ---- ������� CRC ----
 n = _CRC_calc(_UART_TX_Buf,b);				// ������� CRC
 _UART_TX_Buf[b++] = n & 0xFF;	  		 // ����� ������� ���� - ������� �������� ������� ����
 _UART_TX_Buf[b++] = ((n >> 8) & 0xFF); // ����� ������� ���� - ����� ������� ����

_UART_TX_length = b;	  	  		 // ������� ��� - �� ������������ ���� �� ������
}

/*****************************************************************************
������ � ���� ������� Holding 
******************************************************************************
*****************************************************************************/
void MODBUS_K6(void)
{
unsigned int n;
unsigned char b;
asm("wdr");
n = 0;
if(SubFunc > maxAddrRegHOLD) {m_coderr=2; return;} // ���� ������������� ��������� �������� ������ ���������

MB_Hreg[SubFunc] = (_UART_RX_dup[4] << 8) | _UART_RX_dup[5]; // ������ ���������� � Holding ������� �� ���������� ������
// --- ��� ����������� ��01-3200.� ---
#ifdef _KM3200_
if(SubFunc == 6) wr_mcnt();	// ���� ��� ������������ ��������� - �������� ����� �������� � EEPROM
else if(SubFunc == 7) wr_vcnt1();	// ���� ��� �������� �������� ��� �������� �1 - �������� ����� �������� � EEPROM
else if(SubFunc == 8) wr_vcnt2();	// ���� ��� �������� �������� ��� �������� �2 - �������� ����� �������� � EEPROM
else if(SubFunc == 9) wr_vcnt3();	// ���� ��� �������� �������� ��� �������� �3 - �������� ����� �������� � EEPROM
else if(SubFunc == 10) wr_vhcnt1();	// ���� ��� ������� ������� ����� ��� �������� �1 - �������� ����� �������� � EEPROM
else if(SubFunc == 11) wr_vlcnt1();	// ���� ��� ������� ������� ����� ��� �������� �1 - �������� ����� �������� � EEPROM
else if(SubFunc == 12) wr_vhcnt2();	// ���� ��� ������� ������� ����� ��� �������� �2 - �������� ����� �������� � EEPROM
else if(SubFunc == 13) wr_vlcnt2();	// ���� ��� ������� ������� ����� ��� �������� �2 - �������� ����� �������� � EEPROM
else if(SubFunc == 14) wr_vhcnt3();	// ���� ��� ������� ������� ����� ��� �������� �3 - �������� ����� �������� � EEPROM
else if(SubFunc == 15) wr_vlcnt3();	// ���� ��� ������� ������� ����� ��� �������� �3 - �������� ����� �������� � EEPROM
else if(SubFunc == 29) wr_adr();	// ���� ��� ����� ����������� - �������� ����� �������� � EEPROM
else if(SubFunc == 30) wr_par();	// ���� ��� ��������� ���� - �������� ����� �������� � EEPROM
#endif

// *************** ������ �������� ����� **********************************
b = 0;
 _UART_TX_Buf[b++] = Address_device & 0xFF;			// ������� ����� �����������
 _UART_TX_Buf[b++] = Func; 		   					// ������� ���������� �������
 _UART_TX_Buf[b++] = _UART_RX_dup[2];				// ������� ��. ���� ���������� ������ ��������
 _UART_TX_Buf[b++] = _UART_RX_dup[3]; 	   	   		// ������� ��. ���� ���������� ������ ��������
 _UART_TX_Buf[b++] = _UART_RX_dup[4];				// ������� ��. ���� ������ ��������
 _UART_TX_Buf[b++] = _UART_RX_dup[5]; 	   	   		// ������� ��. ���� ������ ��������
// ---- ��� - �� ����� ��� ��� �������� CRC ----
 n = _CRC_calc(_UART_TX_Buf,b);				// ������� CRC
 _UART_TX_Buf[b++] = n & 0xFF;	  			// ����� ������� ���� - ������� �������� ������� ����
 _UART_TX_Buf[b++] = ((n >> 8) & 0xFF);		// ����� ������� ���� - ����� ������� ����
 
_UART_TX_length = b;	  	  				// ������� ��� - �� ������������ ���� �� ������
}

/*****************************************************************************
 ��������� (������) � ��������� ���������� ������� DO

*****************************************************************************/
void MODBUS_K15(void)
{
unsigned int n;
unsigned char b,d,c;
asm("wdr");
d = SubFunc + Data; // ������ ���� ����� ������ ��� ������

if(d > maxAddr_DO) {m_coderr=2; return;} // ���� ������������� ������ ���������

n = 0;
// ������������ ������ ��� ��������� ������� � int ������� �������
if(_UART_RX_dup[6] == 2) {n = (_UART_RX_dup[8] << 8) | _UART_RX_dup[7];}
else if(_UART_RX_dup[6] == 1) {n = _UART_RX_dup[7];}
else {m_coderr=2; return;} // �� ����� ������ ���-�� ���� ��� ������

for(b = SubFunc,c = 0; b < d; b++) // ��������� ������� ����� � �������� R_DOUT
  {
  if(((n >> c) & 0x01) != 0) OutControl(b,1); // ���� ����������� ��� �� = 0 - ��������� ��������������� ��� � ��������
  else OutControl(b,0); 	   					// ���� = 0 - ������� ��������������� ��� � ��������
  c++;
  }
// *************** ������ �������� ����� **********************************
b = 0;
 _UART_TX_Buf[b++] = Address_device & 0xFF;		// ������� ����� �����������
 _UART_TX_Buf[b++] = Func; 						// ������� ���������� �������
 _UART_TX_Buf[b++] = _UART_RX_dup[2];			// ������� ���������� ����� ������ ��. ����
 _UART_TX_Buf[b++] = _UART_RX_dup[3]; 	   		// ������� ���������� ����� ������ ��. ����
 _UART_TX_Buf[b++] = _UART_RX_dup[4];			// ������� ��������� ������ ���-�� ������� ��. ����
 _UART_TX_Buf[b++] = _UART_RX_dup[5]; 	   		// ������� ��������� ������ ���-�� ������� ��. ����
// ---- ������� CRC ----
 n = _CRC_calc(_UART_TX_Buf,b);				// ������� CRC
 _UART_TX_Buf[b++] = n & 0xFF;	  			// ����� ������� ���� - ������� �������� ������� ����
 _UART_TX_Buf[b++] = ((n >> 8) & 0xFF);		// ����� ������� ���� 

 _UART_TX_length = b;	  	  				// ������� ��� - �� ������������ ���� �� ������
}

/*****************************************************************************
 ��������� (������) � ��������� Holding ��������� (������)
******************************************************************************
*****************************************************************************/
void MODBUS_K16(void)
{
// ���������� ����������
unsigned int n;
unsigned char b,d,c;

asm("wdr");

d = SubFunc + Data; // ������ ���� ����� ������������� ��������
if(d > maxAddrRegHOLD) {m_coderr=2; return;} // ���� ������������� ��������� �������� ������ ���������

// --- ������� ������ � ��������  ---
for(b = SubFunc, c = 0; b < d; b++) // ���������
 {
  MB_Hreg[b] = (_UART_RX_dup[7+(c*2)] << 8) | _UART_RX_dup[8+(c*2)]; // ��� ������ � Holding
  // --- ��� ����������� ��01-3200.� ---
  #ifdef _KM3200_
  if(b == 6) wr_mcnt();	// ���� ��� ������������ ��������� - �������� ����� �������� � EEPROM
  else if(b == 7) wr_vcnt1();	// ���� ��� �������� �������� ��� �������� �1 - �������� ����� �������� � EEPROM
  else if(b == 8) wr_vcnt2();	// ���� ��� �������� �������� ��� �������� �2 - �������� ����� �������� � EEPROM
  else if(b == 9) wr_vcnt3();	// ���� ��� �������� �������� ��� �������� �3 - �������� ����� �������� � EEPROM
  else if(b == 10) wr_vhcnt1();	// ���� ��� ������� ������� ����� ��� �������� �1 - �������� ����� �������� � EEPROM
  else if(b == 11) wr_vlcnt1();	// ���� ��� ������� ������� ����� ��� �������� �1 - �������� ����� �������� � EEPROM
  else if(b == 12) wr_vhcnt2();	// ���� ��� ������� ������� ����� ��� �������� �2 - �������� ����� �������� � EEPROM
  else if(b == 13) wr_vlcnt2();	// ���� ��� ������� ������� ����� ��� �������� �2 - �������� ����� �������� � EEPROM
  else if(b == 14) wr_vhcnt3();	// ���� ��� ������� ������� ����� ��� �������� �3 - �������� ����� �������� � EEPROM
  else if(b == 15) wr_vlcnt3();	// ���� ��� ������� ������� ����� ��� �������� �3 - �������� ����� �������� � EEPROM
  else if(b == 29) wr_adr();	// ���� ��� ����� ����������� - �������� ����� �������� � EEPROM
  else if(b == 30) wr_par();	// ���� ��� ��������� ���� - �������� ����� �������� � EEPROM
  #endif
  c++;
 }
 // ----------------- ���������� ����� ---------------------------------------------
b = 0;
 _UART_TX_Buf[b++] = Address_device & 0xFF;   // ������� ����� �����������
 _UART_TX_Buf[b++] = Func; 		  // ������� ���������� �������
 _UART_TX_Buf[b++] = _UART_RX_dup[2];		  // ������� ��. ���� ���������� ������ ��������
 _UART_TX_Buf[b++] = _UART_RX_dup[3];		  // ������� ��. ���� ���������� ������ ��������
 _UART_TX_Buf[b++] = _UART_RX_dup[4];		  // ������� ��. ���� ���-�� ��������� ���������
 _UART_TX_Buf[b++] = _UART_RX_dup[5];		  // ������� ��. ���� ���-�� ��������� ���������
// --- ������� CRC ---
 n = _CRC_calc(_UART_TX_Buf,b);				// ������� CRC
 _UART_TX_Buf[b++] = n & 0xFF;	  		  // ����� ������� ���� - ������� �������� ������� ����
 _UART_TX_Buf[b++] = ((n >> 8) & 0xFF);  // ����� ������� ���� - ����� ������� ����

_UART_TX_length = b;	  	  				// ������� ��� - �� ������������ ���� �� ������
}

/**************************************************************************
�-� ������������ �������� ����� � ��������� ������ 
��� ������ �� ����������� ������

���� ������:
			00 - ��� ������
			01 - ��� ������� �� ��������������
			02 - �������� ����� ��������
			03 - ������������ �������� ������ ��� ������ � �������
			04 - ���� ����������
			05 - ������������� ������� (������ ������ � �����������,
			     ��� ���������� ��������� ����� )
			06 - ���������� ������ ���������� ��������
			08 - ������ �������� ��� ��������� � �������� ???
� ����������� ������� ��������� ������������:
����� ������ ����� �� ����� ��������� Holding, � ������� �� ������� �������� �������
**************************************************************************/

void MODBUS_RTU(void)
{
	/*
	buf[0]			- ����� ����������
	buf[1]			- ��� �������
	buf[2]+buf[3]		- ��� ���������� (����� ��������)
	buf[4]+buf[5]		- ������ (��� ������� 0x05, 0x06 ������������ ������, ��� 0x01-0x04 ���������� ����������� ������)
	buf[6]+buf[7]		- CRC ������� (����������� ����� ��������� Modbus RTU)
	*/
unsigned int q;
asm("wdr");
// --------------- ������� ����������� ������� ������� �������� -----------------------------------------------
m_coderr = 0;											// ��� ������ ���������� = 0
Func = _UART_RX_dup[1];									// ������� ����������� ��������
SubFunc = (_UART_RX_dup[2]<<8) | _UART_RX_dup[3];		// ������� ��������� ����� ������������ ���� �� �������
Data = (_UART_RX_dup[4]<<8) | _UART_RX_dup[5];	// ������� ���-�� ������������� �������
_UART_TX_length = 0; // ������ ���-�� ���� ��� ��������

if((Func == 5) || (Func == 6)) Data = 1; // --- ���� ������� 5 ��� 6 �� �� ��������� - ���������� �������� = 1

// ---------------------------- �������� ������� ---------------------------------------------------------------
//------------�������� ���������� �� ������������ ---------------------
if (Data==0)	//��������� 0 ������� - ��� ������ ������ ����� 5 � 6
 {
 m_coderr=3; // ��������� ��� ������ - �������� ����� ��������
 }/*---------------------------- k1 -----------------------------------*/	
else if (Func == 1)//������ ����������� ������ DO[1...16]
 {
  MODBUS_K1_2();
 }	
/*---------------------------- k2 -----------------------------------*/	
else if (Func == 2)//������ ����������� ����� DI[1...16]
 {// 
 MODBUS_K1_2();  
 }	
/*---------------------------- k3 -----------------------------------*/	
else if (Func == 3)//������ ���������� ��������� Holding[1...16]
 {// 
 MODBUS_K3_4();  
 }
/*---------------------------- k4 -----------------------------------*/	
else if (Func == 4)//������ ���������� ��������� Input [1...16]
 {// 
 MODBUS_K3_4();  
 }
/*---------------------------- k5 -----------------------------------*/		
else if (Func == 5)//��������� (������) ����������� ������ DO[1...16]
 {
 MODBUS_K5();				  
 }
/*---------------------------- k6 -----------------------------------*/		
else if (Func == 6)//��������� (������) ������ �������� Holding
 {
 MODBUS_K6();				  
 }
/*---------------------------- k15 -----------------------------------*/		
else if (Func == 15)//��������� (������) ���������� ���������� ������� DO[1...16]
 {
 MODBUS_K15();				  
 }
/*---------------------------- k16 -----------------------------------*/	
else if (Func == 16)//��������� (������) ���������� ��������� Holding[1...16]
 {
 MODBUS_K16();				 
 }
else m_coderr=1; // �� ��������� ������ ������� ��� ������� ����������� (�� ������� �� ���� �������)

/* ------ ����� ���� ��������� ����� ��� ������ ���� �� ��� �� ��������� ------- */
if(m_coderr == 0)   // --- ���� ������ ��� ---
 {
 if(_UART_TX_length != 0)  _UART_Go(_UART_TX_length); // � ���� ����� ��� �������� �� - �������������� ���� ��������
 }
else		 	   // --- ������ - ���� �������� ����� ������ ---
 {
  _UART_TX_Buf[0] = Address_device & 0xFF;	// ������� ����� �����������
  _UART_TX_Buf[1] = (Func | 0x80); 			// ������� ���������� ������� � ���. ������ ������
  _UART_TX_Buf[2] = m_coderr;	  			// ����� ��� ������
  // -------------- ��� - �� ����� ��� ��� �������� CRC --------------------
  q = _CRC_calc(_UART_TX_Buf,3);		// ������� CRC
  _UART_TX_Buf[3] = q & 0xFF;	  		// ����� ������� ���� - ������� �������� ������� ����
  _UART_TX_Buf[4] = ((q >> 8) & 0xFF);	// ����� ������� ���� - ����� ������� ����
  _UART_TX_length = 5;	  	  			// ������� ��� - �� ������������ ���� �� ������ NetTxData
  _UART_Go(_UART_TX_length);			// �������������� ���� ��������
 }
 
}

#endif
// *********************************************************** END ***************************************************************************************