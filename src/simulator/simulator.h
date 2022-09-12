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

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <qtconcurrentrun.h>
#include <QElapsedTimer>

#include "circmatrix.h"

class BaseProcessor;
class eElement;
class eNode;

class MAINMODULE_EXPORT Simulator : public QObject
{
    Q_OBJECT
    public:
        Simulator( QObject* parent=0 );
        ~Simulator();

 static Simulator* self() { return m_pSelf; }

        void runContinuous();
        void stopTimer();
        void resumeTimer();
        void pauseSim();
        void resumeSim();
        void stopSim();
        void stopDebug();
        void startSim();
        void debug( bool run );
        void runGraphicStep();
        void runGraphicStep1();
        void runGraphicStep2();
        void runExtraStep( uint64_t timeNs );

        void runCircuit();
        
        double stepsPerus() { return m_stepsPerus; }
        double realSpeed() { return m_realSpeed; } // 0 to 10000 => 0 to 100%
        double m_stepNano;

        uint64_t stepsPerFrame() { return m_stepsPF; }

        uint64_t step() { return m_step; }
        uint64_t circTime() { return m_circTime; } // Circuit Time in nanoseconds
        void setCircTime( uint64_t time );

        int stepsPF() { return m_stepsPF; }
        uint64_t stepsPS() { return m_stepsPS; }
        void simuRateChanged( uint64_t rate );

        int  reaClock() { return m_stepsReac; }
        void setReaClock( int value );

        int    noLinAcc() { return m_noLinAcc; }
        void   setNoLinAcc( int ac );
        double NLaccuracy() { return 1/pow(10,m_noLinAcc)/2; }
        
        bool isRunning() { return m_isrunning; }
        bool isPaused() { return m_paused; }

        void addToEnodeBusList( eNode* nod );
        void remFromEnodeBusList( eNode* nod, bool del );

        void addToEnodeList( eNode* nod );
        void remFromEnodeList( eNode* nod, bool del );
        
        void addToChangedNodeList( eNode* nod );
        
        void addToElementList( eElement* el );
        void remFromElementList( eElement* el );
        
        void addToUpdateList( eElement* el );
        void remFromUpdateList( eElement* el );

        void addToSimuClockList( eElement* el );
        void remFromSimuClockList( eElement* el );
        
        void addToChangedFast( eElement* el );
        
        void addToReactiveList( eElement* el );

        void addToNoLinList( eElement* el );

        void timerEvent( QTimerEvent* e );

        uint64_t mS(){ return m_RefTimer.elapsed(); }

    signals:
        void pauseDebug();
        void resumeDebug();
        void rateChanged();
        
    private:
 static Simulator* m_pSelf;

        inline void solveMatrix();

        QFuture<void> m_CircuitFuture;

        CircMatrix m_matrix;

        QList<eNode*>    m_eNodeList;
        QList<eNode*>    m_eNodeBusList;
        QList<eNode*>    m_eChangedNodeList;

        QList<eElement*> m_elementList;
        QList<eElement*> m_updateList;
        
        QList<eElement*> m_changedFast;
        QList<eElement*> m_reactive;
        QList<eElement*> m_nonLinear;
        QList<eElement*> m_simuClock;
        QList<BaseProcessor*> m_mcuList;

        bool m_isrunning;
        bool m_debugging;
        bool m_paused;
        bool m_error;

        int m_timerId;
        int m_timerTick;
        int m_noLinAcc;
        int m_numEnodes;
        int m_stepsReac;

        double m_stepsPerus;
        double m_realSpeed;

        uint64_t m_stepsPS;
        uint64_t m_stepsPF;
        uint64_t m_reacCounter;
        uint64_t m_updtCounter;

        uint64_t m_circTime;
        uint64_t m_step;
        uint64_t m_tStep;
        uint64_t m_lastStep;
        
        uint64_t m_refTime;
        uint64_t m_lastRefT;

        QElapsedTimer m_RefTimer;
};
 #endif


