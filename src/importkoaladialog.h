/****************************************************************************
Copyright 2016 Ricardo Quesada

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
****************************************************************************/

#pragma once

#include <QDialog>

namespace Ui {
class ImportKoalaDialog;
}

class ImportKoalaDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportKoalaDialog(QWidget *parent = 0);
    ~ImportKoalaDialog();

    const QString& getFilepath() const;

protected:
    bool validateKoalaFile(const QString& filepath);
    bool processChardef(const std::string& key, quint8 *outKey, quint8* outColorRAM);


private slots:
    void on_pushButton_clicked();

    void on_pushButtonConvert_clicked();

private:

    Ui::ImportKoalaDialog *ui;
    bool _validKoalaFile;
    QString _filepath;
};
