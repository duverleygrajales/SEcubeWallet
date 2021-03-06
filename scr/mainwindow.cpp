#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDesktopServices>
#include <QUrl>
#include <QModelIndex>
#include <QItemSelectionModel>
#include <QDebug>
#include <QDate>
//#define DEV // uncomment this line to disable login dialog so you can test non-secube related code

#define IDENT_COL   0
#define USER_COL    1
#define DOM_COL     2
#define PASS_COL    3
#define DATE_COL    4
#define DESC_COL    5

#define CUSTOM      7

#define UNUSED(expr) (void)(expr)

#define DRIVER "QSQLITE"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow ){
    init();
}

MainWindow::~MainWindow()//Destructor
{
    if (ui->CipherClose->isEnabled()) // if there is an opened database the button is enabled
        on_CipherClose_clicked(); //Call cipher before closing
    secure_finit(); /*Once the user has finished all the operations it is strictly required to call the secure_finit() to avoid memory leakege*/

    if(db.open()){ // close anye existent connections and database
        model->clear();
        proxyModel->clear();
        db.close();
        QSqlDatabase::removeDatabase(DRIVER);
        db = QSqlDatabase();
    }

    if(s.logged_in)
        L1_logout(&s);
    delete ui;
}

void MainWindow::init()
{
    // Configure UI and initial state of buttons as not clickable because there is no wallet to edit yet
    ui->setupUi(this);
    ui->AddEntry->setEnabled(false);
    ui->EditEntry->setEnabled(false);
    ui->DeleteEntry->setEnabled(false);
    ui->LaunchEntry->setEnabled(false);
    ui->CipherClose->setEnabled(false);
    ui->Showpass->setEnabled(false);
    ui->DomainFilter->setEnabled(false);
    ui->UserFilter->setEnabled(false);
    ui->DescFilter->setEnabled(false);
    ui->olderText->setEnabled(false);
    ui->Months->setEnabled(false);
    ui->CustomMonths->setEnabled(false);
    ui->NewTable->setEnabled(false);
    ui->WalletList->setEnabled(false);
    ui->DeleteWallet->setEnabled(false);
    ui->TableTitle->setVisible(false);
    ui->WalletView->hide();

    ui->WalletView->horizontalHeader()->setStretchLastSection(true);
    setWindowTitle(tr("SEcube Wallet"));

    help = new helpWindow;

#ifndef DEV
    //SEcube Password login dialog
    LoginDialog* loginDialog = new LoginDialog( this );
    loginDialog->exec();

    if(loginDialog->result() == QDialog::Rejected){
        qDebug() << "User aborted, terminating";
        exit(1);
    }

    // Get opened session in LoginDialog
    se3_session* tmp=loginDialog->getSession();
    memcpy(&s, tmp, sizeof(se3_session));
    if(L1_crypto_set_time(&s, (uint32_t)time(0))){
        qDebug () << "Error during L1_crypto_set_time, terminating";
        exit(1);
    }

    if(secure_init(&s, -1, SE3_ALGO_MAX+1)){
        qDebug () << "Error during initialization, terminating";
        exit(1);
        /*After the board is connected and the user is correctly logged in, the secure_init() should be issued.
         * The parameter se3_session *s contains all the information that let the system acknowledge which board
         * is connected and if the user has successfully logged in.*/
    }
#endif
    return;
}


// New wallet button clicked: promt secureFileDialog and create database.
void MainWindow::on_NewWallet_clicked()
{
    SecureFileDialog *fileDialog = new SecureFileDialog( this, 1 ); //option 1 means we are creating new file
    fileDialog->exec();
    if(fileDialog->result()==QDialog::Rejected){
        return; // Error in Dialog or Cancel button, do nothing
    }
    fileName = fileDialog->getChosenFile();

    if(!fileName.isEmpty()){
        ui->TableTitle->setText(fileName); // To display name of wallet in UI.
        ui->TableTitle->setVisible(true);
        if (!OpenDataBase())
            return; //error opening database
        ui->NewTable->setEnabled(true);
        ui->DeleteWallet->setEnabled(true);
    }

    return;
}

