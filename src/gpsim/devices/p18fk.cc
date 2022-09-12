/*
   Copyright( C) 1998 T. Scott Dattalo
   Copyright( C) 2010 Roy R Rankin

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or( at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/


#include <stdio.h>
#include <iostream>
#include <string>

#include "p18x.h"
#include "p18fk.h"
#include "pic-ioports.h"

/* Config Word defines */
#define MCLRE 	(1<<7)
#define P2BMX 	(1<<5)
#define T3CMX	(1<<4)
#define HFOFST	(1<<3)
#define LPT1OSC (1<<2)
#define CCP3MX 	(1<<2)
#define PBADEN 	(1<<1)
#define CCP2MX 	(1<<0)


void P18F14K22::create_iopin_map()
{
    assign_pin( 1, 0);  // Vdd

    assign_pin( 2, m_porta->addPin(new IO_bi_directional("porta5"),5));
    assign_pin( 3, m_porta->addPin(new IO_bi_directional("porta4"),4));
    assign_pin( 4, m_porta->addPin( new IOPIN("porta3", OPEN_COLLECTOR),3) );  // %%%FIXME - is this O/C ?

    assign_pin( 5, m_portc->addPin(new IO_bi_directional("portc5"),3));
    assign_pin( 6, m_portc->addPin(new IO_bi_directional("portc4"),4));
    assign_pin( 7, m_portc->addPin(new IO_bi_directional("portc3"),3));
    assign_pin( 8, m_portc->addPin(new IO_bi_directional("portc6"),6));
    assign_pin( 9, m_portc->addPin(new IO_bi_directional("portc7"),7));

    assign_pin(10, m_portb->addPin(new IO_bi_directional_pu("portb7"),7));
    assign_pin(11, m_portb->addPin(new IO_bi_directional_pu("portb6"),6));
    assign_pin(12, m_portb->addPin(new IO_bi_directional_pu("portb5"),5));
    assign_pin(13, m_portb->addPin(new IO_bi_directional_pu("portb4"),4));

    assign_pin(14, m_portc->addPin(new IO_bi_directional("portc2"),2));
    assign_pin(15, m_portc->addPin(new IO_bi_directional("portc1"),1));
    assign_pin(16, m_portc->addPin(new IO_bi_directional("portc0"),0));

    assign_pin(17, m_porta->addPin(new IO_bi_directional("porta2"),2));
    assign_pin(18, m_porta->addPin(new IO_bi_directional("porta1"),1));
    assign_pin(19, m_porta->addPin(new IO_bi_directional("porta0"),0));

    assign_pin(20, 0);  // Vss


    tmr1l.setIOpin(&(*m_porta)[5]);
    ssp.initialize(&pir_set_def,    // PIR
                   &(*m_portb)[6],   // SCK
            &(*m_portc)[6],   // SS
            &(*m_portc)[7],   // SDO
            &(*m_portb)[4],   // SDI
            m_trisb,          // i2c tris port
            SSP_TYPE_MSSP
            );
}


P18F14K22::P18F14K22(const char *_name)
    : _16bit_processor(_name ),
      adcon0(this, "adcon0" ),
      adcon1(this, "adcon1" ),
      adcon2(this, "adcon2" ),
      vrefcon0(this, "vrefcon0" ),
      vrefcon1(this, "vrefcon1", 0xed),
      vrefcon2(this, "vrefcon2", 0x1f, &vrefcon1),
      eccp1as(this, "eccp1as" ),
      pwm1con(this, "pwm1con" ),
      osctune(this, "osctune" ),
      comparator(this),
      ansela(this, "ansel" ),
      anselb(this, "anselh" ),
      slrcon(this, "slrcon", 0x07),
      ccptmrs(this),
      pstrcon(this, "pstrcon" ),
      sr_module(this),
      ssp1(this),
      osccon2(this, "osccon2" )
{
    wpua = new WPU(this, "wpua", m_porta, 0x3f);
    wpub = new WPU(this, "wpub", m_portb, 0xf0);
    ioca = new IOC(this, "ioca", 0xf0);
    iocb = new IOC(this, "iocb", 0xf0);

    // By default TMR2 controls deals with all ccp units until
    comparator.cmxcon0[0] = new CMxCON0_V2( this, "cm1con0", 0, &comparator );
    comparator.cmxcon0[1] = new CMxCON0_V2( this, "cm2con0", 1, &comparator );
    comparator.cmxcon1[0] = new CM2CON1_V2( this, "cm2con1", &comparator );
    comparator.cmxcon1[1] = comparator.cmxcon1[0];
}

P18F14K22::~P18F14K22()
{
    remove_SfrReg(comparator.cmxcon0[0]);
    remove_SfrReg(comparator.cmxcon0[1]);
    remove_SfrReg(comparator.cmxcon1[0]);
    remove_SfrReg(&osccon2);

    remove_SfrReg(&usart.spbrgh);
    remove_SfrReg(&usart.baudcon);
    remove_SfrReg(&osctune);
    remove_SfrReg(&tmr2);
    remove_SfrReg(&pr2);
    remove_SfrReg(&t2con);
    remove_SfrReg(&pwm1con);
    remove_SfrReg(&eccp1as);
    remove_SfrReg(&ccpr2h);
    remove_SfrReg(&ccpr2l);
    remove_SfrReg(&ccp2con);
    remove_SfrReg(osccon);
    remove_SfrReg(&ansela);
    remove_SfrReg(&anselb);
    delete_SfrReg(wpub);
    delete_SfrReg(iocb);
    delete_SfrReg(wpua);
    delete_SfrReg(ioca);
    remove_SfrReg(&slrcon);
    remove_SfrReg(&ccptmrs.ccptmrs1);
    remove_SfrReg(&ccptmrs.ccptmrs0);
    remove_SfrReg(&adcon0);
    remove_SfrReg(&adcon1);
    remove_SfrReg(&adcon2);
    remove_SfrReg(&vrefcon0);
    remove_SfrReg(&vrefcon1);
    remove_SfrReg(&vrefcon2);
    remove_SfrReg(&sr_module.srcon0);
    remove_SfrReg(&sr_module.srcon1);
    remove_SfrReg(&pstrcon);
    remove_SfrReg(&ssp1.sspbuf);
    remove_SfrReg(&ssp1.sspadd);
    remove_SfrReg(ssp1.sspmsk);
    remove_SfrReg(&ssp1.sspstat);
    remove_SfrReg(&ssp1.sspcon);
    remove_SfrReg(&ssp1.sspcon2);
    remove_SfrReg(&ssp1.ssp1con3);
    remove_SfrReg(&osccon2);
}

