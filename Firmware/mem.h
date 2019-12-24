/************************************************************************************************
* ���������� ��� ������ � ������� EEPROM � FLASH ������������ KM01-3200.M0
*
*************************************************************************************************/
#ifndef _MEM_	
#define _MEM_

// --- ������������ ���������� ---
#include <avr/io.h>	
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "km_3200.h"

// --- �������� ��������� ��� ������ ---
typedef struct
{
	unsigned char Address_device;	// ����� ������ � ���� ModBus
	unsigned char COM_Speed;		// ��������� �������� � ����
	unsigned char COM_Param;		// ��������� ����
	unsigned char INP_Mode;			// ����� ������ �������� ������
	unsigned char CB_cnt1;			// �������� �������������� �������� ��� ����� �1
	unsigned char CB_cnt2;			// �������� �������������� �������� ��� ����� �2
	unsigned char CB_cnt3;			// �������� �������������� �������� ��� ����� �3
	unsigned int HW_cnt1;			// ������� ����� ���������� �������� ��� �������� ����� �1
	unsigned int LW_cnt1;			// ������� ����� ���������� �������� ��� �������� ����� �1
	unsigned int HW_cnt2;			// ������� ����� ���������� �������� ��� �������� ����� �2
	unsigned int LW_cnt2;			// ������� ����� ���������� �������� ��� �������� ����� �2
	unsigned int HW_cnt3;			// ������� ����� ���������� �������� ��� �������� ����� �3
	unsigned int LW_cnt3;			// ������� ����� ���������� �������� ��� �������� ����� �3
	
} config;

// --------- ���������� ������� ��������� � EEPROM -------------------------------------------
config MEM EEMEM;	// �������� �������� - ������� ��������� � EEPROM

// ------- ������� ����������� �� ��������� (��������� � EEPROM) -----------------------------
config MEM_df EEMEM = 
{
	0xEF,					// ����� ������ � ���� ModBus (239)
	0x07,					// ��������� �������� � ���� (19200)
	0x00,					// ��������� ���� (8E1)	
	0x00,					// �������� ����� � ������� ������ (�� ������� �����)
	0x01,					// �������� �������� ����������� �� 1 ��� ����� �1
	0x01,					// �������� �������� ����������� �� 1 ��� ����� �2
	0x01,					// �������� �������� ����������� �� 1 ��� ����� �3
	0x0000,					// �� ������� ����� = 0
	0x0000,					// �� ������� ����� = 0
	0x0000,					// �� ������� ����� = 0
	0x0000,					// �� ������� ����� = 0
	0x0000,					// �� ������� ����� = 0
	0x0000					// �� ������� ����� = 0	
};

/**************** ��������� ��������� �������� (�������� �� ���������) ********************************************
* ���������� ��� ������� ���������� �  � �������������� � eeprom, ����� �� eeprom � �������� ���
*******************************************************************************************************************/
void set_default(void)
{
config tma;		// ��������� ������!
	// --- ������� ������ �������� �� ��������� ---
	if(eeprom_is_ready() == 0) eeprom_busy_wait();	// ����� ���������� eeprom
	WDR();
	eeprom_read_block(&tma,&MEM_df,sizeof(MEM_df));	// ������ �c� �� EEPROM � ���
	WDR();
	if(eeprom_is_ready() == 0) eeprom_busy_wait();	// ����� ���������� eeprom
	WDR();
	eeprom_write_block(&tma,&MEM,sizeof(tma));		// �������� ��� ��������� ��� � EEPROM ��������� ������� ��������
	
}

//================================================ ������ � INPUT ���������� ===============================================================

