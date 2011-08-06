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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <qapplication.h>
#include <iostream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lineEdit->setText(QDir::homePath());
    connect(ui->browse_button, SIGNAL(clicked()),
            this, SLOT(openFile()));
    connect(this, SIGNAL(fileAnalysisUpdate(int)),
            this, SLOT(fileAnalysisUpdated(int)));
    connect(this, SIGNAL(fileAnalysisComplete()),
            this, SLOT(fileAnalysisCompleted()));
    connect(ui->process_button, SIGNAL(clicked()),
            this, SLOT(processFile()));
    connect(this, SIGNAL(fileProcessingUpdate(qint64,qint64)),
            this, SLOT(fileProcessingUpdated(qint64,qint64)));
    connect(this, SIGNAL(fileProcessingFailure(QString,bool)),
            this, SLOT(fileProcessingFailed(QString,bool)));
    connect(this, SIGNAL(fileProcessingFinish()),
            this, SLOT(fileProcessingFinished()));
    connect(this, SIGNAL(kickOffPreset()),
            this, SLOT(openFile()));
    ui->statusBar->showMessage(tr("Please select an .alog file..."));
    ui->progressBar->setValue(0);
    ui->process_button->setEnabled(false);
    this->canceled = false;
    this->filename = "";
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::presetFile(QString filename) {
    this->filename = filename;
    this->preset_file = true;
    emit this->kickOffPreset();
}

void MainWindow::processFile() {
    ui->process_button->setEnabled(false);
    ui->quit_button->setText("Cancel");
    QFileInfo file_info(this->filename);
    QDir file_dir = file_info.dir();
    QString file_name = file_info.fileName();
    QDir dest_dir(file_dir.filePath(file_name.section("", 0, -7) + "_expanded"));
    if(dest_dir.exists()) {
        QMessageBox msgBox;
        msgBox.setText("Expanded folder already exists and will be overwritten. Would you like to continue?");
        msgBox.addButton(QMessageBox::Yes);
        QAbstractButton *nobutton = msgBox.addButton(QMessageBox::No);
        if(!this->preset_file)
            msgBox.exec();
            if(msgBox.clickedButton() == nobutton) {
                this->reset();
                return;
            }
        if(!FileUtils::removeDir(dest_dir.path())) {
            QMessageBox msgBox;
            msgBox.setText("Error: cannot delete old path.");
            msgBox.exec();
            this->reset();
            return;
        }
    }
    QtConcurrent::run(this, &MainWindow::_processFile, dest_dir);
}

void MainWindow::_processFile(QDir dest_dir) {
    QFile file(this->filename);

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit this->fileProcessingFailure("Error: could not open the file specified.", true);
        this->reset();
        return;
    }

    if(!dest_dir.mkdir(dest_dir.path())) {
        emit this->fileProcessingFailure("Error: cannot create output path.", true);
        return;
    }

    // Open the expand log file
    QFile log_file(dest_dir.filePath("expansion.log"));
    if(!log_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit this->fileProcessingFailure("Error: could not open the log file.", true);
        return;
    }

    // Open the Matlab file
    QFile mat_file(dest_dir.filePath("import_channels.m"));
    if(!mat_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit this->fileProcessingFailure("Error: could not open the matlab script file.", true);
        return;
    }
    mat_file.write("clear; clc;\n\n");


    mat_file.write(QString("filename='" +filename.section("", 0, -7) + ".mat';\n\n").toUtf8());

    // Parse the file and write the channels
    QHash<QString, QFile *> file_hash;
    qint64 file_count = 0;
    while(!file.atEnd() && !this->canceled) {
        QString line(file.readLine());
        file_count += 1;
        if(file_count % 10000 == 0) {
            emit fileProcessingUpdate(file_count, (qint64)(file_count*100.0/this->number_of_lines));
        }
        if(line.startsWith('%')) {
            log_file.write("Skipping line with %: ");
            log_file.write(line.toUtf8());
            log_file.write("\n");
            continue;
        }
        QStringList split_line = line.split(" ", QString::SkipEmptyParts);
        // Clear out any useless files

        if(split_line.length() < 4) {
            split_line = line.split("\t", QString::SkipEmptyParts);
        }
        if(split_line.length() < 4) {
            log_file.write("Skipping line with malformed elements: ");
            log_file.write(line.toUtf8());
            log_file.write("\n");
            continue;
        }

        int channel_name_number;
        if(this->ui->checkBox->isChecked()) {
            channel_name_number = 2;
        } else {
            channel_name_number = 1;
        }

        if(!file_hash.contains(split_line[channel_name_number])) {
            file_hash.insert(split_line[channel_name_number], new QFile(dest_dir.filePath(split_line[channel_name_number]+".csv")));
            if(!file_hash.value(split_line[channel_name_number])->open(QIODevice::WriteOnly | QIODevice::Text)) {
                emit this->fileProcessingFailure("Error: could not open channel file.", true);
                break;
            }
            // Put line in matlab file
            mat_file.write(QString("% Loading "+split_line[channel_name_number]+"\ntry\n").toUtf8());
            mat_file.write(QString("     load('"+split_line[channel_name_number]+".csv');\n").toUtf8());
            mat_file.write(QString("catch err\n     display('Error loading "+split_line[channel_name_number]+":');\n").toUtf8());
            mat_file.write("     display(err.message);\nend\n\n");
        }

        QFile *file_handle;
        file_handle = file_hash.value(split_line[channel_name_number]);
        file_handle->write(split_line[0].toUtf8());
        file_handle->write(",");
        file_handle->write(split_line[3].toUtf8());
        file_handle->write("\n");
    }

    mat_file.write("save(filename);\n\nclear;\n\n");

    QHashIterator<QString, QFile *> i(file_hash);
    while (i.hasNext()) {
        i.next();
        file_hash.value(i.key())->flush();
        file_hash.value(i.key())->close();
        delete file_hash.value(i.key());
    }

    if(this->canceled) {
        emit this->fileProcessingFailure("Error: file processing canceled, data may be incomplete.", true);
        this->canceled = false;
        this->cancel2quit();
    } else {
        emit this->fileProcessingFinish();
    }
}