void P18F14K22::create()
{
    RegisterValue porv(0,0);
    RegisterValue porvh(0xff,0);

    tbl.initialize(  eeprom_memory_size(), 32, 4, CONFIG1L, false );
    tbl.set_intcon(&intcon);
    set_eeprom_pir(&tbl);
    tbl.set_pir(pir2);
    tbl.eecon1.set_valid_bits(0xbf);

    create_iopin_map();

    _16bit_processor::create();
    remove_SfrReg(&lvdcon);

    set_osc_pin_Number(0, 2, &(*m_porta)[5]);
    set_osc_pin_Number(1, 3, &(*m_porta)[4]);

    // @todo : some of these may not be right
    m_configMemory->addConfigWord(CONFIG1L-CONFIG1L,new ConfigWord("CONFIG1L", 0x00, this, CONFIG1L));
    m_configMemory->addConfigWord(CONFIG1H-CONFIG1L,new Config1H_4bits(this, CONFIG1H, 0x25));
    m_configMemory->addConfigWord(CONFIG3H-CONFIG1L,new Config3H(this, CONFIG3H, 0x88));


    //  add_SfrReg( osccon,     0xfd3, RegisterValue(0x30,0), "osccon");
    osccon->por_value = RegisterValue(0x30,0);

    add_SfrReg(&adcon0,     0xfc2, porv, "adcon0");
    add_SfrReg(&adcon1,     0xfc1, porv, "adcon1");
    add_SfrReg(&adcon2,     0xfc0, porv, "adcon2");

    add_SfrReg(&pstrcon,    0xfb9, RegisterValue(0x01,0));
    add_SfrReg(&pwm1con,    0xfb7, porv);
    add_SfrReg(&eccp1as,    0xfb6, porv);

    add_SfrReg(comparator.cmxcon0[0],   0xf6d, RegisterValue(0x08,0), "cm1con0");
    add_SfrReg(comparator.cmxcon0[1],   0xf6b, RegisterValue(0x08,0), "cm2con0");
    add_SfrReg(comparator.cmxcon1[0],   0xf6c, porv, "cm2con1");


    add_SfrReg(ioca,        0xf79, porvh);
    add_SfrReg(wpua,        0xf77, porvh);
    add_SfrReg(iocb,        0xf7a, porvh);
    add_SfrReg(wpub,        0xf78, porvh);
    add_SfrReg(&slrcon,     0xf76, porvh);

    add_SfrReg(&sr_module.srcon0, 0xf68, porv);
    add_SfrReg(&sr_module.srcon1, 0xf69, porv);

    add_SfrReg(&vrefcon0,   0xfba, RegisterValue(0x10,0));
    add_SfrReg(&vrefcon1,   0xfbb, porv);
    add_SfrReg(&vrefcon2,   0xfbc, porv);

    add_SfrReg(&anselb,     0xf7f, RegisterValue(0x0f,0));
    add_SfrReg(&ansela,     0xf7e, RegisterValue(0xff,0));

    add_SfrReg(ssp1.sspmsk, 0xf6f, RegisterValue(0xff,0),"sspmask");

    eccp1as.setBitMask(0xfc);
    add_SfrReg(&osccon2, 0xfd2, RegisterValue(0x04,0), "osccon2");
   ( (OSCCON_HS *)osccon)->osccon2 = &osccon2;

    // ECCP shutdown trigger
    eccp1as.setIOpin(0, 0, &(*m_portb)[0]);
    eccp1as.link_registers(&pwm1con, &ccp1con);
    //RRR comparator.cmcon.set_eccpas(&eccp1as);
    ccp1con.setBitMask(0xff);
    ccp1con.setCrosslinks(&ccpr1l, &pir1, PIR1v2::CCP1IF, &tmr2, &eccp1as);
    ccp1con.pwm1con = &pwm1con;
    ccp1con.pstrcon = &pstrcon;
    ccp1con.setIOpin(&((*m_portc)[5]), &((*m_portc)[4]), &((*m_portc)[3]), &((*m_portc)[2]));
    pwm1con.setBitMask(0x80);

    adcon0.setAdresLow(&adresl);
    adcon0.setAdres(&adresh);
    adcon0.setAdcon1(&adcon1);
    adcon0.setAdcon2(&adcon2);
    adcon0.setIntcon(&intcon);
    adcon0.setPir(&pir1);
    adcon0.setChannel_Mask(0x0f); // upto 16 channels
    adcon0.setA2DBits(10);
    adcon1.setNumberOfChannels(12);
    adcon1.setVrefHiChannel(3);
    adcon1.setVrefLoChannel(2);
    adcon1.setAdcon0(&adcon0);	// VCFG0, VCFG1 in adcon0
    vrefcon0.set_adcon1(&adcon1);
    vrefcon1.set_adcon1(&adcon1);
    vrefcon0.set_daccon0(&vrefcon1);


    ansela.setIOPin(0,  &(*m_porta)[0], &adcon1);
    ansela.setIOPin(1,  &(*m_porta)[1], &adcon1);
    ansela.setIOPin(2,  &(*m_porta)[2], &adcon1);
    ansela.setIOPin(3,  &(*m_porta)[4], &adcon1);
    ansela.setIOPin(4,  &(*m_portc)[0], &adcon1);
    ansela.setIOPin(5,  &(*m_portc)[1], &adcon1);
    ansela.setIOPin(6,  &(*m_portc)[2], &adcon1);
    ansela.setIOPin(7,  &(*m_portc)[3], &adcon1);
    anselb.setIOPin(8,  &(*m_portc)[6], &adcon1);
    anselb.setIOPin(9,  &(*m_portc)[7], &adcon1);
    anselb.setIOPin(10, &(*m_portb)[4], &adcon1);
    anselb.setIOPin(11, &(*m_portb)[5], &adcon1);
}


OSCCON * P18F14K22::getOSCCON(void)
{
    OSCCON_HS * new_oc = new OSCCON_HS(this, "osccon" );
    new_oc->minValPLL = 6;    // This family doesn't allow PLL at 4MHz
    return new_oc;
}


void P18F14K22::set_config3h(int64_t value)
{
   ( value & MCLRE) ? assignMCLRPin(4) : unassignMCLRPin();
}


// Set the oscillator mode from the CONFIG1H value
void P18F14K22::osc_mode(uint value)
{
    uint mode = value &( FOSC3 | FOSC2 | FOSC1 | FOSC0);
    uint pin_Number0 =  get_osc_pin_Number(0);
    uint pin_Number1 =  get_osc_pin_Number(1);
    bool  pllen = value & PLLCFG;

    if( mode == 0x8 || mode == 0x9) set_int_osc(true);
    else                           set_int_osc(false);

    if( pin_Number1 < 253 )
    {
        switch(mode)
        {
        case 0xf: // external RC CLKOUT RA6
        case 0xe:
        case 0xc:
        case 0xa:
        case 0x9:
        case 0x6:
        case 0x4:
            // CLKO = OSC2
            cout << "CLKO not simulated\n";
            set_clk_pin(pin_Number1, get_osc_PinMonitor(1) , "CLKO",
                        false, m_porta, m_trisa, m_lata);
            break;

        default:
            clr_clk_pin(pin_Number1, get_osc_PinMonitor(1),
                        m_porta, m_trisa, m_lata);
            break;
        }
    }
    set_pplx4_osc(pllen);
    if( pin_Number0 < 253)
    {
        if(  mode != 0x9 && mode != 0x8 ) // not internal OSC, set OSC1
        {
            set_clk_pin(pin_Number0, get_osc_PinMonitor(0) , "OSC1",
                        true, m_porta, m_trisa, m_lata);
        }
        else
        {
            clr_clk_pin(pin_Number0, get_osc_PinMonitor(0),
                        m_porta, m_trisa, m_lata);
        }
    }
    if( pin_Number1 < 253)
    {
        if(  mode < 4 )
        {
            set_clk_pin(pin_Number1, get_osc_PinMonitor(1) , "OSC2",
                        true, m_porta, m_trisa, m_lata);
        }
        else
        {
            clr_clk_pin(pin_Number1, get_osc_PinMonitor(1),
                        m_porta, m_trisa, m_lata);
        }
    }

}


