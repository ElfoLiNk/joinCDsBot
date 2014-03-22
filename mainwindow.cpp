#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QProcess>
#include <QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    format = ".avi";
    type = "CD";
#ifdef Q_OS_WIN
    mkvmerge = "C:\\Program Files\\MKVToolNix\\mkvmerge.exe";
    ui->programLineEdit->setText(mkvmerge);
#endif
#ifdef Q_OS_UNIX
    mkvmerge = "mkvmerge";
    ui->programLineEdit->setText(mkvmerge);
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_folderPushButton_clicked()
{
    QFileDialog folderDlg;
    folder = folderDlg.getExistingDirectory(0,"Caption",QString(),QFileDialog::ShowDirsOnly);
    ui->folderLineEdit->setText(folder);
    QDir recoredDir(folder);
    allFiles = recoredDir.entryList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
    //(QDir::Filter::Files,QDir::SortFlag::NoSort)
    partFiles = allFiles.filter(type,Qt::CaseSensitive);
    partFiles = partFiles.filter(format,Qt::CaseSensitive);
    ui->joinPushButton->setEnabled(true);
}

void MainWindow::on_joinPushButton_clicked()
{
    if(!partFiles.isEmpty()){
        do{
            findFilmPart();

            process = new QProcess(this);
            QObject::connect(process, SIGNAL(readyReadStandardError()),this, SLOT(updateOutputTextEdit()));
            QObject::connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),this, SLOT(processFinished(int, QProcess::ExitStatus)));
            QObject::connect(process, SIGNAL(error(QProcess::ProcessError)),this, SLOT(processError(QProcess::ProcessError)));

            QString output = '"'+folder+"/"+film.remove(" "+type+"1"+format)+".mkv"+'"';
            for(int i = 0; i < filmPart.size();i++){
                filmCD.append('"'+folder+"/"+filmPart.at(i)+'"');
            }

            QStringList arguments;
            arguments.append("-o");
            arguments.append(output);

            for(int i = 0;i < filmCD.size();i++){
                //process->setNativeArguments(filmCD.at(i));
                if(i==0){
                    arguments.append(filmCD.at(i));
                }else{
                    arguments.append("+ "+filmCD.at(i));
                }
            }

#ifdef Q_OS_WIN
            QString command = "";
            for(int i = 0; i < arguments.size();i++)
            {
                command = command+" "+arguments.at(i);
            }

            QString batfile = film.remove(" "+type+"1"+format)+".bat";
            QFile filebat(batfile);
            filebat.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream out(&filebat);
            out << '"'+mkvmerge+'"'+command;

            filebat.close();

            process->start("cmd.exe", QStringList() << "/c" << batfile);
            if(process->waitForStarted(60000)){
                if(process->waitForFinished(320000)){
                    //ui->logTextEdit->append(process->readAllStandardOutput());
                }
            }

            QFile file(batfile);
            file.setPermissions(QFile::ReadOther | QFile::WriteOther);
            file.remove();
            file.close();
#endif
#ifdef Q_OS_UNIX
            process->start(mkvmerge, arguments);
            if(process->waitForStarted(60000)){
                if(process->waitForFinished(320000)){
                    //ui->logTextEdit->append(process->readAllStandardOutput());
                }
            }

#endif
        }while(ui->autoCheckBox->isChecked() && !partFiles.isEmpty());
    }else{
        ui->logTextEdit->append("Nothing to join");
    }
}

void MainWindow::updateOutputTextEdit()
{
    QByteArray newData = process->readAllStandardError();
    QString text = ui->logTextEdit->toPlainText()
            + QString::fromLocal8Bit(newData);
    ui->logTextEdit->setPlainText(text);
}


void MainWindow::findFilmPart() {
    film = partFiles.at(0);
    filmPart.append(partFiles.filter(film.remove(" "+type+"1"+format),Qt::CaseSensitive));
    for(int i = 0; i < filmPart.size();i++){
        partFiles.removeAll(filmPart.at(i));
    }
}

void MainWindow::processFinished(int exitCode,
                                 QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit) {
        ui->logTextEdit->append(tr("Join program crashed"));
    } else if (exitCode != 0) {
        ui->logTextEdit->append(tr("Join failed"));
        ui->autoCheckBox->setChecked(false);
        ui->logTextEdit->append(process->readAllStandardOutput());
    } else {
        ui->logTextEdit->append(tr("File %1 created").arg(film));
        for(int i = 0;i < filmPart.size();i++){
            QFile filefilm(folder+"/"+filmPart.at(i));
            filefilm.setPermissions(QFile::ReadOther | QFile::WriteOther);
            bool info = filefilm.remove();
            QString error = filefilm.errorString();
            filefilm.close();
        }
        filmCD.clear();
        filmPart.clear();
    }
    ui->joinPushButton->setEnabled(true);
}

void MainWindow::processError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        ui->logTextEdit->append(tr("Join program not found"));
        ui->joinPushButton->setEnabled(true);
    }
}

void MainWindow::on_programLineEdit_textEdited(const QString &arg1)
{
    mkvmerge = arg1;
}

void MainWindow::on_typeLineEdit_textEdited(const QString &arg1)
{
    type = arg1;
}

void MainWindow::on_formatLineEdit_textEdited(const QString &arg1)
{
    format = arg1;
}