void MainWindow::fileProcessingFinished() {
    this->cancel2quit();
    ui->progressBar->setValue(100);
    ui->process_button->setEnabled(true);
    ui->statusBar->showMessage("Finished processing "+QString("").setNum(this->number_of_lines)+" lines.");
    if(this->preset_file)
        QApplication::exit(0);
}

void MainWindow::fileProcessingFailed(QString error_msg, bool reset) {
    if(!error_msg.isEmpty()) {
        QMessageBox msgBox;
        msgBox.setText(error_msg);
        msgBox.exec();
    }
    if(reset) {
        this->reset();
    } else {
        ui->process_button->setEnabled(false);
        this->cancel2quit();
    }
}

void MainWindow::cancel2quit() {
    disconnect(ui->quit_button, SIGNAL(clicked()),
               this, SLOT(processCanceled()));
    ui->quit_button->setText("Quit");
    connect(ui->quit_button, SIGNAL(clicked()),
            this, SLOT(close()));
    //ui->checkBox->setEnabled(false);
}

void MainWindow::quit2cancel() {
    disconnect(ui->quit_button, SIGNAL(clicked()),
               this, SLOT(close()));
    ui->quit_button->setText("Cancel");
    connect(ui->quit_button, SIGNAL(clicked()),
            this, SLOT(processCanceled()));
    //ui->checkBox->setEnabled(true);
}

void MainWindow::processCanceled() {
    this->canceled = true;
}

void MainWindow::fileProcessingUpdated(qint64 file_count, qint64 progress) {
    ui->progressBar->setValue(progress);
    ui->statusBar->showMessage("Processing "+QString("").setNum(file_count)+" of "+QString("").setNum(this->number_of_lines)+" lines.");
}

void MainWindow::openFile() {
    if(this->filename == "") {
        this->filename = QFileDialog::getOpenFileName(this,
             tr("Open .alog file"), QDir::homePath(), tr(".alog Files (*.alog)"));
    }

    ui->lineEdit->setText(this->filename);
    this->quit2cancel();
    QtConcurrent::run(this, &MainWindow::analyzeFile, this->filename);
}


void MainWindow::analyzeFile(QString filename) {
    QFile file(filename);

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    int counter = 0;
    char buf[4048];
    qint64 linelength;
    long long linetotal = 0;
    while(!file.atEnd()) {
        linelength = file.readLine(buf, sizeof(buf));
        linetotal += linelength;
        if(counter % 10000 == 0) {
            emit fileAnalysisUpdate((int)(linetotal*100/file.size()));
        }
        counter += 1;
    }

    this->number_of_lines = counter;

    file.close();

    emit fileAnalysisComplete();
}

void MainWindow::fileAnalysisCompleted() {
    QString counter_str;
    counter_str.setNum(this->number_of_lines);
    ui->statusBar->showMessage("Ready to process " + counter_str + " lines.");
    ui->process_button->setEnabled(true);
    ui->progressBar->setValue(0);
    this->cancel2quit();
    if(this->preset_file)
        ui->process_button->click();
}

void MainWindow::fileAnalysisUpdated(int progress) {
    ui->progressBar->setValue(progress);
}

void MainWindow::reset() {
    ui->process_button->setEnabled(false);
    ui->progressBar->setValue(0);
    ui->statusBar->showMessage(tr("Please select an .alog file..."));
    ui->lineEdit->setText("");
}