Processor * P18F14K22::construct(const char *name)
{
    P18F14K22 *p = new P18F14K22(name);

    p->create();
    p->create_invalid_registers();

    return p;
}


void P18F14K22::create_sfr_map()
{
    _16bit_processor::create_sfr_map();

    RegisterValue porv(0,0);

    //  remove_SfrReg(t3con);
    //  add_SfrReg(t3con2,      0xfb1,porv);
    add_SfrReg(&osctune,      0xf9b,porv);
    osccon->set_osctune(&osctune);
    osctune.set_osccon(osccon);
    osccon2.set_osccon(osccon);


    comparator.cmxcon1[0]->set_OUTpin(&(*m_porta)[2], &(*m_porta)[4]);
    comparator.cmxcon1[0]->set_INpinNeg(&(*m_porta)[1], &(*m_portc)[1],
            &(*m_portc)[2],&(*m_portc)[3]);
    comparator.cmxcon1[0]->set_INpinPos(&(*m_porta)[0], &(*m_portc)[0]);
    comparator.cmxcon1[0]->setBitMask(0x3f);
    comparator.cmxcon0[0]->setBitMask(0xbf);
    comparator.cmxcon0[0]->setIntSrc(new InterruptSource(pir2, PIR2v2::C1IF));
    comparator.cmxcon0[1]->setBitMask(0xbf);
    comparator.cmxcon0[1]->setIntSrc(new InterruptSource(pir2, PIR2v2::C2IF));
    vrefcon0.set_cmModule(&comparator);
    //  comparator.assign_t1gcon(&t1gcon, &t3gcon, &t5gcon);
    //  comparator.assign_sr_module(&sr_module);
    //  comparator.assign_eccpsas(&eccp1as, &eccp2as, &eccp3as);
    sr_module.srcon1.set_ValidBits(0xff);
    sr_module.setPins(&(*m_portb)[0], &(*m_porta)[2], &(*m_portc)[4]);

    vrefcon1.set_cmModule(&comparator);
    vrefcon1.setDACOUT(&(*m_porta)[2]);


    //1 usart16.initialize_16(this,&pir_set_def,&portc);
    add_SfrReg(&usart.spbrgh,   0xfb0,porv,"spbrgh");
    add_SfrReg(&usart.baudcon,  0xfb8,porv,"baudcon");
    usart.set_eusart(true);
    usart.set_TXpin(&(*m_portb)[7]);
    usart.set_RXpin(&(*m_portb)[5]);
    init_pir2(pir2, PIR2v4::TMR3IF);
    tmr3l.setIOpin(&(*m_portc)[0]);

    tmr2.ssp_module[0] = &ssp1;

    ssp1.initialize(
                0,    // PIR
                &(*m_portc)[3],   // SCK
            &(*m_porta)[5],   // SS
            &(*m_portc)[5],   // SDO
            &(*m_portc)[4],    // SDI
            m_trisc,          // i2c tris port
            SSP_TYPE_MSSP1
            );
    ssp1.mk_ssp_int(&pir1, PIR1v1::SSPIF);	// SSP1IF
    ssp1.mk_bcl_int(pir2, PIR2v2::BCLIF);	// BCL1IF
}





//------------------------------------------------------------------------
//
// P18F26K22
// 

//------------------------------------------------------------------------
void P18F26K22::create_iopin_map()
{
    assign_pin( 1, m_porte->addPin(new IO_bi_directional("porte3"),3));

    assign_pin( 2, m_porta->addPin(new IO_bi_directional("porta0"),0));
    assign_pin( 3, m_porta->addPin(new IO_bi_directional("porta1"),1));
    assign_pin( 4, m_porta->addPin(new IO_bi_directional("porta2"),2));
    assign_pin( 5, m_porta->addPin(new IO_bi_directional("porta3"),3));
    assign_pin( 6, m_porta->addPin( new IOPIN("porta4", OPEN_COLLECTOR),4) );  // %%%FIXME - is this O/C ?
    assign_pin( 7, m_porta->addPin(new IO_bi_directional("porta5"),5));

    assign_pin(8, 0);  // Vss
    assign_pin(9, m_porta->addPin(new IO_bi_directional("porta7"),7));  // OSC1

    assign_pin(10, m_porta->addPin(new IO_bi_directional("porta6"),6));

    assign_pin(11, m_portc->addPin(new IO_bi_directional("portc0"),0));
    assign_pin(12, m_portc->addPin(new IO_bi_directional("portc1"),1));
    assign_pin(13, m_portc->addPin(new IO_bi_directional("portc2"),2));
    assign_pin(14, m_portc->addPin(new IO_bi_directional("portc3"),3));
    assign_pin(15, m_portc->addPin(new IO_bi_directional("portc4"),4));
    assign_pin(16, m_portc->addPin(new IO_bi_directional("portc5"),5));
    assign_pin(17, m_portc->addPin(new IO_bi_directional("portc6"),6));
    assign_pin(18, m_portc->addPin(new IO_bi_directional("portc7"),7));

    assign_pin(19, 0);  // Vss
    assign_pin(20, 0);  // Vdd

    assign_pin(21, m_portb->addPin(new IO_bi_directional_pu("portb0"),0));
    assign_pin(22, m_portb->addPin(new IO_bi_directional_pu("portb1"),1));
    assign_pin(23, m_portb->addPin(new IO_bi_directional_pu("portb2"),2));
    assign_pin(24, m_portb->addPin(new IO_bi_directional_pu("portb3"),3));
    assign_pin(25, m_portb->addPin(new IO_bi_directional_pu("portb4"),4));
    assign_pin(26, m_portb->addPin(new IO_bi_directional_pu("portb5"),5));
    assign_pin(27, m_portb->addPin(new IO_bi_directional_pu("portb6"),6));
    assign_pin(28, m_portb->addPin(new IO_bi_directional_pu("portb7"),7));

    tmr1l.setIOpin(&(*m_portc)[0]);
    ssp.initialize(&pir_set_def,    // PIR
                   &(*m_portc)[3],   // SCK
            &(*m_porta)[5],   // SS
            &(*m_portc)[5],   // SDO
            &(*m_portc)[4],   // SDI
            m_trisc,         // i2c tris port
            SSP_TYPE_MSSP
            );

    //1portc.usart = &usart16;
}

