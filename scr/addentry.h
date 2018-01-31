#ifndef ADDENTRY_H
#define ADDENTRY_H

#include <QDialog>
#include <QString>
#include <QAbstractButton>
#include <QDialogButtonBox>

namespace Ui {
class AddEntry;
}

class AddEntry : public QDialog
{
    Q_OBJECT

public:
    explicit AddEntry(QWidget *parent = 0);
    explicit AddEntry(QWidget *parent, QString EditUserIn, QString EditPassIn, QString EditDomIn);
    ~AddEntry();

    QString getUser();
    QString getDomain();
    QString getPassword();


private slots:
    void on_InUser_textChanged(const QString &arg1);
    void on_InDomain_textChanged(const QString &arg1);
    void on_InPass_textChanged(const QString &arg1);
    void on_InPass2_textChanged(const QString &arg1);
    void on_buttonBox_clicked(QAbstractButton *button);

    void on_ShowPass_toggled(bool checked);

private:
    Ui::AddEntry *ui;

    void EnableOkButton();
    void PasswordWarning();

    bool EqPass;
};

#endif // ADDENTRY_H