/************ ������ ����� ����������� �� FLASH � Ireg ������� ****************************************************/
void rd_name(void)
{
for(unsigned char i=0;i<6;i++)
	{
		MB_Ireg[28+i] = (unsigned int) (pgm_read_byte(&fl_name[i*2]) << 8);
		MB_Ireg[28+i] |= pgm_read_byte(&fl_name[(i*2)+1]);
	}
}
/************ ������ ������ �� ����������� �� FLASH � Ireg ������� ***********************************************/
void rd_ver(void)
{
for(unsigned char i=0;i<3;i++)
  {
	MB_Ireg[36+i] = (unsigned int) (pgm_read_byte(&fl_version[i*2]) << 8);
	MB_Ireg[36+i] |= pgm_read_byte(&fl_version[(i*2)+1]);
  }
}
/************ ������ ��������� ������ ����������� �� FLASH � Ireg ������� ****************************************/
void rd_ser(void)
{
for(unsigned char i=0;i<3;i++)
  {
	MB_Ireg[39+i] = (unsigned int) (pgm_read_byte(&fl_progid[i*2]) << 8);
	MB_Ireg[39+i] |= pgm_read_byte(&fl_progid[(i*2)+1]);
  }
}
//================================================ ������ � HOLDING ���������� ===============================================================
//============================================================================================================================================

/************ ������ ������ ����������� � ModBus ���� �� EEPROM � Hreg ������� ***********************************/
void rd_adr(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();						// ����� ���������� eeprom
	WDR();
	MB_Hreg[29] = (unsigned int) eeprom_read_byte(&MEM.Address_device); // ������ ������� ����� ����������� �� EEPROM � ���			
}
/*** ������ ��������� �������� � ���������� � ���� ��� �����������, �� EEPROM � Hreg ������� *********************/
void rd_par(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();				// ����� ���������� eeprom
	WDR();
	MB_Hreg[30] = eeprom_read_byte(&MEM.COM_Param) << 8;		// ������ ������� ��������� ���� ��� ����������� � ����� �.�. ��� ������� ����
	WDR();
	MB_Hreg[30] |= eeprom_read_byte(&MEM.COM_Speed);			// ������ ��������� �������� ��� ����������� � �������� �� �������.
}

//=====================================================================================================================================
//======================================== ������ � �������� ������� - ������ =========================================================

/****** ������ �������� ������ ������ �������� ������ ����������� �� EEPROM � Hreg ������� ********************/
void rd_mcnt(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();				// ����� ���������� eeprom
	WDR();
	MB_Hreg[6] = (unsigned int) eeprom_read_byte(&MEM.INP_Mode); // ������ �������� ������� ������ �� EEPROM � ���
}
/****** ������ �������� �������� ��� ����� �1 ����������� �� EEPROM � Hreg ������� *****************************/
void rd_vcnt1(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();				// ����� ���������� eeprom
	WDR();
	MB_Hreg[7] = (unsigned int) eeprom_read_byte(&MEM.CB_cnt1); // ������ �������� �������� �� EEPROM � ���
}
/****** ������ �������� ����� �������� �������� ��� ����� �1 ����������� �� EEPROM � Hreg ������� *************/
void rd_vlcnt1(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();			// ����� ���������� eeprom
	WDR();
	MB_Hreg[11] = eeprom_read_word(&MEM.LW_cnt1);			// ������ �� ����� �������� ����� �������� �� EEPROM � ���
}
/****** ������ �������� ����� �������� �������� ��� ����� �1 ����������� �� EEPROM � Hreg ������� *************/
void rd_vhcnt1(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();			// ����� ���������� eeprom
	WDR();
	MB_Hreg[10] = eeprom_read_word(&MEM.HW_cnt1);			// ������ �� ����� �������� ����� �������� �� EEPROM � ���
}

/****** ������ �������� �������� ��� ����� �2 ����������� �� EEPROM � Hreg ������� *****************************/
void rd_vcnt2(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();				// ����� ���������� eeprom
	WDR();
	MB_Hreg[8] = (unsigned int) eeprom_read_byte(&MEM.CB_cnt2); // ������ �������� �������� �� EEPROM � ���
}
/****** ������ �������� ����� �������� �������� ��� ����� �2 ����������� �� EEPROM � Hreg ������� *************/
void rd_vlcnt2(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();			// ����� ���������� eeprom
	WDR();
	MB_Hreg[13] = eeprom_read_word(&MEM.LW_cnt2);			// ������ �� ����� �������� ����� �������� �� EEPROM � ���
}
/****** ������ �������� ����� �������� �������� ��� ����� �2 ����������� �� EEPROM � Hreg ������� *************/
void rd_vhcnt2(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();			// ����� ���������� eeprom
	WDR();
	MB_Hreg[12] = eeprom_read_word(&MEM.HW_cnt2);			// ������ �� ����� �������� ����� �������� �� EEPROM � ���
}