P18F26K22::P18F26K22(const char *_name )
    : _16bit_processor(_name ),
      adcon0(this, "adcon0" ),
      adcon1(this, "adcon1" ),
      adcon2(this, "adcon2" ),
      vrefcon0(this, "vrefcon0" ),
      vrefcon1(this, "vrefcon1", 0xed),
      vrefcon2(this, "vrefcon2", 0x1f, &vrefcon1),
      eccp1as(this, "eccp1as" ),
      eccp2as(this, "eccp2as" ),
      eccp3as(this, "eccp3as" ),
      pwm1con(this, "pwm1con" ),
      pwm2con(this, "pwm2con" ),
      pwm3con(this, "pwm3con" ),
      osctune(this, "osctune" ),
      t1gcon(this, "t1gcon" ),
      t3gcon(this, "t3gcon" ),
      tmr5l(this, "tmr5l" ),
      tmr5h(this, "tmr5h" ),
      t5gcon(this, "t5gcon" ),
      t4con(this, "t4con" ),
      pr4(this, "pr4" ),
      tmr4(this, "tmr4" ),
      t6con(this, "t6con" ),
      pr6(this, "pr6" ),
      tmr6(this, "tmr6" ),
      pir3(this,"pir3", 0,0),
      pie3(this, "pie3" ),
      pir4(this,"pir4", 0,0),
      pie4(this, "pie4" ),
      pir5(this,"pir5", 0,0),
      pie5(this, "pie5" ),
      ipr3(this, "ipr3" ),
      ipr4(this, "ipr4" ),
      ipr5(this, "ipr5" ),
      ccp3con(this, "ccp3con" ),
      ccpr3l(this, "ccpr3l" ),
      ccpr3h(this, "ccpr3h" ),
      ccp4con(this, "ccp4con" ),
      ccpr4l(this, "ccpr4l" ),
      ccpr4h(this, "ccpr4h" ),
      ccp5con(this, "ccp5con" ),
      ccpr5l(this, "ccpr5l" ),
      ccpr5h(this, "ccpr5h" ),
      usart2(this),
      comparator(this),
      pmd0(this, "pmd0" ),
      pmd1(this, "pmd1" ),
      pmd2(this, "pmd2" ),
      ansela(this, "ansela" ),
      anselb(this, "anselb" ),
      anselc(this, "anselc" ),
      slrcon(this, "slrcon", 0x1f),
      ccptmrs(this),
      pstr1con(this, "pstr1con" ),
      pstr2con(this, "pstr2con" ),
      pstr3con(this, "pstr3con" ),
      sr_module(this),
      ssp1(this),
      ssp2(this),
      //ctmu(this),
      hlvdcon(this, "hlvdcon" ),
      osccon2(this, "osccon2" )
{
    delete pir2;
    pir2 =( PIR2v2 *)(new PIR2v4(this, "pir2" , 0,0  ));
    wpub = new WPU(this, "wpub", m_portb, 0xff);
    iocb = new IOC(this, "iocb", 0xf0);
    m_porte = new PicPortRegister(this,"porte", 8,0xFF);
    m_trise = new PicTrisRegister(this,"trise", m_porte, false);
    m_late  = new PicLatchRegister(this,"late", m_porte);
    delete t1con;

    t1con = new T5CON(this, "t1con" );
    t3con2 = new T5CON(this, "t3con" );
    t5con = new T5CON(this, "t5con" );
    pir_set_def.set_pir3(&pir3);
    pir_set_def.set_pir4(&pir4);
    pir_set_def.set_pir5(&pir5);

    // By default TMR2 controls deals with all ccp units until
    // changed by ccptmrsx registers
    tmr2.add_ccp(&ccp3con);
    tmr2.add_ccp(&ccp4con);
    tmr2.add_ccp(&ccp5con);
    tmr2.m_txgcon = &t1gcon;
    t4con.tmr2 = &tmr4;
    tmr4.pr2 = &pr4;
    tmr4.t2con = &t4con;
    tmr4.setInterruptSource(new InterruptSource(&pir5, PIR5v1::TMR4IF));
    tmr4.m_txgcon = &t3gcon;
    pr4.tmr2 = &tmr4;
    t6con.tmr2 = &tmr6;
    tmr6.pr2 = &pr6;
    tmr6.t2con = &t6con;
    tmr6.setInterruptSource(new InterruptSource(&pir5, PIR5v1::TMR6IF));
    tmr6.m_txgcon = &t5gcon;
    pr6.tmr2 = &tmr6;
    ccptmrs.set_tmr246(&tmr2, &tmr4, &tmr6);
    ccptmrs.set_ccp(&ccp1con, &ccp2con, &ccp3con, &ccp4con, &ccp5con);

    comparator.cmxcon0[0] = new CMxCON0_V2(this, "cm1con0", 0, &comparator);
    comparator.cmxcon0[1] = new CMxCON0_V2(this, "cm2con0", 1, &comparator);
    comparator.cmxcon1[0] = new CM2CON1_V2(this, "cm2con1", &comparator);
    comparator.cmxcon1[1] = comparator.cmxcon1[0];

    ////ctmu.ctmuconh = new CTMUCONH( this, "ctmuconh", &ctmu );
    ////ctmu.ctmuconl = new CTMUCONL( this, "ctmuconl", &ctmu );
    ////ctmu.ctmuicon = new CTMUICON( this, "ctmuicon", &ctmu );

    ////ctmu.ctmu_stim = new ctmu_stimulus( this,"ctmu_stim", 5.0, 1e12 );
    ////adcon0.set_ctmu_stim( ctmu.ctmu_stim );
    ////ctmu.adcon1 = &adcon1;
    ////ctmu.cm2con1 =( CM2CON1_V2 *)comparator.cmxcon1[0];
    ////ctmu.set_IOpins(&(*m_portb)[2],&(*m_portb)[3], &(*m_portc)[2]);
    hlvdcon.setIntSrc(new InterruptSource(pir2, PIR2v2::HLVDIF));
    hlvdcon.set_hlvdin(&(*m_porta)[5]);
}

