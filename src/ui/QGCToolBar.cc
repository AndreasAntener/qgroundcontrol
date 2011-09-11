/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

#include <QToolButton>
#include <QLabel>
#include "QGCToolBar.h"
#include "UASManager.h"
#include "MainWindow.h"

QGCToolBar::QGCToolBar(QWidget *parent) :
    QToolBar(parent),
    toggleLoggingAction(NULL),
    logReplayAction(NULL),
    mav(NULL)
{
    setObjectName("QGC_TOOLBAR");

    toggleLoggingAction = new QAction(QIcon(":"), "Logging", this);
    toggleLoggingAction->setCheckable(true);
    logReplayAction = new QAction(QIcon(":"), "Replay", this);
    logReplayAction->setCheckable(true);

    addSeparator();

    addAction(toggleLoggingAction);
    addAction(logReplayAction);

    // CREATE TOOLBAR ITEMS
    // Add internal actions
    // Add MAV widget
    symbolButton = new QToolButton(this);
    nameLabel = new QLabel("------", this);
    modeLabel = new QLabel("------", this);
    stateLabel = new QLabel("------", this);
    wpLabel = new QLabel("---", this);
    distlabel = new QLabel("--- ---- m", this);
    messageLabel = new QLabel("No system messages.", this);
    //symbolButton->setIcon(":");
    symbolButton->setStyleSheet("QWidget { background-color: #050508; color: #DDDDDF; background-clip: border; } QToolButton { font-weight: bold; font-size: 12px; border: 0px solid #999999; border-radius: 5px; min-width:22px; max-width: 22px; min-height: 22px; max-height: 22px; padding: 0px; margin: 0px; background-color: none; }");
    addWidget(symbolButton);
    addWidget(nameLabel);
    addWidget(modeLabel);
    addWidget(stateLabel);
    addWidget(wpLabel);
    addWidget(distlabel);
    addWidget(messageLabel);

    // DONE INITIALIZING BUTTONS

    setActiveUAS(UASManager::instance()->getActiveUAS());
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
}

void QGCToolBar::setLogPlayer(QGCMAVLinkLogPlayer* player)
{
    connect(toggleLoggingAction, SIGNAL(triggered(bool)), player, SLOT(playPause(bool)));
    connect(logReplayAction, SIGNAL(triggered(bool)), this, SLOT(logging(bool)));
}

void QGCToolBar::logging(bool enabled)
{
    // Stop logging in any case
    MainWindow::instance()->getMAVLink()->enableLogging(false);
    if (enabled)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Specify MAVLink log file name"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("MAVLink Logfile (*.mavlink *.log *.bin);;"));

        if (!fileName.endsWith(".mavlink"))
        {
            fileName.append(".mavlink");
        }

        QFileInfo file(fileName);
        if (file.exists() && !file.isWritable())
        {
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setText(tr("The selected logfile is not writable"));
            msgBox.setInformativeText(tr("Please make sure that the file %1 is writable or select a different file").arg(fileName));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
        }
        else
        {
            MainWindow::instance()->getMAVLink()->setLogfileName(fileName);
            MainWindow::instance()->getMAVLink()->enableLogging(true);
        }
    }
}

void QGCToolBar::addPerspectiveChangeAction(QAction* action)
{
    insertAction(toggleLoggingAction, action);
}

void QGCToolBar::setActiveUAS(UASInterface* active)
{
    // Do nothing if system is the same or NULL
    if ((active == NULL) || mav == active) return;

    if (mav)
    {
        // Disconnect old system
        disconnect(mav, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*,QString,QString)));
        disconnect(mav, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
        disconnect(mav, SIGNAL(nameChanged(QString)), this, SLOT(updateName(QString)));
        disconnect(mav, SIGNAL(systemTypeSet(UASInterface*,uint)), this, SLOT(setSystemType(UASInterface*,uint)));
        disconnect(mav, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(receiveTextMessage(int,int,int,QString)));
    }

    // Connect new system
    mav = active;
    connect(active, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*, QString,QString)));
    connect(active, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
    connect(active, SIGNAL(nameChanged(QString)), this, SLOT(updateName(QString)));
    connect(active, SIGNAL(systemTypeSet(UASInterface*,uint)), this, SLOT(setSystemType(UASInterface*,uint)));
    connect(active, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(receiveTextMessage(int,int,int,QString)));

    // Update all values once
    nameLabel->setText(mav->getUASName());
    modeLabel->setText(mav->getShortMode());
    stateLabel->setText(mav->getShortState());
    setSystemType(mav, mav->getSystemType());
}

void QGCToolBar::createCustomWidgets()
{

}

void QGCToolBar::updateState(UASInterface* system, QString name, QString description)
{
    Q_UNUSED(system);
    Q_UNUSED(description);
    stateLabel->setText(tr(" State: %1").arg(name));
}

void QGCToolBar::updateMode(int system, QString name, QString description)
{
    Q_UNUSED(system);
    Q_UNUSED(description);
    modeLabel->setText(tr(" Mode: %1").arg(name));
}

void QGCToolBar::updateName(const QString& name)
{
    nameLabel->setText(name);
}

/**
 * The current system type is represented through the system icon.
 *
 * @param uas Source system, has to be the same as this->uas
 * @param systemType type ID, following the MAVLink system type conventions
 * @see http://pixhawk.ethz.ch/software/mavlink
 */
void QGCToolBar::setSystemType(UASInterface* uas, unsigned int systemType)
{
    Q_UNUSED(uas);
        // Set matching icon
        switch (systemType) {
        case 0:
            symbolButton->setIcon(QIcon(":/images/mavs/generic.svg"));
            break;
        case 1:
            symbolButton->setIcon(QIcon(":/images/mavs/fixed-wing.svg"));
            break;
        case 2:
            symbolButton->setIcon(QIcon(":/images/mavs/quadrotor.svg"));
            break;
        case 3:
            symbolButton->setIcon(QIcon(":/images/mavs/coaxial.svg"));
            break;
        case 4:
            symbolButton->setIcon(QIcon(":/images/mavs/helicopter.svg"));
            break;
        case 5:
            symbolButton->setIcon(QIcon(":/images/mavs/unknown.svg"));
            break;
        default:
            symbolButton->setIcon(QIcon(":/images/mavs/unknown.svg"));
            break;
        }
}

void QGCToolBar::receiveTextMessage(int uasid, int componentid, int severity, QString text)
{
    Q_UNUSED(uasid);
    Q_UNUSED(componentid);
    Q_UNUSED(severity);
    messageLabel->setText(text);
}

QGCToolBar::~QGCToolBar()
{
    delete toggleLoggingAction;
    delete logReplayAction;
}