// Create/Open sqlite dataBase
bool MainWindow::OpenDataBase (){

    if(!(QSqlDatabase::isDriverAvailable(DRIVER))) { //Check if sqlite is installed on OS
        qWarning() << "MainWindow::DatabaseConnect - ERROR: no driver " << DRIVER << " available";
        exit (1); // as the application does not work without Sqlite, exit.
    }
    if (db.open()){ //close any prev. opened connections and database
        model->clear();
        proxyModel->clear();
        db.close();
        QSqlDatabase::removeDatabase(DRIVER);
        db = QSqlDatabase();
    }

    db = QSqlDatabase::addDatabase(DRIVER);
    db.setDatabaseName(fileName);
    if(!db.open()){ //Check if it was possible to open the database
        qWarning() << "ERROR: " << db.lastError();
        return false;
    }

    ui->WalletList->clear();

    QStringList CurrentTables = db.tables(QSql::Tables);
    if (!CurrentTables.isEmpty()){
        ui->WalletList->setEnabled(true);
        ui->WalletList->addItems(CurrentTables);
    }
    ui->NewTable->setEnabled(true);
    ui->WalletList->setEnabled(true);
    ui->DeleteWallet->setEnabled(true);
    ui->TableTitle->setText(fileName); // To display name of wallet in UI.
    ui->TableTitle->setVisible(true);
    ui->OpenCyphered->setEnabled(false);
    ui->CipherClose->setEnabled(true);

    return true;

}

//After table is created in database, display it in a QTableView, and manage it using a Table Model
void MainWindow::CreateViewTable(const QString &WalletName ){

    // Create and configure model to access table easily
    model = new QSqlTableModel;
    model->setTable(WalletName);
    model->select();
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);// as the table will be not edditable, the changes are updated by calling submitAll()

    //Create ProxyModel for filtering
    proxyModel = new MySortFilterProxyModel(this);
    proxyModel->setSourceModel(model); //connect proxyModel to Model

    //Connect the model to the view table: Sqltable -> Model -> ProxyModel ->  TableView
    ui->WalletView->setModel(proxyModel);
    //Configure the table
    //ui->WalletView->setSortingEnabled(true);//enable sorting
    ui->WalletView->setEditTriggers(QAbstractItemView::NoEditTriggers);// To make the table view not edditable
    ui->WalletView->setColumnHidden(IDENT_COL, true);//Hide Identification column, not important for user, only for sqlite.
    ui->WalletView->verticalHeader()->hide();
    ui->WalletView->setColumnHidden(PASS_COL, true);//Initially hide passwords column
    ui->WalletView->setSelectionBehavior( QAbstractItemView::SelectItems ); //To allow only one row selection...
    ui->WalletView->setSelectionMode( QAbstractItemView::SingleSelection ); //So we can edit one entry per time
    ui->WalletView->show();// show table, initially hidden


    //As we now have a wallet, we can enable the buttons
    ui->AddEntry->setEnabled(true);
    ui->EditEntry->setEnabled(true);
    ui->DeleteEntry->setEnabled(true);
    ui->LaunchEntry->setEnabled(true);
    ui->Showpass->setEnabled(true);
    ui->Showpass->setChecked(false);
    ui->CipherClose->setEnabled(true);
    ui->DomainFilter->setEnabled(true);
    ui->UserFilter->setEnabled(true);
    ui->DescFilter->setEnabled(true);
    ui->olderText->setEnabled(true);
    ui->Months->setEnabled(true);
    ui->CustomMonths->setEnabled(true);

    return;
}