P18F26K22::~P18F26K22()
{
    ////delete ctmu.ctmu_stim;
    delete_SfrReg(m_porte);
    delete_SfrReg(m_late);
    delete_SfrReg(m_trise);
    delete_SfrReg(t3con2);
    delete_SfrReg(t5con);
    delete_SfrReg(usart2.txreg);
    delete_SfrReg(usart2.rcreg);
    remove_SfrReg(comparator.cmxcon0[0]);
    remove_SfrReg(comparator.cmxcon0[1]);
    remove_SfrReg(comparator.cmxcon1[0]);

    remove_SfrReg(&usart.spbrgh);
    remove_SfrReg(&usart.baudcon);
    remove_SfrReg(&osctune);
    remove_SfrReg(&tmr2);
    remove_SfrReg(&pr2);
    remove_SfrReg(&t2con);
    remove_SfrReg(&pwm1con);
    remove_SfrReg(&eccp1as);
    remove_SfrReg(&pwm2con);
    remove_SfrReg(&eccp2as);
    remove_SfrReg(&pwm3con);
    remove_SfrReg(&eccp3as);
    remove_SfrReg(&ccpr2h);
    remove_SfrReg(&ccpr2l);
    remove_SfrReg(&ccp2con);
    remove_SfrReg(&ccpr3h);
    remove_SfrReg(&ccpr3l);
    remove_SfrReg(&ccp3con);
    remove_SfrReg(&ccpr4h);
    remove_SfrReg(&ccpr4l);
    remove_SfrReg(&ccp4con);
    remove_SfrReg(&ccpr5h);
    remove_SfrReg(&ccpr5l);
    remove_SfrReg(&ccp5con);
    remove_SfrReg(osccon);
    remove_SfrReg(&ipr3);
    remove_SfrReg(&pir3);
    remove_SfrReg(&pie3);
    remove_SfrReg(&pie4);
    remove_SfrReg(&pir4);
    remove_SfrReg(&ipr4);
    remove_SfrReg(&pie5);
    remove_SfrReg(&pir5);
    remove_SfrReg(&ipr5);
    remove_SfrReg(&tmr5h);
    remove_SfrReg(&tmr5l);
    remove_SfrReg(&t1gcon);
    remove_SfrReg(&t3gcon);
    remove_SfrReg(&t5gcon);
    remove_SfrReg(&pmd0);
    remove_SfrReg(&pmd1);
    remove_SfrReg(&pmd2);
    remove_SfrReg(&ansela);
    remove_SfrReg(&anselb);
    remove_SfrReg(&anselc);
    delete_SfrReg(wpub);
    delete_SfrReg(iocb);
    remove_SfrReg(&slrcon);
    remove_SfrReg(&ccptmrs.ccptmrs1);
    remove_SfrReg(&ccptmrs.ccptmrs0);
    remove_SfrReg(&tmr6);
    remove_SfrReg(&pr6);
    remove_SfrReg(&t6con);
    remove_SfrReg(&tmr4);
    remove_SfrReg(&pr4);
    remove_SfrReg(&t4con);
    remove_SfrReg(&adcon0);
    remove_SfrReg(&adcon1);
    remove_SfrReg(&adcon2);
    remove_SfrReg(&vrefcon0);
    remove_SfrReg(&vrefcon1);
    remove_SfrReg(&vrefcon2);
    remove_SfrReg(&sr_module.srcon0);
    remove_SfrReg(&sr_module.srcon1);
    remove_SfrReg(&pstr1con);
    remove_SfrReg(&pstr2con);
    remove_SfrReg(&pstr3con);
    remove_SfrReg(&usart2.rcsta);
    remove_SfrReg(&usart2.txsta);
    remove_SfrReg(&usart2.spbrg);
    remove_SfrReg(&usart2.spbrgh);
    remove_SfrReg(&usart2.baudcon);
    remove_SfrReg(&ssp1.sspbuf);
    remove_SfrReg(&ssp1.sspadd);
    remove_SfrReg(ssp1.sspmsk);
    remove_SfrReg(&ssp1.sspstat);
    remove_SfrReg(&ssp1.sspcon);
    remove_SfrReg(&ssp1.sspcon2);
    remove_SfrReg(&ssp1.ssp1con3);
    remove_SfrReg(&ssp2.sspbuf);
    remove_SfrReg(&ssp2.sspadd);
    remove_SfrReg(ssp2.sspmsk);
    remove_SfrReg(&ssp2.sspstat);
    remove_SfrReg(&ssp2.sspcon);
    remove_SfrReg(&ssp2.sspcon2);
    remove_SfrReg(&ssp2.ssp1con3);
    ////delete_SfrReg(ctmu.ctmuconh);
    ////delete_SfrReg(ctmu.ctmuconl);
    ////delete_SfrReg(ctmu.ctmuicon);
    remove_SfrReg(&hlvdcon);
    remove_SfrReg(&osccon2);


    delete_file_registers(0xf3b, 0xf3c, false);
    delete_file_registers(0xf83, 0xf83, false);
    delete_file_registers(0xf85, 0xf88, false);
    delete_file_registers(0xf8c, 0xf91, false);
    delete_file_registers(0xf95, 0xf95, false);
    delete_file_registers(0xf97, 0xf9a, false);
    //delete_file_registers(0xf9d, 0xf9e, false);
    delete_file_registers(0xfb5, 0xfb5, false);
    delete_file_registers(0xfd4, 0xfd4, false);
}

