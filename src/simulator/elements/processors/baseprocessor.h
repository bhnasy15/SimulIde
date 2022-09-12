/***************************************************************************
 *   Copyright (C) 2012 by santiago González                               *
 *   santigoro@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 *                                                                         *
 ***************************************************************************/

#ifndef PROCESSOR_H
#define PROCESSOR_H

//#include <QtGui>

#include "e-element.h"
#include "ramtable.h"
#include "terminalwidget.h"

class RamTable;

class MAINMODULE_EXPORT BaseProcessor : public QObject, public eElement
{
    Q_OBJECT
    public:
        BaseProcessor( QObject* parent=0 );
        ~BaseProcessor();
        
 static BaseProcessor* self() { return m_pSelf; }
 
        QString getFileName();

        virtual bool setDevice( QString device ){return false;}
        virtual void setDataFile( QString datafile );

        virtual bool loadFirmware( QString file )=0;
        virtual bool getLoadStatus() { return m_loadStatus; }
        virtual void terminate();

        virtual void setFreq( double freq );
        virtual void simuClockStep();
        virtual void stepOne();
        virtual void stepCpu()=0;
        virtual void reset()=0;

        virtual int  pc()=0;
        virtual int  status();

        virtual uint64_t cycle()=0;
        
        virtual void hardReset( bool reset );
        virtual int getRamValue( QString name );
        virtual int getRegAddress( QString name );
        virtual void addWatchVar( QString name, int address, QString type );
        virtual void updateRamValue( QString name );

        virtual uint8_t getRamValue( int address )=0;
        virtual void    setRamValue( int address, uint8_t value )=0;
        virtual uint16_t getFlashValue( int address )=0;
        virtual void     setFlashValue( int address, uint16_t value )=0;
        virtual uint8_t getRomValue( int address )=0;
        virtual void    setRomValue( int address, uint8_t value )=0;

        virtual void uartOut( int uart, uint32_t value );
        virtual void uartIn( int uart, uint32_t value );
        
        virtual void initialized();
        virtual QStringList getRegList() { return m_regList; }
        
        virtual RamTable* getRamTable() { return m_ramTable; }

        uint32_t ramSize()  { return m_ramSize; }
        uint32_t flashSize(){ return m_flashSize; }
        uint32_t romSize()  { return m_romSize; }
        uint32_t wordSize() { return m_wordSize; }

        virtual QVector<int>* eeprom();
        virtual void setEeprom( QVector<int>* eep );
        
        virtual void setRegisters();

        void setDebugging( bool d ) { m_debugging = d; }
        void setExtraStep();
        void setMain() { m_pSelf = this; }

    signals:
        void uartDataOut( int uart, int value );
        void uartDataIn(  int uart, int value );
    
    protected:
 static BaseProcessor* m_pSelf;
        
        virtual int validate( int address ) { return address; }
        
        void runSimuStep();

        uint32_t m_ramSize;
        uint32_t m_flashSize;
        uint32_t m_romSize;
        uint8_t  m_wordSize; // Size of Program memory word in bytes

        QString m_symbolFile;
        QString m_dataFile;
        QString m_device;
        QString m_statusReg;

        double   m_nextCycle;
        double   m_mcuStepsPT;
        double   m_stepNS;
        int      m_msimStep;
        uint64_t m_extraCycle;
        double   m_cpuTime;

        RamTable* m_ramTable;

        QStringList m_regList;

        QHash<QString, int>     m_regsTable;   // int max 32 bits
        QHash<QString, QString> m_typeTable;

        QVector<int> m_eeprom;

        bool m_resetStatus;
        bool m_loadStatus;
        bool m_debugging;
};


#endif