//Call dialog and populate Table with the aqcuired data
void MainWindow::on_AddEntry_clicked(){

    AddEntry *add = new AddEntry(this);
    add->exec();
    if(add->result()==QDialog::Rejected){
        return; // Error or cancel, do nothing
    }

    QSqlRecord rec = model->record(); // Temp entry
    rec.setGenerated("id", false); // The ID column is managed automatically by the model, not by the user
    rec.setValue("Username", add->getUser());
    rec.setValue("Password",add->getPassword());
    rec.setValue("Domain",add->getDomain());
    rec.setValue("Description",add->getDescription());
    rec.setValue("Date", QDate::currentDate());

    int newRecNo = model->rowCount(); //Where to insert the new record?: at the end of the table
    if (model->insertRecord(newRecNo, rec))
        model->submitAll();// if insert ok, submit changes.

    return;
}

void MainWindow::on_WalletView_doubleClicked(const QModelIndex &index){
    MainWindow::on_EditEntry_clicked();
    UNUSED(index);
}

//Call dialog to allow user edit the data
void MainWindow::on_EditEntry_clicked()
{
    QModelIndexList selection = ui->WalletView->selectionModel()->selectedIndexes();//Get Items selected in table
    model->submitAll();//Submit any pending changes before continue. (cannot be done before select bcs it deletes it)

    // If Multiple rows can be selected (Disabled for edit. Could be good for delete)
    for(int i=0; i< selection.count(); i++){
        int row = selection.at(i).row();

        //Call add entry with second constructor, so we can pass it the values to edit
        AddEntry *add = new AddEntry(this, proxyModel->index(row,USER_COL).data().toString(),
                                           proxyModel->index(row,PASS_COL).data().toString(),
                                           proxyModel->index(row,DOM_COL).data().toString(),
                                           proxyModel->index(row,DESC_COL).data().toString());
        add->exec();

        if(add->result()==QDialog::Rejected)
            return; // If error or cancel, do nothing

        //Change the values in proxy
        proxyModel->setData(proxyModel->index(row,USER_COL), add->getUser());
        proxyModel->setData(proxyModel->index(row,PASS_COL), add->getPassword());
        proxyModel->setData(proxyModel->index(row,DOM_COL),  add->getDomain());
        proxyModel->setData(proxyModel->index(row,DESC_COL), add->getDescription());
        proxyModel->setData(proxyModel->index(row,DATE_COL), QDate::currentDate());

        if(!model->submitAll()) //Submit changes to database
            qDebug() << "Error: " << model->lastError();
    }
    return;
}

void MainWindow::on_DeleteEntry_clicked()
{
    QModelIndexList selection = ui->WalletView->selectionModel()->selectedIndexes();//Get Items selected in table
    model->submitAll();//Submit any pending changes before continue. (cannot be done before select bcs it deletes it)

    // If Multiple rows can be selected (disabled to allow edit)
    for(int i=0; i< selection.count(); i++){
        //Before continuing, promt confirmation dialog
        DeleteConfirmation * conf = new DeleteConfirmation;
        conf->exec();
        if(conf->result()==QDialog::Rejected)
            return;//if error or cancel, do nothing

        QModelIndex index = selection.at(i);
        proxyModel->removeRow(index.row());
        if(!model->submitAll()) //Submit changes if no errors
            qDebug() << "Error: " << model->lastError();
    }
    return;
}

void MainWindow::on_LaunchEntry_clicked(){

    QModelIndexList selection = ui->WalletView->selectionModel()->selectedIndexes();//Get Items selected in table
    model->submitAll();//Submit any pending changes before continue. (cannot be done before select bcs it deletes it)

    // If Multiple rows can be selected (Disabled for edit. Could be good for delete)
    for(int i=0; i< selection.count(); i++){
        int row = selection.at(i).row();
        QString domain=proxyModel->index(row,DOM_COL).data().toString();
        QString domainaux=domain;
        if (!domain.startsWith("http://") && !domain.startsWith("https://"))
            domain = "http://" + domain;

        if (domain.contains("www.")){
            if (!domainaux.remove("www.").contains("."))
                domain = domain+".com";
        }else if (!domain.contains("."))
            domain = domain+".com";
        qDebug() <<domain;
        QDesktopServices::openUrl(QUrl(domain));
    }
}

//When user wants, make passwords visible
void MainWindow::on_Showpass_toggled(bool checked){
    ui->WalletView->setColumnHidden(PASS_COL, !checked);
    return;
}