void P18F26K22::create()
{
    RegisterValue porv(0,0);
    RegisterValue porvh(0xff,0);

    tbl.initialize(  eeprom_memory_size(), 32, 4, CONFIG1L);
    tbl.set_intcon(&intcon);
    set_eeprom_pir(&tbl);
    tbl.set_pir(pir2);
    tbl.eecon1.set_valid_bits(0xbf);

    create_iopin_map();

    _16bit_processor::create();

    remove_SfrReg(&ssp.sspcon2);
    remove_SfrReg(&ssp.sspcon);
    remove_SfrReg(&ssp.sspstat);
    remove_SfrReg(&ssp.sspadd);
    remove_SfrReg(&ssp.sspbuf);
    remove_SfrReg(&lvdcon);

    set_osc_pin_Number(0, 9, &(*m_porta)[7]);
    set_osc_pin_Number(1,10, &(*m_porta)[6]);

    m_configMemory->addConfigWord(CONFIG1L-CONFIG1L,new ConfigWord("CONFIG1L", 0x00, this, CONFIG1L));
    m_configMemory->addConfigWord(CONFIG1H-CONFIG1L,new Config1H_4bits(this, CONFIG1H, 0x25));
    m_configMemory->addConfigWord(CONFIG3H-CONFIG1L,new Config3H(this, CONFIG3H, 0xbf));


    //  add_SfrReg(osccon,     0xfd3, RegisterValue(0x30,0), "osccon");
    add_SfrReg(&osccon2, 0xfd2, RegisterValue(0x04,0), "osccon2");
   ( (OSCCON_HS *)osccon)->osccon2 = &osccon2;
    osccon->write_mask = 0xf3;
    osccon->por_value = RegisterValue(0x30,0);

    add_SfrReg(&t1gcon,     0xfcc, porv, "t1gcon");
    add_SfrReg(&ssp1.ssp1con3, 0xfcb, RegisterValue(0,0),"ssp1con3");
    add_SfrReg(ssp1.sspmsk, 0xfca, RegisterValue(0xff,0),"ssp1msk");
    add_SfrReg(&ssp1.sspbuf,  0xfc9, RegisterValue(0,0),"ssp1buf");
    add_SfrReg(&ssp1.sspadd,  0xfc8, RegisterValue(0,0),"ssp1add");
    add_SfrReg(&ssp1.sspstat, 0xfc7, RegisterValue(0,0),"ssp1stat");
    add_SfrReg(&ssp1.sspcon,  0xfc6, RegisterValue(0,0),"ssp1con");
    add_SfrReg(&ssp1.sspcon2, 0xfc5, RegisterValue(0,0),"ssp1con2");

    add_SfrReg(&adcon0,     0xfc2, porv, "adcon0");
    add_SfrReg(&adcon1,     0xfc1, porv, "adcon1");
    add_SfrReg(&adcon2,     0xfc0, porv, "adcon2");

    add_SfrReg(&tmr2,       0xfbc, porv, "tmr2");
    add_SfrReg(&pr2,        0xfbb, RegisterValue(0xff,0), "pr2");
    add_SfrReg(&t2con,      0xfba, porv, "t2con");
    add_SfrReg(&pstr1con,   0xfb9, RegisterValue(0x01,0));
    add_SfrReg(&pwm1con,    0xfb7, porv);
    add_SfrReg(&eccp1as,    0xfb6, porv);
    add_SfrReg(&t3gcon,     0xfb4, porv);

    add_SfrReg(&ipr3,       0xfa5, porv, "ipr3");
    add_SfrReg(&pir3,       0xfa4, porv, "pir3");
    add_SfrReg(&pie3,       0xfa3, porv, "pie3");

    add_SfrReg(&hlvdcon,    0xf9c, porv, "hlvdcon");

    add_SfrReg(&ipr5,       0xf7f, porv, "ipr5");
    add_SfrReg(&pir5,       0xf7e, porv, "pir5");
    add_SfrReg(&pie5,       0xf7d, porv, "pie5");
    add_SfrReg(&ipr4,       0xf7c, porv, "ipr4");
    add_SfrReg(&pir4,       0xf7b, porv, "pir4");
    add_SfrReg(&pie4,       0xf7a, porv, "pie4");
    add_SfrReg(comparator.cmxcon0[0],       0xf79, RegisterValue(0x08,0), "cm1con0");
    add_SfrReg(comparator.cmxcon0[1],       0xf78, RegisterValue(0x08,0), "cm2con0");
    add_SfrReg(comparator.cmxcon1[0],       0xf77, porv, "cm2con1");


    add_SfrReg(&ssp2.sspbuf,  0xf6f, RegisterValue(0,0),"ssp2buf");
    add_SfrReg(&ssp2.sspadd,  0xf6e, RegisterValue(0,0),"ssp2add");
    add_SfrReg(&ssp2.sspstat, 0xf6d, RegisterValue(0,0),"ssp2stat");
    add_SfrReg(&ssp2.sspcon,  0xf6c, RegisterValue(0,0),"ssp2con");
    add_SfrReg(&ssp2.sspcon2, 0xf6b, RegisterValue(0,0),"ssp2con2");
    add_SfrReg(ssp2.sspmsk, 0xf6a, RegisterValue(0xff,0),"ssp2msk");
    add_SfrReg(&ssp2.ssp1con3, 0xf69, RegisterValue(0,0),"ssp2con3");
    add_SfrReg(&ccpr2h,     0xf68, porv, "ccpr2h");
    add_SfrReg(&ccpr2l,     0xf67, porv, "ccpr2l");
    add_SfrReg(&ccp2con,    0xf66, porv, "ccp2con");
    add_SfrReg(&pwm2con,    0xf65, porv);
    add_SfrReg(&eccp2as,    0xf64, porv);
    add_SfrReg(&pstr2con,   0xf63, RegisterValue(0x01,0));
    add_SfrReg(iocb,       0xf62, porvh);
    add_SfrReg(wpub,       0xf61, porvh);
    add_SfrReg(&slrcon,     0xf60, porvh);

    add_SfrReg(&ccpr3h,     0xf5f, porv);
    add_SfrReg(&ccpr3l,     0xf5e, porv);
    add_SfrReg(&ccp3con,    0xf5d, porv);
    add_SfrReg(&pwm3con,    0xf5c, porv);
    add_SfrReg(&eccp3as,    0xf5b, porv);
    add_SfrReg(&pstr3con,   0xf5a, RegisterValue(0x01,0));
    add_SfrReg(&ccpr4h,     0xf59, porv);
    add_SfrReg(&ccpr4l,     0xf58, porv);
    add_SfrReg(&ccp4con,    0xf57, porv);
    add_SfrReg(&ccpr5h,     0xf56, porv);
    add_SfrReg(&ccpr5l,     0xf55, porv);
    add_SfrReg(&ccp5con,    0xf54, porv);
    add_SfrReg(&tmr4,       0xf53, porv);
    add_SfrReg(&pr4,        0xf52, porvh);
    add_SfrReg(&t4con,      0xf51, porv);
    add_SfrReg(&tmr5h,      0xf50, porv, "tmr5h");

    add_SfrReg(&tmr5l,      0xf4f, porv, "tmr5l");
    add_SfrReg(t5con,       0xf4e, porv);
    add_SfrReg(&t5gcon,     0xf4d, porv);
    add_SfrReg(&tmr6,       0xf4c, porv);
    add_SfrReg(&pr6,        0xf4b, porvh);
    add_SfrReg(&t6con,      0xf4a, porv);
    add_SfrReg(&ccptmrs.ccptmrs0, 0xf49, porv);
    add_SfrReg(&ccptmrs.ccptmrs1, 0xf48, porv);
    add_SfrReg(&sr_module.srcon0, 0xf47, porv);
    add_SfrReg(&sr_module.srcon1, 0xf46, porv);
    ////add_SfrReg(ctmu.ctmuconh, 0xf45, porv, "ctmuconh");
    ////add_SfrReg(ctmu.ctmuconl, 0xf44, porv, "ctmuconl");
    ////add_SfrReg(ctmu.ctmuicon, 0xf43, porv, "ctmuicon");
    add_SfrReg(&vrefcon0,   0xf42, RegisterValue(0x10,0));
    add_SfrReg(&vrefcon1,   0xf41, porv);
    add_SfrReg(&vrefcon2,   0xf40, porv);

    add_SfrReg(&pmd0,       0xf3f, porv);
    add_SfrReg(&pmd1,       0xf3e, porv);
    add_SfrReg(&pmd2,       0xf3d, porv);
    add_SfrReg(&anselc,     0xf3a, RegisterValue(0xfc,0));
    add_SfrReg(&anselb,     0xf39, RegisterValue(0x3f,0));
    add_SfrReg(&ansela,     0xf38, RegisterValue(0x2f,0));

    add_SfrReg(new RegZero(this, "ZeroFD4" ),    0xFD4, porv);
    add_SfrReg(new RegZero(this, "ZeroFB5" ),    0xFB5, porv);
    add_SfrReg(new RegZero(this, "ZeroF9A" ),    0xF9A, porv);
    add_SfrReg(new RegZero(this, "ZeroF99" ),    0xF99, porv);
    add_SfrReg(new RegZero(this, "ZeroF98"),    0xF98, porv);
    add_SfrReg(new RegZero(this, "ZeroF97" ),    0xF97, porv);
    add_SfrReg(new RegZero(this, "trisd" ),    0xF95, porv);
    add_SfrReg(new RegZero(this, "ZeroF91" ),    0xF91, porv);
    add_SfrReg(new RegZero(this, "ZeroF90" ),    0xF90, porv);
    add_SfrReg(new RegZero(this, "ZeroF8F" ),    0xF8F, porv);
    add_SfrReg(new RegZero(this, "ZeroF8E" ),    0xF8E, porv);
    add_SfrReg(new RegZero(this, "late" ),    0xF8D, porv);
    add_SfrReg(new RegZero(this, "latd" ),    0xF8C, porv);
    add_SfrReg(new RegZero(this, "ZeroF88" ),    0xF88, porv);
    add_SfrReg(new RegZero(this, "ZeroF87" ),    0xF87, porv);
    add_SfrReg(new RegZero(this, "ZeroF86" ),    0xF86, porv);
    add_SfrReg(new RegZero(this, "ZeroF85" ),    0xF85, porv);
    add_SfrReg(new RegZero(this, "portd" ),    0xF83, porv);
    add_SfrReg(new RegZero(this, "ansele" ),    0xF3C, porv);
    add_SfrReg(new RegZero(this, "anseld" ),    0xF3B, porv);

    eccp1as.setBitMask(0xfc);
    eccp2as.setBitMask(0xfc);
    eccp3as.setBitMask(0xfc);
    // ECCP shutdown trigger
    eccp1as.setIOpin(0, 0, &(*m_portb)[0]);
    eccp2as.setIOpin(0, 0, &(*m_portb)[0]);
    eccp3as.setIOpin(0, 0, &(*m_portb)[0]);
    eccp1as.link_registers(&pwm1con, &ccp1con);
    eccp2as.link_registers(&pwm2con, &ccp2con);
    eccp3as.link_registers(&pwm3con, &ccp3con);
    //RRR comparator.cmcon.set_eccpas(&eccp1as);
    ccp1con.setBitMask(0xff);
    ccp2con.setBitMask(0xff);
    ccp3con.setBitMask(0xff);
    ccp4con.setBitMask(0x3f);
    ccp5con.setBitMask(0x3f);
    ccp1con.setCrosslinks(&ccpr1l, &pir1, PIR1v2::CCP1IF, &tmr2, &eccp1as);
    ccp2con.setCrosslinks(&ccpr2l, pir2, PIR2v2::CCP2IF, &tmr2, &eccp2as);
    ccp3con.setCrosslinks(&ccpr3l, &pir4, PIR4v1::CCP3IF, &tmr2, &eccp3as);
    ccp1con.pwm1con = &pwm1con;
    ccp2con.pwm1con = &pwm2con;
    ccp3con.pwm1con = &pwm3con;
    ccp1con.pstrcon = &pstr1con;
    ccp2con.pstrcon = &pstr2con;
    ccp3con.pstrcon = &pstr3con;
    ccp1con.setIOpin(&((*m_portc)[2]), &((*m_portb)[2]), &((*m_portb)[1]), &((*m_portb)[4]));
    pwm1con.setBitMask(0x80);
    //ccp3con.setCrosslinks(&ccpr3l, &pir3, PIR3v2::CCP3IF, &tmr6, &eccp3as);
    adcon0.setAdresLow(&adresl);
    adcon0.setAdres(&adresh);
    adcon0.setAdcon1(&adcon1);
    adcon0.setAdcon2(&adcon2);
    adcon0.setIntcon(&intcon);
    adcon0.setPir(&pir1);
    adcon0.setChannel_Mask(0x1f); // upto 32 channels
    adcon0.setA2DBits(10);
    adcon1.setNumberOfChannels(20);
    adcon1.setVrefHiChannel(3);
    adcon1.setVrefLoChannel(2);
    adcon1.setAdcon0(&adcon0);	// VCFG0, VCFG1 in adcon0
    vrefcon0.set_adcon1(&adcon1);
    vrefcon1.set_adcon1(&adcon1);
    vrefcon0.set_daccon0(&vrefcon1);


    ansela.setIOPin(0,  &(*m_porta)[0], &adcon1);
    ansela.setIOPin(1,  &(*m_porta)[1], &adcon1);
    ansela.setIOPin(2,  &(*m_porta)[2], &adcon1);
    ansela.setIOPin(3,  &(*m_porta)[3], &adcon1);
    ansela.setIOPin(4,  &(*m_porta)[5], &adcon1);
    anselb.setIOPin(8,  &(*m_portb)[2], &adcon1);
    anselb.setIOPin(9,  &(*m_portb)[3], &adcon1);
    anselb.setIOPin(10, &(*m_portb)[1], &adcon1);
    anselb.setIOPin(11, &(*m_portb)[4], &adcon1);
    anselb.setIOPin(12, &(*m_portb)[0], &adcon1);
    anselb.setIOPin(13, &(*m_portb)[5], &adcon1);
    anselc.setIOPin(14, &(*m_portc)[2], &adcon1);
    anselc.setIOPin(15, &(*m_portc)[3], &adcon1);
    anselc.setIOPin(16, &(*m_portc)[4], &adcon1);
    anselc.setIOPin(17, &(*m_portc)[5], &adcon1);
    anselc.setIOPin(18, &(*m_portc)[6], &adcon1);
    anselc.setIOPin(19, &(*m_portc)[7], &adcon1);

    osccon->write_mask = 0xf3;


}
void P18F26K22::set_config3h(int64_t value)
{
    PinModule *p2b;
   ( value & MCLRE) ? assignMCLRPin(1) : unassignMCLRPin();
    if(  value & P2BMX)
        p2b = &((*m_portb)[5]);
    else
        p2b = &((*m_portc)[0]);

    if(  value & CCP3MX )
        ccp3con.setIOpin(&((*m_portb)[5]), &((*m_portc)[5]));
    else
        ccp3con.setIOpin(&((*m_portc)[6]), &((*m_portc)[5]));

    if(  value & CCP2MX )
        ccp2con.setIOpin(&((*m_portc)[1]), p2b);
    else
        ccp2con.setIOpin(&((*m_portb)[3]), p2b);

    if(  value & PBADEN )
        anselb.por_value=RegisterValue( 0x3f,0);
    else
        anselb.por_value=RegisterValue(0,0);

}