/****** ������ �������� �������� ��� ����� �3 ����������� �� EEPROM � Hreg ������� *****************************/
void rd_vcnt3(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();				// ����� ���������� eeprom
	WDR();
	MB_Hreg[9] = (unsigned int) eeprom_read_byte(&MEM.CB_cnt3); // ������ �������� �������� �� EEPROM � ���
}
/****** ������ �������� ����� �������� �������� ��� ����� �3 ����������� �� EEPROM � Hreg ������� *************/
void rd_vlcnt3(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();			// ����� ���������� eeprom
	WDR();
	MB_Hreg[15] = eeprom_read_word(&MEM.LW_cnt3);			// ������ �� ����� �������� ����� �������� �� EEPROM � ���
}
/****** ������ �������� ����� �������� �������� ��� ����� �3 ����������� �� EEPROM � Hreg ������� *************/
void rd_vhcnt3(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();			// ����� ���������� eeprom
	WDR();
	MB_Hreg[14] = eeprom_read_word(&MEM.HW_cnt3);			// ������ �� ����� �������� ����� �������� �� EEPROM � ���
}
//====================================== ����� ������ � �������� ������� ==============================================================
//=====================================================================================================================================

//------------------------------------------------------------------------------------------------------------------------- ������ ---
/******************* ������ ������ ����������� � ���� ModBus �� Hreg � EEPROM ***********************************/
void wr_adr(void)
{
unsigned char b;
	b = (unsigned char) MB_Hreg[29];				// ���� ������� ����
	if(eeprom_is_ready() == 0) eeprom_busy_wait();	// ����� ���������� eeprom
	WDR();
	eeprom_write_byte(&MEM.Address_device,b);		// �������� �������� � ������ EEPROM
}
/******************* ������ ���������� ���� ����������� � ModBus �� Hreg � EEPROM ******************************/
void wr_par(void)
{
	unsigned char b;
	b = (unsigned char) MB_Hreg[30];				// ���� ������� ����
	if(eeprom_is_ready() == 0) eeprom_busy_wait();	// ����� ���������� eeprom
	WDR();
	eeprom_write_byte(&MEM.COM_Speed,b);			// �������� �������� �������� � ������ EEPROM
	WDR();
	if(eeprom_is_ready() == 0) eeprom_busy_wait();	// ����� ���������� eeprom
	WDR();
	b = (unsigned char) (MB_Hreg[30] >> 8);			// ���� ������� ����
	if(eeprom_is_ready() == 0) eeprom_busy_wait();	// ����� ���������� eeprom
	WDR();
	eeprom_write_byte(&MEM.COM_Param,b);			// �������� �������� ���������� � ������ EEPROM
}

//=========================================== ������ � �������� ������� - ������ ================================================
//===============================================================================================================================
/******************* ������ ������ ������ ������ ����������� �� Hreg � EEPROM ******************************/
void wr_mcnt(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();					// ����� ���������� eeprom
	WDR();
	eeprom_write_byte(&MEM.INP_Mode,(unsigned char) MB_Hreg[6]);	// �������� �������� � ������ EEPROM
}