//To change envirolment, key ID and encryption
void MainWindow::on_EnvironmentBut_clicked(){
    EnvironmentDialog *envDialog = new EnvironmentDialog(this, &s);
    envDialog->exec();
    return;
}

//Cipher database file and delete the plain file. (From SeFile_IMG example)
void MainWindow::on_CipherClose_clicked(){
    uint16_t ret = SE3_OK;
    char enc_filename[65];
    uint16_t enc_len = 0;

    memset(enc_filename, 0, 65);
    crypto_filename(fileName.toUtf8().data(), enc_filename, &enc_len );

    QString *path_plainfile = new QString(fileName);
    if((ret = secure_open(path_plainfile->toUtf8().data(), &sefile_file, SEFILE_WRITE, SEFILE_NEWFILE))){
        if ( ret == SEFILE_SIGNATURE_MISMATCH )  // Check if signature on header failed
            qDebug() << "Signature Mismatch";
        else
            qDebug() << "Error in open sec file: " << ret;

        return; //Something went wrong with open file, do nothing
    }

    //fileName holds the current database file
    QFile plain_file(fileName);
    plain_file.open(QIODevice::ReadOnly); //only need to read it
    qint64 size_len = plain_file.size();

    // Begin encryption of file
    uint8_t *buffer = (uint8_t*) malloc((size_t)size_len*sizeof(uint8_t));
    memcpy(buffer, plain_file.readAll().data(), (size_t)size_len);
    if((ret = secure_write(&sefile_file, buffer, size_len))) {
        if ( ret == SEFILE_SIGNATURE_MISMATCH )
            qDebug() << "Signature Mismatch";
        else
            qDebug() << "Error in open sec file: " << ret;

        free(buffer);
        secure_close(&sefile_file);
        plain_file.close();
        return; //Something went wrong with open file, do nothing
    }

    free(buffer);
    secure_close(&sefile_file);
    plain_file.close();

    // If encryption ok, delete plain file and set UI to "not wallet" state
    QFile::remove(fileName);
    ui->WalletView->hide();

    ui->AddEntry->setEnabled(false);
    ui->EditEntry->setEnabled(false);
    ui->DeleteEntry->setEnabled(false);
    ui->CipherClose->setEnabled(false);
    ui->LaunchEntry->setEnabled(false);
    ui->TableTitle->setVisible(false);
    ui->Showpass->setEnabled(false);
    ui->DomainFilter->setEnabled(false);
    ui->UserFilter->setEnabled(false);
    ui->DescFilter->setEnabled(false);
    ui->olderText->setEnabled(false);
    ui->Months->setEnabled(false);
    ui->CustomMonths->setEnabled(false);
    ui->NewTable->setEnabled(false);
    ui->WalletList->setEnabled(false);
    ui->DeleteWallet->setEnabled(false);
    ui->OpenCyphered->setEnabled(true);

    if(db.open()){ // close anye existent connections and database
        db.close();
        QSqlDatabase::removeDatabase(DRIVER);
        db = QSqlDatabase();
    }

    return;
}