void P18F26K22::osc_mode(uint value)
{
    uint mode = value &( FOSC3 | FOSC2 | FOSC1 | FOSC0);
    uint pin_Number0 =  get_osc_pin_Number(0);
    uint pin_Number1 =  get_osc_pin_Number(1);


    set_pplx4_osc(value & PLLCFG);

    if( mode == 0x8 || mode == 0x9)
    {
        if( osccon) osccon->set_config_irc(true);
        set_int_osc(true);
    }
    else
    {
        set_int_osc(false);
        if( osccon) osccon->set_config_irc(false);
    }

    if( osccon)
    {
        osccon->set_config_ieso(value & IESO);
        osccon->set_config_xosc(mode < 4);
    }

    switch(mode)
    {
    case 0xf:	// external RC CLKOUT RA6
    case 0xe:
    case 0xc:
    case 0xa:
    case 0x9:
    case 0x6:
    case 0x4:
        if( pin_Number1 < 253)  // CLKO = OSC2
        {
            cout << "CLKO not simulated\n";
            set_clk_pin(pin_Number1, get_osc_PinMonitor(1) , "CLKO",
                        false, m_porta, m_trisa, m_lata);
        }
        break;
    }
    if( pin_Number0 < 253)
    {
        if( mode != 0x9 && mode != 0x8 ) // not internal OSC, set OSC1
        {
            set_clk_pin(pin_Number0, get_osc_PinMonitor(0) , "OSC1",
                        true, m_porta, m_trisa, m_lata);
        }
        else
        {
            clr_clk_pin(pin_Number0, get_osc_PinMonitor(0),
                        m_porta, m_trisa, m_lata);
        }

    }
    if( pin_Number1 < 253)
    {
        if( mode < 4)
        {
            set_clk_pin(pin_Number1, get_osc_PinMonitor(1) , "OSC2",
                        true, m_porta, m_trisa, m_lata);
        }
        else
        {
            clr_clk_pin(pin_Number1, get_osc_PinMonitor(1),
                        m_porta, m_trisa, m_lata);
        }
    }
}

void P18F26K22::update_vdd()
{
    hlvdcon.check_hlvd();
    Processor::update_vdd();
}

Processor * P18F26K22::construct(const char *name)
{

    P18F26K22 *p = new P18F26K22(name);

    p->create();
    p->create_invalid_registers();

    return p;
}