/********** ������ ����� ��������� ��� �������� ����� �1 ����������� �� Hreg � EEPROM *********************/
void wr_vcnt1(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();					// ����� ���������� eeprom
	WDR();
	eeprom_write_byte(&MEM.CB_cnt1,(unsigned char) MB_Hreg[7]);	// �������� �������� � ������ EEPROM
}
/*** ������ �������� �������� �������� ����� ��� �������� �1 ����������� �� Hreg � EEPROM ******************/
void wr_vlcnt1(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();	// ����� ���������� eeprom
	WDR();
	eeprom_write_word(&MEM.LW_cnt1,MB_Hreg[11]);	// �������� �������� � ������ EEPROM
}
/*** ������ �������� �������� �������� ����� ��� �������� �1 ����������� �� Hreg � EEPROM ******************/
void wr_vhcnt1(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();	// ����� ���������� eeprom
	WDR();
	eeprom_write_word(&MEM.HW_cnt1,MB_Hreg[10]);	// �������� �������� � ������ EEPROM
}

/********** ������ ����� ��������� ��� �������� ����� �2 ����������� �� Hreg � EEPROM *********************/
void wr_vcnt2(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();					// ����� ���������� eeprom
	WDR();
	eeprom_write_byte(&MEM.CB_cnt2,(unsigned char) MB_Hreg[8]);	// �������� �������� � ������ EEPROM
}
/*** ������ �������� �������� �������� ����� ��� �������� �2 ����������� �� Hreg � EEPROM ******************/
void wr_vlcnt2(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();	// ����� ���������� eeprom
	WDR();
	eeprom_write_word(&MEM.LW_cnt2,MB_Hreg[13]);	// �������� �������� � ������ EEPROM
}
/*** ������ �������� �������� �������� ����� ��� �������� �2 ����������� �� Hreg � EEPROM ******************/
void wr_vhcnt2(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();	// ����� ���������� eeprom
	WDR();
	eeprom_write_word(&MEM.HW_cnt2,MB_Hreg[12]);	// �������� �������� � ������ EEPROM
}
/********** ������ ����� ��������� ��� �������� ����� �3 ����������� �� Hreg � EEPROM *********************/
void wr_vcnt3(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();					// ����� ���������� eeprom
	WDR();
	eeprom_write_byte(&MEM.CB_cnt3,(unsigned char) MB_Hreg[9]);	// �������� �������� � ������ EEPROM
}
/*** ������ �������� �������� �������� ����� ��� �������� �3 ����������� �� Hreg � EEPROM ******************/
void wr_vlcnt3(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();	// ����� ���������� eeprom
	WDR();
	eeprom_write_word(&MEM.LW_cnt3,MB_Hreg[15]);	// �������� �������� � ������ EEPROM
}
/*** ������ �������� �������� �������� ����� ��� �������� �3 ����������� �� Hreg � EEPROM ******************/
void wr_vhcnt3(void)
{
	if(eeprom_is_ready() == 0) eeprom_busy_wait();	// ����� ���������� eeprom
	WDR();
	eeprom_write_word(&MEM.HW_cnt3,MB_Hreg[14]);	// �������� �������� � ������ EEPROM
}

/*************************************** ������������� ��������� �� ������ ********************************/
void InitMem(void)
{
	// �������� ��������� ��������� INPUT
	rd_name();		// ��� �����������
	rd_ver();		// ������ �� �����������
	rd_ser();		// �������� ����� �����������
	// �������� ��������� ��������� HOLD
	rd_adr();		// ����� ����������� � ���� ModBus
	rd_par();		// ��������� ����
	rd_mcnt();		// ��������� ������ ������ �������� ������
	rd_vcnt1();		// �������� �������� ��� ����� �1
	rd_vcnt2();		// �������� �������� ��� ����� �2
	rd_vcnt3();		// �������� �������� ��� ����� �3
	rd_vhcnt1();	// ������� ����� �������� �������� �������� ����� �1
	rd_vlcnt1();	// ������� ����� �������� �������� �������� ����� �1
	rd_vhcnt2();	// ������� ����� �������� �������� �������� ����� �2
	rd_vlcnt2();	// ������� ����� �������� �������� �������� ����� �2
	rd_vhcnt3();	// ������� ����� �������� �������� �������� ����� �3
	rd_vlcnt3();	// ������� ����� �������� �������� �������� ����� �3
}

#endif
//***************************************************** END ************************************************