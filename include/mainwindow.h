/*
Copyright (c) 2010, William J. Woodall IV
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
in the documentation and/or other materials provided with the distribution.  Neither the name of the QTMaps Library nor the 
names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QDir>
#include <QtConcurrentRun>
#include <QMessageBox>
#include <QHash>

#include "fileutils.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void presetFile(QString filename);
    ~MainWindow();

public slots:
    void openFile();
    void fileAnalysisUpdated(int progress);
    void fileAnalysisCompleted();
    void processFile();
    void fileProcessingUpdated(qint64 file_count, qint64 progress);
    void fileProcessingFailed(QString error_msg, bool reset);
    void processCanceled();
    void fileProcessingFinished();

signals:
    void kickOffPreset();
    void fileAnalysisUpdate(int progress);
    void fileAnalysisComplete();
    void fileProcessingUpdate(qint64 file_count, qint64 progress);
    void fileProcessingFailure(QString error_msg, bool reset);
    void fileProcessingFinish();

private:
    Ui::MainWindow *ui;
    void analyzeFile(QString filename);
    void _processFile(QDir dest_dir);
    void cancel2quit();
    void quit2cancel();
    void reset();
    int number_of_lines;
    bool canceled;
    QString filename;
    bool preset_file;
};

#endif // MAINWINDOW_H