//Open Cyphered database and display it
void MainWindow::on_OpenCyphered_clicked(){
    QByteArray plain_buffer;
    uint8_t *buffer;
    uint16_t ret = SE3_OK;
    uint32_t nBytesRead = 0, file_len = 0;

    //call securefileDialog. Option 0 displays the cyphered files in the current directory
    SecureFileDialog *fileDialog = new SecureFileDialog( this, 0 );
    fileDialog->exec();
    if(fileDialog->result()==QDialog::Rejected){
        return ;
    }

    path = fileDialog->getChosenFile();
    fileName = QFileInfo(QFile(path)).fileName();

    if(fileName.isNull()) return;
    path = path.left(path.size() - fileName.size());

    QString *path_plainfile = new QString(path + fileName);

    //Open file
    if((ret = secure_open(path_plainfile->toUtf8().data(), &sefile_file, SEFILE_WRITE, SEFILE_OPEN))){
        if ( ret == SEFILE_SIGNATURE_MISMATCH )  // Check if signature on header failed
            qDebug() << "Signature Mismatch";
        else
            qDebug() << "Error in open sec file: " << ret;

        return; //Something went wrong with open file, do nothing
    }

    //Get size
    if((ret = secure_getfilesize(path_plainfile->toUtf8().data(), &file_len))){
        if ( ret == SEFILE_SIGNATURE_MISMATCH )
            qDebug() << "Signature Mismatch";
        else if (ret > 0)
            qDebug() << "Error in open sec file: " << ret;
        secure_close(&sefile_file);
        return;
    }

    //Start decrypthion
    buffer = (uint8_t *)calloc(file_len, sizeof(uint8_t));
    if((ret = secure_read(&sefile_file,  buffer, file_len, &nBytesRead))){
        if ( ret == SEFILE_SIGNATURE_MISMATCH )
            qDebug() << "Signature Mismatch";
        else if (ret > 0)
            qDebug() << "Error in open sec file: " << ret;
        secure_close(&sefile_file);
        return;
    }

    plain_buffer = QByteArray((char *)buffer, (int)nBytesRead);
    QFile plain_file(fileName); //Create plain fail
    plain_file.open(QIODevice::WriteOnly); //We need to write it
    plain_file.write(plain_buffer);//Write
    plain_file.close();//Close

    //Re-Open data base and create table.
    if (!OpenDataBase())
        return; //error opening database
    //CreateViewTable();
    return;
}

void MainWindow::on_DomainFilter_textChanged(const QString &arg1){ //update Domain filter
    proxyModel->setFilterDomain(arg1);
}

void MainWindow::on_UserFilter_textChanged(const QString &arg1){ //Update User filter
    proxyModel->setFilterUser(arg1);
}

void MainWindow::on_DescFilter_textChanged(const QString &arg1){
    proxyModel->setFilterDesc(arg1);
}

void MainWindow::on_Months_currentIndexChanged(int index){
    proxyModel->setFilterOlder(index, ui->CustomMonths->text());
}

void MainWindow::on_CustomMonths_textChanged(const QString &arg1){
    proxyModel->setFilterOlder(ui->Months->currentIndex(), arg1);
}

void MainWindow::on_Help_clicked(){
    help->show();
}

void MainWindow::on_NewTable_clicked(){

    NewTable *newT = new NewTable(this);
    newT->exec();
    if(newT->result()==QDialog::Rejected){
        return; // Error or cancel, do nothing
    }
    QString WalletName = newT->getName();

    QSqlQuery query;
    query.prepare("create table "+WalletName+
                  "(id integer primary key, "
                  "Username TEXT, "
                  "Domain TEXT, "
                  "Password TEXT, "
                  "Date TEXT, "
                  "Description TEXT )"); //The table

    if (!query.exec())
        qWarning() << "Couldn't create the table" << WalletName <<" one might already exist";

    query.finish();

    ui->WalletList->setEnabled(true);
    if(ui->WalletList->findText(WalletName) == -1) //Check if item already in the list
        ui->WalletList->addItem(WalletName); //if not, add it
    ui->WalletList->setCurrentText(WalletName);

    CreateViewTable(WalletName);
}

void MainWindow::on_WalletList_currentIndexChanged(const QString &arg1){ //just change the view to the selected wallet
    CreateViewTable(arg1);
}

void MainWindow::on_DeleteWallet_clicked(){
    QString WalletName=ui->WalletList->currentText();
    if (!WalletName.isEmpty()){
        DeleteConfirmation * conf = new DeleteConfirmation;
        conf->exec();
        if(conf->result()==QDialog::Rejected)
            return;//if error or cancel, do nothing

        QSqlQuery query(db);

        query.prepare("DROP TABLE "+WalletName);
        if (!query.exec()){
            qWarning() << "Couldn't delete table" << WalletName;
            qWarning() << "ERROR: " << query.lastError();
            return;
        }
        ui->WalletList->removeItem(ui->WalletList->currentIndex());
    }
}