void P18F26K22::create_sfr_map()
{
    _16bit_processor::create_sfr_map();

    RegisterValue porv(0,0);

    add_SfrReg(m_porte,       0xf84,porv);
    add_SfrReg(m_trise,       0xf96,RegisterValue(0x07,0));

    remove_SfrReg(t3con);
    add_SfrReg(t3con2,      0xfb1,porv);
    add_SfrReg(&osctune,      0xf9b,porv);
    osccon->set_osctune(&osctune);
    osctune.set_osccon(osccon);
    osccon2.set_osccon(osccon);

    comparator.cmxcon1[0]->set_OUTpin(&(*m_porta)[4], &(*m_porta)[5]);
    comparator.cmxcon1[0]->set_INpinNeg(&(*m_porta)[0], &(*m_porta)[1],
            &(*m_portb)[3],&(*m_portb)[2]);
    comparator.cmxcon1[0]->set_INpinPos(&(*m_porta)[3], &(*m_porta)[2]);
    comparator.cmxcon1[0]->setBitMask(0x3f);
    comparator.cmxcon0[0]->setBitMask(0xbf);
    comparator.cmxcon0[0]->setIntSrc(new InterruptSource(pir2, PIR2v2::C1IF));
    comparator.cmxcon0[1]->setBitMask(0xbf);
    comparator.cmxcon0[1]->setIntSrc(new InterruptSource(pir2, PIR2v2::C2IF));
    vrefcon0.set_cmModule(&comparator);
    comparator.assign_t1gcon(&t1gcon, &t3gcon, &t5gcon);
    comparator.assign_sr_module(&sr_module);
    comparator.assign_eccpsas(&eccp1as, &eccp2as, &eccp3as);
    sr_module.srcon1.set_ValidBits(0xff);
    sr_module.setPins(&(*m_portb)[0], &(*m_porta)[4], &(*m_porta)[5]);

    vrefcon1.set_cmModule(&comparator);
    vrefcon1.setDACOUT(&(*m_porta)[2]);



    ccp2con.setCrosslinks(&ccpr2l, pir2, PIR2v2::CCP2IF, &tmr2);
    ccp2con.setIOpin(&((*m_portc)[1]));     // May be altered by Config3H_2x21::set
    ccpr2l.ccprh  = &ccpr2h;
    ccpr2l.tmrl   = &tmr1l;
    ccpr2h.ccprl  = &ccpr2l;
    ccp3con.setCrosslinks(&ccpr3l, &pir3, PIR3v1::CCP3IF, &tmr2);
    ccpr3l.ccprh  = &ccpr3h;
    ccpr3l.tmrl   = &tmr1l;
    ccpr3h.ccprl  = &ccpr3l;
    ccp4con.setCrosslinks(&ccpr4l, &pir3, PIR3v1::CCP4IF, &tmr2);
    ccp4con.setIOpin(&((*m_portb)[0]));
    ccpr4l.ccprh  = &ccpr4h;
    ccpr4l.tmrl   = &tmr1l;
    ccpr4h.ccprl  = &ccpr4l;
    ccp5con.setIOpin(&((*m_porta)[4]));
    ccp5con.setCrosslinks(&ccpr5l, &pir3, PIR3v1::CCP5IF, &tmr2);
    ccpr5l.ccprh  = &ccpr5h;
    ccpr5l.tmrl   = &tmr1l;
    ccpr5h.ccprl  = &ccpr5l;

    //1 usart16.initialize_16(this,&pir_set_def,&portc);
    usart.txsta.new_name("txsta1");
    usart.txreg->new_name("txreg1");
    usart.rcsta.new_name("rcsta1");
    usart.rcreg->new_name("rcreg1");
    usart.mk_rcif_int(&pir1, PIR1v2::RCIF);
    usart.mk_txif_int(&pir1, PIR1v2::TXIF);
    add_SfrReg(&usart.spbrgh,   0xfb0,porv,"spbrgh1");
    add_SfrReg(&usart.baudcon,  0xfb8,porv,"baudcon1");
    usart.set_eusart(true);
    init_pir2(pir2, PIR2v4::TMR3IF);
    tmr3l.setIOpin(&(*m_portc)[0]);

    pir3.set_intcon(&intcon);
    pir3.set_pie(&pie3);
    pir3.set_ipr(&ipr3);
    pie3.setPir(&pir3);

    pir4.set_intcon(&intcon);
    pir4.set_pie(&pie4);
    pir4.set_ipr(&ipr4);
    pie4.setPir(&pir4);

    pir5.set_intcon(&intcon);
    pir5.set_pie(&pie5);
    pir5.set_ipr(&ipr5);
    pie5.setPir(&pir5);

   ( (T5CON *)t1con)->t1gcon = &t1gcon;
    t1gcon.setInterruptSource(new InterruptSource(&pir3, PIR3v3::TMR1GIF));
    t3gcon.setInterruptSource(new InterruptSource(&pir3, PIR3v3::TMR3GIF));
    t5gcon.setInterruptSource(new InterruptSource(&pir3, PIR3v3::TMR5GIF));

    t1gcon.set_tmrl(&tmr1l);
    t3gcon.set_tmrl(&tmr3l);
    t5gcon.set_tmrl(&tmr5l);
    t1gcon.setGatepin(&(*m_portb)[5]);
    t3gcon.setGatepin(&(*m_portc)[0]);
    t5gcon.setGatepin(&(*m_portb)[4]);

    t3con2->tmrl  = &tmr3l;
    t5con->tmrl  = &tmr5l;
   ( (T5CON *)t3con2)->t1gcon = &t3gcon;
   ( (T5CON *)t5con)->t1gcon = &t5gcon;
    tmr5l.setInterruptSource(new InterruptSource(&pir5, PIR5v1::TMR5IF));
    tmr5l.tmrh = &tmr5h;
    tmr5h.tmrl  = &tmr5l;
    tmr3l.t1con  = t3con2;
    tmr5l.t1con  = t5con;

    //cout << "Create second USART\n";
    usart2.initialize(&pir3,&(*m_portb)[6], &(*m_portb)[7],
            new _TXREG(this,"txreg2", &usart2),
            new _RCREG(this,"rcreg2", &usart2));

    add_SfrReg(&usart2.baudcon,  0xf70, porv, "baudcon2");
    add_SfrReg(&usart2.rcsta,    0xf71, porv, "rcsta2");
    add_SfrReg(&usart2.txsta,    0xf72, RegisterValue(0x02,0), "txsta2");
    add_SfrReg(usart2.txreg,     0xf73, porv, "txreg2");
    add_SfrReg(usart2.rcreg,     0xf74, porv, "rcreg2");
    add_SfrReg(&usart2.spbrg,    0xf75, porv, "spbrg2");
    add_SfrReg(&usart2.spbrgh,   0xf76, porv, "spbrgh2");

    usart2.mk_rcif_int(&pir3, PIR3v1::RCIF);
    usart2.mk_txif_int(&pir3, PIR3v1::TXIF);
    tmr2.ssp_module[0] = &ssp1;
    tmr2.ssp_module[1] = &ssp2;

    ssp1.initialize(
                0,    // PIR
                &(*m_portc)[3],   // SCK
            &(*m_porta)[5],   // SS
            &(*m_portc)[5],   // SDO
            &(*m_portc)[4],    // SDI
            m_trisc,          // i2c tris port
            SSP_TYPE_MSSP1
            );
    ssp1.mk_ssp_int(&pir1, PIR1v1::SSPIF);	// SSP1IF
    ssp1.mk_bcl_int(pir2, PIR2v2::BCLIF);	// BCL1IF

    ssp2.initialize(
                0,    // PIR
                &(*m_portb)[1],   // SCK
            &(*m_portb)[0],   // SS
            &(*m_portb)[3],   // SDO
            &(*m_portb)[2],    // SDI
            m_trisb,          // i2c tris port
            SSP_TYPE_MSSP1
            );
    ssp2.mk_ssp_int(&pir3, PIR3v3::SSP2IF);	// SSP2IF
    ssp2.mk_bcl_int(&pir3, PIR3v3::BCL2IF);	// BCL2IF

}

